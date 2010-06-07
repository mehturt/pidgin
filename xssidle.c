/*
 * Xss Idle Plugin
 *
 * Copyright (C) 2010, mehturt <mehturt@gmail.com>
 * 
 * Based on the Purple's Hello World Plugin
 * 
 * Copyright (C) 2004, Gary Kramlich <grim@guifications.org>,
 *               2007, John Bailey <rekkanoryo@cpw.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 *
 */
#ifndef PURPLE_PLUGINS
# define PURPLE_PLUGINS
#endif

#include <glib.h>

/* This will prevent compiler errors in some instances and is better explained in the
 * how-to documents on the wiki */
#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif

#include <plugin.h>
#include <version.h>
#include <idle.h>
#include <debug.h>
#include <X11/extensions/scrnsaver.h>

time_t get_time_idle(void);

PurpleIdleUiOps my_idle_ui_ops =
{
	get_time_idle,
	NULL,
	NULL,
	NULL,
	NULL
};

time_t get_time_idle(void)
{
	purple_debug_info("xssidle",
	            "Called get_time_idle\n");

	/* Get threshold value */
	gint away_seconds;

	away_seconds = 60 *
		purple_prefs_get_int("/purple/away/mins_before_away");

	/* Query xscreensaver */
	Display* dpy;
	time_t idle_time = 0;
	int state = ScreenSaverOff;

	dpy = XOpenDisplay(0);
	if (dpy)
	{
		XScreenSaverInfo i;
		Status s = XScreenSaverQueryInfo(dpy,
			DefaultRootWindow(dpy),
			&i);
		if (s != 0)
		{
			idle_time = i.idle / 1000;
			state = i.state;

			/* Adjust idle time in case screen saver is
			 * on.
			 */
			if (idle_time < away_seconds &&
				state == ScreenSaverOn)
			{
				idle_time = away_seconds + 1;
			}
		}
		XCloseDisplay(dpy);
	}

	purple_debug_info("xssidle",
	            "Called get_time_idle, idle %d, state %d\n",
		    (int)idle_time,
		    state);
	return idle_time;
}


static gboolean
plugin_load (PurplePlugin * plugin)
{
	purple_idle_set_ui_ops(&my_idle_ui_ops);

	return TRUE;
}

/* For specific notes on the meanings of each of these members, consult the C Plugin Howto
 * on the website. */
static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"gnt-xssidle",
	"Xss idle",
	"0.1",

	"Xss Idle Plugin",
	"Set idle time based on information from X11 Screen Saver"
	" (such as xscreensaver)",
	"mehturt <mehturt@gmail.com>",
	"http://github.com/mehturt/pidgin",


	plugin_load,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin (PurplePlugin * plugin)
{
}

PURPLE_INIT_PLUGIN (xssidle, init_plugin, info)
