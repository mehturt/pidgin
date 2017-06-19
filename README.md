A few pidgin/finch/purple plugins.
Tested against:
* finch 2.6.6 on Ubuntu 10.04.
* finch 2.11 on Debian jessie

# Required packages
* libncurses5-dev
* finch-dev
* libxss-dev
* libpurple-dev
* libglib2.0-dev

# Build and install

* make && make install

# Plugins

## xssidle
Set idle time based on information from X11 Screen Saver (such as
xscreensaver).

In configuration, the _Report Idle Time_ must be set to
**Based on keyboard use**.

## betterhistory
Shows recently logged conversations in new conversations.
When a new conversation is opened this plugin will insert
the last conversation into the current conversation.

