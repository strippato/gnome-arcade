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

// TODO check all the path
void
uipref_showDialog (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{

	static GtkWidget *dialog = NULL;


	GtkWidget *label;

	GtkWindow *win = gtk_application_get_active_window (app_application);

	if (dialog) {
        gtk_window_present (GTK_WINDOW (dialog));
        return;
    }

	dialog = gtk_dialog_new_with_buttons ("Preferences",
	                                    GTK_WINDOW (win),
	                                    GTK_DIALOG_DESTROY_WITH_PARENT,
	                                    "_OK", GTK_RESPONSE_OK,
	                                    "_Cancel", GTK_RESPONSE_CANCEL,
	                                    NULL);

	GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	GtkWidget *table = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (table), 4);
	gtk_grid_set_column_spacing (GTK_GRID (table), 4);

	gtk_box_pack_start (GTK_BOX (content_area), table, TRUE, TRUE, 0);

	// mame
	label = gtk_label_new_with_mnemonic ("_M.A.M.E. path");
	gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
	GtkWidget *mamePath = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY(mamePath), 40);
	gtk_entry_set_text (GTK_ENTRY (mamePath), cfg_keyStr ("MAME_EXE"));
	gtk_grid_attach (GTK_GRID (table), mamePath, 1, 0, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), mamePath);

	// rom
	label = gtk_label_new_with_mnemonic ("_rom path");
	gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);
	GtkWidget *romPath = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (romPath), cfg_keyStr ("ROM_PATH"));
	gtk_grid_attach (GTK_GRID (table), romPath, 1, 1, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), romPath);

	// tile
	label = gtk_label_new_with_mnemonic ("_tile path");
	gtk_grid_attach (GTK_GRID (table), label, 0, 2, 1, 1);
	GtkWidget *tilePath = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (tilePath), cfg_keyStr ("TILE_PATH"));
	gtk_grid_attach (GTK_GRID (table), tilePath, 1, 2, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), tilePath);

	// www
	label = gtk_label_new_with_mnemonic ("_web path");
	gtk_grid_attach (GTK_GRID (table), label, 0, 3, 1, 1);
	GtkWidget *webPath = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (webPath), cfg_keyStr ("WEB_PATH"));
	gtk_grid_attach (GTK_GRID (table), webPath, 1, 3, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), webPath);

	// FIXME better a combobox
	// web provider
	label = gtk_label_new_with_mnemonic ("web _provider");
	gtk_grid_attach (GTK_GRID (table), label, 0, 4, 1, 1);
	GtkWidget *webProvider = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (webProvider), cfg_keyStr ("WEB_PROVIDER"));
	gtk_grid_attach (GTK_GRID (table), webProvider, 1, 4, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), webProvider);

	gtk_widget_show_all (table);
	gint response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_OK) {
		g_print ("MAME PATH->%s\n", gtk_entry_get_text (GTK_ENTRY (mamePath)));
		g_print ("ROM PATH->%s\n", gtk_entry_get_text (GTK_ENTRY (romPath)));
		g_print ("TILE PATH->%s\n", gtk_entry_get_text (GTK_ENTRY (tilePath)));
		g_print ("WEB PATH->%s\n", gtk_entry_get_text (GTK_ENTRY (webPath)));
		g_print ("WEB PRIVIDER->%s\n", gtk_entry_get_text (GTK_ENTRY (webProvider)));
		// TODO input cehck & save
	}
	gtk_widget_destroy (dialog);
	dialog = NULL;
}
