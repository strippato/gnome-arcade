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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

#include "app.h"
#include "global.h"
#include "uipref.h"
#include "config.h"

#include "rom.h"
#include "www.h"

// TODO check all the path
void
uipref_showDialog (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{

	static GtkWidget *dialog = NULL;
	GtkWidget *label;

	if (dialog) {
        gtk_window_present (GTK_WINDOW (dialog));
        return;
    }

	GtkWindow *win = gtk_application_get_active_window (app_application);

	dialog = gtk_dialog_new_with_buttons ("Preferences",
	                                    GTK_WINDOW (win),
	                                    GTK_DIALOG_DESTROY_WITH_PARENT,
	                                    "_Cancel", GTK_RESPONSE_CANCEL,
	                                    "_OK", GTK_RESPONSE_OK,
	                                    NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	GtkWidget *table = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (table), 4);
	gtk_grid_set_column_spacing (GTK_GRID (table), 4);

	gtk_box_pack_start (GTK_BOX (content_area), table, TRUE, TRUE, 20);

	// mame
	label = gtk_label_new_with_mnemonic ("_M.A.M.E. path");
	gtk_widget_set_tooltip_text (label, "Path to MAME program");
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
	GtkWidget *mamePath = gtk_entry_new ();
	gtk_widget_set_tooltip_text (mamePath, MAME_BIN);
	gtk_entry_set_width_chars (GTK_ENTRY (mamePath), 50);
	gtk_entry_set_text (GTK_ENTRY (mamePath), cfg_keyStr ("MAME_EXE"));
	gtk_grid_attach (GTK_GRID (table), mamePath, 1, 0, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), mamePath);

	// rom
	label = gtk_label_new_with_mnemonic ("_rom path");
	gtk_widget_set_tooltip_text (label, "Path to your romset");
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);
	GtkWidget *romPath = gtk_entry_new ();
	gtk_widget_set_tooltip_text (romPath, "/usr/share/gnome-arcade/data/rom/");
	gtk_entry_set_text (GTK_ENTRY (romPath), cfg_keyStr ("ROM_PATH"));
	gtk_grid_attach (GTK_GRID (table), romPath, 1, 1, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), romPath);

	// tile
	label = gtk_label_new_with_mnemonic ("_tile path");
	gtk_widget_set_tooltip_text (label, "Path to your tileset");
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (table), label, 0, 2, 1, 1);
	GtkWidget *tilePath = gtk_entry_new ();
	gtk_widget_set_tooltip_text (tilePath, "/usr/share/gnome-arcade/data/tile/");
	gtk_entry_set_text (GTK_ENTRY (tilePath), cfg_keyStr ("TILE_PATH"));
	gtk_grid_attach (GTK_GRID (table), tilePath, 1, 2, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), tilePath);

	// web provider
	label = gtk_label_new_with_mnemonic ("web _provider");
	gtk_widget_set_tooltip_text (label, "Tile will be downloaded from this link");
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (table), label, 0, 4, 1, 1);

 	GtkWidget *webProvider = gtk_combo_box_text_new ();

    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://www.progettoemma.net/snap/%s/0000.png", "snapshot@www.progettoemma.net");
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://mrdo.mameworld.info/mame_artwork/%s.png", "snapshot@mrdo.mameworld.info");
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://www.mamedb.com/snap/%s.png", "snapshot@www.mamedb.com");

    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://www.progettoemma.net/snap/%s/title.png", "title@www.progettoemma.net");
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://www.mamedb.com/titles/%s.png", "title@www.mamedb.com");

    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://www.progettoemma.net/snap/%s/flyer.png", "flyer@www.progettoemma.net");

    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://www.progettoemma.net/snap/%s/score.png", "score@www.progettoemma.net");

    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://www.mamedb.com/cabinets/%s.png", "cabinet@www.mamedb.com");

    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (webProvider), "http://www.progettoemma.net/snap/%s/gameover.png", "gameover@www.progettoemma.net");

    gtk_grid_attach (GTK_GRID (table), webProvider, 1, 4, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), webProvider);
	g_object_set (webProvider, "active-id", cfg_keyStr ("WEB_PROVIDER"), NULL);

	// www
	label = gtk_label_new_with_mnemonic ("_web path");
	gtk_widget_set_tooltip_text (label, "Tile downloaded form the web provider, will be stored in this directory");
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (table), label, 0, 3, 1, 1);
	GtkWidget *webPath = gtk_entry_new ();
	gtk_widget_set_tooltip_text (webPath, "~/gnome-arcade/data/www/");
	gtk_entry_set_text (GTK_ENTRY (webPath), cfg_keyStr ("WEB_PATH"));
	gtk_grid_attach (GTK_GRID (table), webPath, 1, 3, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), webPath);

	gtk_widget_show_all (table);
	gint response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_OK) {
		// TODO input check
		cfg_setConfig ("MAME_EXE", gtk_entry_get_text (GTK_ENTRY (mamePath)));
		cfg_setConfig ("ROM_PATH", gtk_entry_get_text (GTK_ENTRY (romPath)));
		cfg_setConfig ("TILE_PATH", gtk_entry_get_text (GTK_ENTRY (tilePath)));
		cfg_setConfig ("WEB_PATH", gtk_entry_get_text (GTK_ENTRY (webPath)));

		gchar *link = NULL;
		g_object_get (webProvider, "active-id", &link, NULL);
		cfg_setConfig ("WEB_PROVIDER", link);
		g_free (link);

		if (cfg_saveConfig ()) {
			g_print ("config saved " SUCCESS_MSG "\n");

			// reconfigure web downloader
			www_free ();
			www_init ();

			// fixme reconfigure on mame/tile/path change
		} else {
			g_print ("can't save config " FAIL_MSG "\n");
		}
	}

	gtk_widget_destroy (dialog);
	dialog = NULL;
}
