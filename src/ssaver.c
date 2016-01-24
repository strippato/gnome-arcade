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

#include <stdio.h>
#include <gtk/gtk.h>

#include "global.h"
#include "view.h"
#include "rom.h"
#include "app.h"
#include "view.h"
#include "ui.h"
#include "config.h"
#include "ssaver.h"

static gboolean
ssaver_xdgCommand (gchar *cmdline)
{
    GError *error = NULL;

	if (!g_spawn_command_line_sync (cmdline, NULL, NULL, NULL, &error)) {
		g_error_free (error);
		return FALSE;
	} else {
		return TRUE;
	}
}

void
ssaver_init (void)
{
}

void
ssaver_free (void)
{
}

void
ssaver_resume (void)
{
	if (cfg_keyInt ("SCREENSAVER_MODE") == 0) return;

	unsigned int xwin = ui_getWindowXid ();
    gchar *cmd = g_strdup_printf ("xdg-screensaver resume 0x%x", xwin);

    if (ssaver_xdgCommand (cmd)) {
    	g_print ("enable screen saver " SUCCESS_MSG "\n");
    } else {
    	g_print ("enable screen saver " FAIL_MSG "\n");
    }

    g_free (cmd);
}

void
ssaver_suspend (void)
{
	if (cfg_keyInt ("SCREENSAVER_MODE") == 0) return;

	unsigned int xwin = ui_getWindowXid ();
    gchar *cmd = g_strdup_printf ("xdg-screensaver suspend 0x%x", xwin);

    if (ssaver_xdgCommand (cmd)) {
    	g_print ("disable screen saver " SUCCESS_MSG "\n");
    } else {
	    g_print ("disable screen saver " FAIL_MSG "\n");
    }
    g_free (cmd);
}

