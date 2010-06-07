/**
 * Better gnt history plugin
 *
 * Copyright (C) 2010, mehturt <mehturt@gmail.com>
 *
 * Based on the Purple's gnthistory plugin
 *
 * Copyright (C) 2006 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#ifndef PURPLE_PLUGINS
# define PURPLE_PLUGINS
#endif

#define PREFS_BASE		"/plugins/gnt/betterhistory"
#define PREFS_NUMBER_MSGS	"/number_msgs"
#define PREFS_TODAY		"/today"

#include "conversation.h"
#include "debug.h"
#include "log.h"
#include "request.h"
#include "prefs.h"
#include "signals.h"
#include "util.h"
#include "version.h"

#include "gntconv.h"
#include "gntplugin.h"
#include "gntrequest.h"

static void historize(PurpleConversation *c)
{
	PurpleAccount *account = purple_conversation_get_account(c);
	const char *name = purple_conversation_get_name(c);
	PurpleConversationType convtype;
	GList *logs = NULL;
	const char *alias = name;
	PurpleLogReadFlags flags;
	char *history;
	char *header;
	PurpleMessageFlags mflag;
	int number_history;
	int size;
	GList* p = NULL;
	time_t now;
	struct tm today;
	gboolean display_messages_from_today;

	convtype = purple_conversation_get_type(c);
	if (convtype == PURPLE_CONV_TYPE_IM) {
		GSList *buddies;
		GSList *cur;
		FinchConv *fc = FINCH_CONV(c);
		if (fc->list && fc->list->next) /* We were already in the middle of a conversation. */
			return;

		/* If we're not logging, don't show anything.
		 * Otherwise, we might show a very old log. */
		if (!purple_prefs_get_bool("/purple/logging/log_ims"))
			return;

		/* Find buddies for this conversation. */
		buddies = purple_find_buddies(account, name);

		/* If we found at least one buddy, save the first buddy's alias. */
		if (buddies != NULL)
			alias = purple_buddy_get_contact_alias((PurpleBuddy *)buddies->data);

		for (cur = buddies; cur != NULL; cur = cur->next) {
			PurpleBlistNode *node = cur->data;
			if ((node != NULL) &&
					((purple_blist_node_get_sibling_prev(node) != NULL) ||
						(purple_blist_node_get_sibling_next(node) != NULL))) {
				PurpleBlistNode *node2;

				alias = purple_buddy_get_contact_alias((PurpleBuddy *)node);

				/* We've found a buddy that matches this conversation.  It's part of a
				 * PurpleContact with more than one PurpleBuddy.  Loop through the PurpleBuddies
				 * in the contact and get all the logs. */
				for (node2 = purple_blist_node_get_first_child(purple_blist_node_get_parent(node));
						node2 != NULL ; node2 = purple_blist_node_get_sibling_next(node2)) {
					logs = g_list_concat(
							purple_log_get_logs(PURPLE_LOG_IM,
								purple_buddy_get_name((PurpleBuddy *)node2),
								purple_buddy_get_account((PurpleBuddy *)node2)),
							logs);
				}
				break;
			}
		}
		g_slist_free(buddies);

		if (logs == NULL)
			logs = purple_log_get_logs(PURPLE_LOG_IM, name, account);
		else
			logs = g_list_sort(logs, purple_log_compare);
	} else if (convtype == PURPLE_CONV_TYPE_CHAT) {
		/* If we're not logging, don't show anything.
		 * Otherwise, we might show a very old log. */
		if (!purple_prefs_get_bool("/purple/logging/log_chats"))
			return;

		logs = purple_log_get_logs(PURPLE_LOG_CHAT, name, account);
	}

	if (logs == NULL)
		return;

	purple_debug_info("betterhistory",
		"Logs length is %d\n",
		g_list_length(logs));

	number_history = purple_prefs_get_int(PREFS_NUMBER_MSGS);
	size = g_list_length(logs) > (number_history - 1) ?
		number_history - 1 :
		g_list_length(logs) - 1;
	p = logs;

	int i = 0;
	for (; i < size; ++i)
	{
		p = p->next;
	}

	time(&now);
	localtime_r(&now, &today);

	display_messages_from_today =
		purple_prefs_get_bool(PREFS_TODAY);

	/* Iterate over selected logs. */
	do
	{
		mflag = PURPLE_MESSAGE_NO_LOG | PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_DELAYED;
		history = purple_log_read((PurpleLog*)p->data, &flags);

		/* Check for today's messages, if configured. */
		if (TRUE == display_messages_from_today)
		{
			struct tm log_time;

			localtime_r(&((PurpleLog*)p->data)->time, &log_time);

			if (log_time.tm_mday != today.tm_mday ||
				log_time.tm_mon != today.tm_mon ||
				log_time.tm_year != today.tm_year)
			{
				p = p->prev;
				continue;
			}
		}

		header = g_strdup_printf("<b>Conversation with %s on %s:</b><br>", alias,
				purple_date_format_full(localtime(&((PurpleLog *)p->data)->time)));
		purple_conversation_write(c, "", header, mflag, time(NULL));
		g_free(header);

		if (flags & PURPLE_LOG_READ_NO_NEWLINE)
			purple_str_strip_char(history, '\n');
		purple_conversation_write(c, "", history, mflag, time(NULL));
		g_free(history);

		purple_conversation_write(c, "", "<hr>", mflag, time(NULL));

		p = p->prev;
	}
	while (p != NULL);

	g_list_foreach(logs, (GFunc)purple_log_free, NULL);
	g_list_free(logs);
}

