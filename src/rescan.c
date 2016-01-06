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
#include <glib/gstdio.h>
#include <string.h>

#include "global.h"
#include "app.h"
#include "rom.h"
#include "mame.h"
#include "view.h"
#include "ui.h"
#include "rescan.h"


void
rescan (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{

	if (ui_inSelectState()) return;

	g_print("rescanning...\n");

    while (gtk_events_pending ())
            gtk_main_iteration ();

	// delete the romlist
    gchar *fileRom = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, MAME_LIST_FULL_FILE, NULL);
    if (!g_file_test (fileRom, G_FILE_TEST_EXISTS)) {
    	g_unlink(fileRom);
    }
    g_free (fileRom);

	// delete the clonelist
    gchar *fileClone = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, MAME_LIST_CLONES_FILE, NULL);
    if (!g_file_test (fileClone, G_FILE_TEST_EXISTS)) {
    	g_unlink(fileClone);
    }
    g_free (fileClone);

    /*view */
    view_free ();

    // free romlist
    rom_free ();

    // reinit
    rom_init ();

    rom_load ();

    rom_setSort (ROM_SORT_AZ);

    /* view */
    view_init ();

	view_gotoDefaultView ();


	g_print("rescanning done\n");
}
