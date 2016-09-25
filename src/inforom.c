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
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdarg.h>

#include "global.h"
#include "view.h"
#include "rom.h"
#include "app.h"
#include "ui.h"
#include "mame.h"
#include "util.h"
#include "config.h"
#include "pref.h"
#include "vlc.h"
#include "inforom.h"

static GtkWidget *ir_romName = NULL;
static GtkWidget *ir_year = NULL;
static GtkWidget *ir_manufacturer = NULL;
static GtkWidget *ir_romOf = NULL;
static GtkWidget *ir_chd = NULL;
static GtkWidget *ir_src = NULL;

// forward declaration
static void inforom_updateInfo (struct inforom_info* info);

struct inforom_info*
inforom_build (const gchar *romName,
                const gchar* description,
                const gchar* manufacturer,
                const gchar* year,
                const gchar* romOf,
                const gchar* srcFile,
                gboolean chd)
{
    struct inforom_info *info = g_malloc0 (sizeof (struct inforom_info));
    info->name = g_strdup (romName);
    info->description = g_strdup (description);
    info->manufacturer = g_strdup (manufacturer);
    info->romOf = g_strdup (romOf);
    info->year = g_strdup (year);
    info->srcFile = g_strdup (srcFile);
    info->chd = chd;

    return info;
}

void
inforom_free (struct inforom_info *info)
{
    g_free (info->name);
    g_free (info->description);
    g_free (info->manufacturer);
    g_free (info->romOf);
    g_free (info->year);
    g_free (info->srcFile);
    info->chd = FALSE;

    g_free (info);
    info = NULL;
}

static void
inforom_cmbSelectItem_cb (GtkWidget *sender, GtkWidget *da)
{
    GtkComboBox *cmb = GTK_COMBO_BOX (sender);

    gchar *itm = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));

    // get the rom name
    gchar **vclone = g_strsplit_set (itm, "[]", -1);
    gchar **ptrTxt = NULL;

    for (ptrTxt = vclone; *ptrTxt; ++ptrTxt) {}

    gchar *romName = g_strdup (*(ptrTxt-2));
    g_print ("preview of %s\n", romName);

    struct inforom_info *info = mame_getInfoRom (romName);

    inforom_updateInfo (info);

    mame_freeInfoRom (info);

    vlc_stopVideo ();
    vlc_playVideo (romName, da);

    g_strfreev (vclone);
    g_free (romName);
    g_free (itm);
}

static void
inforom_updateInfo (struct inforom_info* info)
{
    gtk_label_set_text (GTK_LABEL (ir_romName), info->name);
    gtk_label_set_text (GTK_LABEL (ir_year), info->year);
    gtk_label_set_text (GTK_LABEL (ir_manufacturer), info->manufacturer);
    gtk_label_set_text (GTK_LABEL (ir_romOf), info->romOf ? info->romOf : "✗");
    gtk_label_set_text (GTK_LABEL (ir_chd), info->chd ? "✓" : "✗");
    gtk_label_set_text (GTK_LABEL (ir_src), info->srcFile);
    // clone's tag = parent's tag: no update
}

