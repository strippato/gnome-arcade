/* gnome-arcade
 * Copyright (c) 2014 Strippato strippato@gmail.com
 *
 * This file is part of gnome-arcade.
 *
 * gnome-arcade is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gnome-arcade is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gnome-arcade.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* app.h */
#ifndef APP_H
#define APP_H

#define APP_ID 				"org.gnome.arcade"
#define APP_NAME      		"Arcade"
#define APP_DESCRIPTION 	"the best arcade from '70 '80 '90"
#define APP_VERSION     	"0.0.2"
#define APP_AUTHOR      	"Strippato"
#define APP_AUTHOR_EMAIL	"strippato@gmail.com"
#define APP_LICENSE      	"This software is distributed under the terms of the\n"\
							"GNU General Public License 3.0"
#define APP_COPYRIGHT       "Copyright© 2014 Strippato"
#define APP_TRANSLATORS     "Strippato"
#define APP_WEB             "https://github.com/strippato"
#define APP_GNU_WARN        "You should have received a copy of the GNU GPL along with gnome-arcade.\n"\
							"If not, see <http://www.gnu.org/licenses/>."
#ifndef APP_RES
	#define APP_RESOURCE 		"./res/"
#else
	#define APP_RESOURCE 		APP_RES "/res/"
#endif

#ifndef MAME_BIN
	#define MAME_BIN 		"/usr/bin/mame"
#endif

#define APP_DIRCONFIG  		"gnome-arcade"
#define APP_CSS				"gnome-arcade.css"

#define APP_ICON    		"gnome-arcade.png"
#define APP_ICON_ABOUT 		"gnome-arcade-about.png"
#define APP_NOIMAGE    		"noimage.png"
#define APP_NOWSHOWING 		"nowshowing.png"
#define APP_LOADING			"loading.png"
#define APP_PREFERRED       "preferred.png"
#define APP_RANK            "rank.png"

#define APP_SELECT_RANK_ON  "rankon.png"
#define APP_SELECT_RANK_OFF "rankoff.png"

#define APP_SELECT_FAV_ON   "prefon.png"
#define APP_SELECT_FAV_OFF  "prefoff.png"

#define LOCALE_DIR 			"/usr/share/locale"
#define LOCALE_PACKAGE    	"gnome-arcade"

extern const gchar *app_authors[];
extern const gchar *app_artists[];

extern GtkApplication *app_application;
extern GActionEntry app_entries[];
#endif