static void
history_prefs_check(PurplePlugin *plugin)
{
	if (!purple_prefs_get_bool("/purple/logging/log_ims") &&
	    !purple_prefs_get_bool("/purple/logging/log_chats"))
	{
		PurpleRequestFields *fields = purple_request_fields_new();
		PurpleRequestFieldGroup *group;
		PurpleRequestField *field;
		struct {
			const char *pref;
			const char *label;
		} prefs[] = {
			{"/purple/logging/log_ims", "Log IMs"},
			{"/purple/logging/log_chats", "Log chats"},
			{NULL, NULL}
		};
		int iter;
		GList *list = purple_log_logger_get_options();
		const char *system = purple_prefs_get_string("/purple/logging/format");

		group = purple_request_field_group_new("Logging");

		field = purple_request_field_list_new("/purple/logging/format", "Log format");
		while (list) {
			const char *label = list->data;
			list = g_list_delete_link(list, list);
			purple_request_field_list_add(field, label, list->data);
			if (system && strcmp(system, list->data) == 0)
				purple_request_field_list_add_selected(field, label);
			list = g_list_delete_link(list, list);
		}
		purple_request_field_group_add_field(group, field);

		for (iter = 0; prefs[iter].pref; iter++) {
			field = purple_request_field_bool_new(prefs[iter].pref, prefs[iter].label,
						purple_prefs_get_bool(prefs[iter].pref));
			purple_request_field_group_add_field(group, field);
		}

		purple_request_fields_add_group(fields, group);

		purple_request_fields(plugin, NULL, "History Plugin Requires Logging",
			"Logging can be enabled from Tools -> Preferences -> Logging.\n\n"
			"Enabling logs for instant messages and/or chats will activate "
			"history for the same conversation type(s).",
			fields,
			"OK", G_CALLBACK(finch_request_save_in_prefs),
			"Cancel", NULL,
			NULL, NULL, NULL, plugin);
	}
}

static void history_prefs_cb(const char *name, PurplePrefType type,
							 gconstpointer val, gpointer data)
{
	history_prefs_check((PurplePlugin *)data);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_signal_connect(purple_conversations_get_handle(),
			"conversation-created",
			plugin, PURPLE_CALLBACK(historize), NULL);

	purple_prefs_connect_callback(plugin, "/purple/logging/log_ims",
			history_prefs_cb, plugin);
	purple_prefs_connect_callback(plugin, "/purple/logging/log_chats",
			history_prefs_cb, plugin);

	history_prefs_check(plugin);

	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin) {

	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	pref = purple_plugin_pref_new_with_name(PREFS_NUMBER_MSGS);
	purple_plugin_pref_set_label(pref, "Number of history messages "
		"(0 for all)");
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name(PREFS_TODAY);
	purple_plugin_pref_set_label(pref, "Display only today's messages");
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame,
	0,	/* page_num (Reserved) */
	NULL,	/* frame (Reserved) */

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"gnt-betterhistory",
	"BetterHistory",
	"0.1",

	"Shows recently logged conversations in new conversations.",
	"When a new conversation is opened this plugin will insert "
	"the last conversation into the current conversation.",
	"mehturt <mehturt@gmail.com>\n"
	" based on gnthistory by\n"
	"Sean Egan <seanegan@gmail.com>\n"
	"Sadrul H Chowdhury <sadrul@users.sourceforge.net>",
	"http://github.com/mehturt/pidgin",

	plugin_load,
	NULL,
	NULL,
	NULL,
	NULL,
	&prefs_info,	/**< prefs_info */
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
	purple_prefs_add_none(PREFS_BASE);
	purple_prefs_add_int(PREFS_NUMBER_MSGS, 5);
	purple_prefs_add_bool(PREFS_TODAY, false);
}

PURPLE_INIT_PLUGIN(betterhistory, init_plugin, info)