void
inforom_show (struct rom_romItem *item)
{
    if (!item) return;

    struct inforom_info *info = mame_getInfoRom (rom_getItemName (item));
    GtkWindow *win = gtk_application_get_active_window (app_application);

    GtkWidget *dialog = gtk_dialog_new_with_buttons ("Rom information",
                                        GTK_WINDOW (win),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        NULL, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkWidget *da = gtk_drawing_area_new ();
    GtkWidget *cmbItem = gtk_combo_box_text_new ();

    gchar *itm = g_strdup_printf ("%s [%s]", info->description, info->name);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cmbItem), NULL, itm);
    g_free (itm);
    gtk_combo_box_set_active (GTK_COMBO_BOX (cmbItem), 0);

    g_signal_connect (GTK_WIDGET (cmbItem), "changed", G_CALLBACK (inforom_cmbSelectItem_cb), da);

    if (rom_isParent (info->name)) {
        if (g_hash_table_contains (rom_parentTable, info->name)) {
            gchar *l = g_hash_table_lookup (rom_parentTable, info->name);

            gchar **strv = g_strsplit (l, "\n", -1);
            gchar **ptr = NULL;

            for (ptr = strv; *ptr; ++ptr) {

                gchar *cloneTxt = g_strdup ((*ptr));
                gchar **vclone = g_strsplit_set (cloneTxt, "[]", -1);
                gchar **ptrTxt = NULL;

                for (ptrTxt = vclone; *ptrTxt; ++ptrTxt) {}

                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cmbItem), NULL, (*ptr));

                g_strfreev (vclone);
                g_free (cloneTxt);
            }

            g_strfreev (strv);
        }
    }

    GtkWidget *table = gtk_grid_new ();
    gtk_grid_set_row_spacing (GTK_GRID (table), 10);
    gtk_grid_set_column_spacing (GTK_GRID (table), 10);

    GtkWidget *tableTxt = gtk_grid_new ();
    gtk_grid_set_row_spacing (GTK_GRID (tableTxt), 15);
    gtk_grid_set_column_spacing (GTK_GRID (tableTxt), 5);

    gtk_box_pack_start (GTK_BOX (content_area), table, TRUE, TRUE, 10);

    GtkWidget *label;

    // romname
    label = gtk_label_new ("Rom name");
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_start (GTK_WIDGET (label), 10);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 0, 1, 1, 1);

    ir_romName = gtk_label_new (info->name);
    gtk_widget_set_halign (GTK_WIDGET (ir_romName), GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (tableTxt), ir_romName, 1, 1, 1, 1);

    // Year
    label = gtk_label_new ("Year");
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_start (GTK_WIDGET (label), 10);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 0, 2, 1, 1);

    ir_year = gtk_label_new (info->year);
    gtk_widget_set_halign (GTK_WIDGET (ir_year), GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (tableTxt), ir_year, 1, 2, 1, 1);

    // manufacturer
    label = gtk_label_new ("Maker");
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_start (GTK_WIDGET (label), 10);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 0, 3, 1, 1);

    ir_manufacturer = gtk_label_new (info->manufacturer);
    gtk_widget_set_halign (GTK_WIDGET (ir_manufacturer), GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (tableTxt), ir_manufacturer, 1, 3, 1, 1);

    // rom of
    label = gtk_label_new ("Rom of");
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_start (GTK_WIDGET (label), 10);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 0, 4, 1, 1);

    ir_romOf = gtk_label_new (info->romOf ? info->romOf : "✗");
    gtk_widget_set_halign (GTK_WIDGET (ir_romOf), GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (tableTxt), ir_romOf, 1, 4, 1, 1);

    // CHD
    label = gtk_label_new ("Chd");
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_start (GTK_WIDGET (label), 10);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 0, 5, 1, 1);

    ir_chd = gtk_label_new (info->chd ? "✓" : "✗");
    gtk_widget_set_halign (GTK_WIDGET (ir_chd), GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (tableTxt), ir_chd, 1, 5, 1, 1);

    // source
    label = gtk_label_new ("Source");
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_start (GTK_WIDGET (label), 10);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 0, 6, 1, 1);

    ir_src = gtk_label_new (info->srcFile);
    gtk_widget_set_halign (GTK_WIDGET (ir_src), GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (tableTxt), ir_src, 1, 6, 1, 1);

    // Tag
    label = gtk_label_new ("Tag");
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_start (GTK_WIDGET (label), 10);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 0, 7, 1, 1);

    gchar rank[3 * ROM_MAXRANK + 2 + 1];
    if (pref_getPreferred (rom_getItemName (item))) {
        g_utf8_strncpy (rank, "♥ ★★★★★", 2 + pref_getRank (rom_getItemName (item)));
    } else if (pref_getRank (rom_getItemName (item)) > 0) {
        g_utf8_strncpy (rank, "★★★★★", pref_getRank (rom_getItemName (item)));
    } else {
        g_utf8_strncpy (rank, "✗", 1);
    }

    label = gtk_label_new (rank);
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 1, 7, 1, 1);

    // played
    label = gtk_label_new ("Played");
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_start (GTK_WIDGET (label), 10);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 0, 8, 1, 1);

    gint nplay = pref_getNPlay (rom_getItemName (item));
    gchar *text;
    switch (nplay) {
    case 0:
        text = g_strdup_printf ("✗");
        break;
    case 1:
        text = g_strdup_printf ("%i time", nplay);
        break;
    default:
        text = g_strdup_printf ("%i times", nplay);
        break;
    }

    label = gtk_label_new (text);
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (tableTxt), label, 1, 8, 1, 1);
    g_free (text);

    gtk_grid_attach (GTK_GRID (table), tableTxt, 0, 1, 1, 1);

    gtk_grid_attach (GTK_GRID (table), cmbItem, 2, 0, 1, 1);
    gtk_widget_set_halign (GTK_WIDGET (cmbItem), GTK_ALIGN_START);
    gtk_widget_set_margin_end (GTK_WIDGET (cmbItem), 10);

    // da
    gtk_widget_set_size_request (da, 2.5 * cfg_keyInt ("TILE_SIZE_W"), 2.5 * cfg_keyInt ("TILE_SIZE_H"));
    gtk_grid_attach (GTK_GRID (table), da, 2, 1, 1, 1);
    gtk_widget_set_hexpand (da, TRUE);
    gtk_widget_set_vexpand (da, TRUE);
    gtk_widget_set_margin_end (GTK_WIDGET (da), 10);

    gtk_widget_show_all (dialog);

    // start the video
    vlc_playVideo (rom_getItemName (item), GTK_WIDGET (da));

    gtk_dialog_run (GTK_DIALOG (dialog));

    // stop
    vlc_stopVideo ();

    gtk_widget_destroy (dialog);
    mame_freeInfoRom (info);
    dialog = NULL;
}
