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
#include "app.h"
#include "blacklist.h"
#include "view.h"
#include "rom.h"
#include "mame.h"
#include "ui.h"
#include "config.h"
#include "pref.h"
#include "util.h"
#include "www.h"


#define TILE_EXTENSION_PNG  "png"
#define TILE_EXTENSION_JPG  "jpg"

// pixbuf
gchar *rom_tilePath = NULL;

GdkPixbuf *rom_tileNoImage = NULL;
GdkPixbuf *rom_tileNowShowing = NULL;
GdkPixbuf *rom_tileLoading = NULL;
GdkPixbuf *rom_tileFavorite = NULL;
GdkPixbuf *rom_tileRank = NULL;

// rom (all, NO BIOS)
GList *rom_romList = NULL;

static guint rom_count = 0;     // nTot rom + clone (NO BLACKLISTED)
static guint rom_available = 0; // nTot rom available (NO CLONE) (NO BLAKCLISTED)

// clone (only clone, NO BLACKLISTED)
GHashTable* rom_cloneTable = NULL;

GHashTable* rom_parentTableSearch = NULL;  // for searching name romname key=PARENT, data=description ronmname
GHashTable* rom_parentTable       = NULL;  // key=PARENT, data=description (ronmname)

static enum rom_sortOrder rom_sortOrder = ROM_SORT_AZ;

inline static int
rom_sortAZ (struct rom_romItem *itemA, struct rom_romItem *itemB)
{
    gchar* romA = g_utf8_casefold ((gchar*) itemA->desc, -1);
    gchar* romB = g_utf8_casefold ((gchar*) itemB->desc, -1);

    gint result = g_utf8_collate (romA, romB);

    g_free (romA);
    g_free (romB);

    return result;
}

inline static int
rom_sortZA (struct rom_romItem *itemA, struct rom_romItem *itemB)
{
    gchar *romA = g_utf8_casefold ((gchar*) itemA->desc, -1);
    gchar *romB = g_utf8_casefold ((gchar*) itemB->desc, -1);

    gint result = g_utf8_collate (romA, romB);

    g_free (romA);
    g_free (romB);

    return -result;

}

enum rom_sortOrder
rom_getSort (void)
{
    return rom_sortOrder;
}

void
rom_setSort (enum rom_sortOrder order)
{
    rom_sortOrder = order;

    g_print ("sorting romlist... ");
    switch (order) {
    case ROM_SORT_AZ:
        rom_romList = g_list_sort (rom_romList, (GCompareFunc) rom_sortAZ);
        break;
    case ROM_SORT_ZA:
        rom_romList = g_list_sort (rom_romList, (GCompareFunc) rom_sortZA);
        break;
    default:
        rom_romList = g_list_sort (rom_romList, (GCompareFunc) rom_sortAZ);
        break;
    }
    g_print (SUCCESS_MSG "\n");

}

void
rom_listFree (struct rom_romItem *item)
{
    if (item->name) g_free (item->name);
    if (item->description) g_free (item->description);
    if (item->desc) g_free (item->desc);
    if (item->tile) g_object_unref (item->tile);

    item->name = NULL;
    item->description = NULL;
    item->desc = NULL;
    item->tileLoaded = FALSE;
    item->tileLoading = FALSE;
    item->tile = NULL;
    item->rank = 0;
    item->pref = FALSE;
    item->nplay = 0;
    item->romFound = FALSE;

    g_free (item);
    item = NULL;
}

void
rom_init (void)
{
    /* pixbuf */
    rom_tilePath = g_strdup (cfg_keyStr ("TILE_PATH"));

    g_assert (!rom_tileNoImage);
    rom_tileNoImage = gdk_pixbuf_new_from_file_at_size (APP_RESOURCE APP_NOIMAGE, ui_tileSize_W, ui_tileSize_H, NULL);
    g_assert (rom_tileNoImage);

    g_assert (!rom_tileNowShowing);
    rom_tileNowShowing = gdk_pixbuf_new_from_file_at_size (APP_RESOURCE APP_NOWSHOWING, ui_tileSize_W, ui_tileSize_H, NULL);
    g_assert (rom_tileNowShowing);

    g_assert (!rom_tileLoading);
    rom_tileLoading = gdk_pixbuf_new_from_file_at_size (APP_RESOURCE APP_LOADING, ui_tileSize_W, ui_tileSize_H, NULL);
    g_assert (rom_tileLoading);

    g_assert (!rom_tileFavorite);
    rom_tileFavorite = gdk_pixbuf_new_from_file (APP_RESOURCE APP_PREFERRED, NULL);
    g_assert (rom_tileFavorite);

    g_assert (!rom_tileRank);
    rom_tileRank = gdk_pixbuf_new_from_file (APP_RESOURCE APP_RANK, NULL);
    g_assert (rom_tileRank);

    rom_count = 0;
    g_assert (!rom_romList);

    g_assert (!rom_cloneTable);
    rom_cloneTable = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_assert (rom_cloneTable);

    g_assert (!rom_parentTableSearch);
    rom_parentTableSearch = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_assert (rom_parentTableSearch);

    g_assert (!rom_parentTable);
    rom_parentTable = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_assert (rom_parentTable);

//    g_print ("rom download %s\n", cfg_keyBool ("ROM_DOWNLOAD") ? SUCCESS_MSG : FAIL_MSG);
//    g_print ("chd download %s\n", cfg_keyBool ("CHD_DOWNLOAD") ? SUCCESS_MSG : FAIL_MSG);

    blist_init ();
 }

void
rom_free (void)
{
    rom_count = 0;
    rom_available = 0;

    g_list_foreach (rom_romList, (GFunc) rom_listFree, NULL);
    g_list_free (rom_romList);
    rom_romList = NULL;

    /* free pixbuffer */
    g_object_unref (rom_tileNoImage);
    g_object_unref (rom_tileNowShowing);
    g_object_unref (rom_tileLoading);
    g_object_unref (rom_tileFavorite);
    g_object_unref (rom_tileRank);

    rom_tileNoImage = NULL;
    rom_tileNowShowing = NULL;
    rom_tileLoading = NULL;
    rom_tileFavorite = NULL;
    rom_tileRank = NULL;

    g_free (rom_tilePath);
    rom_tilePath = NULL;

    g_hash_table_destroy (rom_cloneTable);
    rom_cloneTable = NULL;

    g_hash_table_destroy (rom_parentTableSearch);
    rom_parentTableSearch = NULL;

    g_hash_table_destroy (rom_parentTable);
    rom_parentTable = NULL;

    blist_free();
 }

 void
 rom_load (void)
 {
    mame_gameList ();
    if ((rom_count <= 0) || (rom_available <=0)) {
        ui_showInfobar ();
    }
 }

inline gboolean
rom_isClone (const gchar *romName)
{
    return g_hash_table_contains (rom_cloneTable, romName);
}

inline gboolean
rom_isParent (const gchar *romName)
{
    return !(g_hash_table_contains (rom_cloneTable, romName));
}

inline const gchar*
rom_parentOf (const gchar *romName)
{
    return g_hash_table_lookup (rom_cloneTable, romName);
}


struct rom_romItem*
rom_newItem (void)
{
    struct rom_romItem* item = g_new (struct rom_romItem, 1);

    item->name = NULL;
    item->description = NULL;
    item->desc = NULL;
    item->tileLoaded = FALSE;
    item->tileLoading = FALSE;
    item->tile = NULL;
    item->rank = 0;
    item->pref = FALSE;
    item->nplay = 0;
    item->romFound = FALSE;

    rom_romList = g_list_prepend (rom_romList, item);
    rom_count++;

    return item;
}


struct rom_romItem*
rom_getItem (int numGame)
{
    g_assert (numGame < rom_count);
    return g_list_nth_data (rom_romList, numGame);
}

void
rom_setItemRomFound (struct rom_romItem* item, gboolean value)
{
    if (value) {
        if (!item->romFound) rom_available++;
    } else {
        if (item->romFound) rom_available--;
    }
    item->romFound = value;
}

inline const gboolean
rom_getItemRomFound (struct rom_romItem* item)
{
    return (item->romFound);
}

inline void
rom_setItemName (struct rom_romItem* item, gchar* name)
{
    item->name = g_strdup (name);
}

inline void
rom_setItemDescription (struct rom_romItem* item, gchar* description)
{
    // for searching
    item->description = g_strdup (description);

    // for sorting/view
    if (cfg_keyBool ("TILE_SHORT_DESCRIPTION")) {

        // hide " (" information
        gchar **romdesv = g_strsplit (description, " (", 2);

        if (cfg_keyBool ("TILE_SHORT_DESCRIPTION_HIDE_PREFIX")) {

            if (g_str_has_prefix (romdesv[0], "The ")) {
                // hide the prefix "The "
                item->desc = g_strdup_printf ("%s, The", romdesv[0] + 4);
            } else if (g_str_has_prefix (romdesv[0], "Vs. ")) {
                // hide the prefix "Vs. "
                item->desc = g_strdup_printf ("%s, Vs.", romdesv[0] + 4);
            } else if (g_str_has_prefix (romdesv[0], "VS ")) {
                // hide the prefix "VS "
                item->desc = g_strdup_printf ("%s, VS", romdesv[0] + 3);
            } else if (g_str_has_prefix (romdesv[0], "'")) {
                // hide the prefix "'88"
                item->desc = g_strdup_printf ("%s", romdesv[0] + 1);
            } else {
                item->desc = g_strdup (romdesv[0]);
            }

        } else {
            item->desc = g_strdup (romdesv[0]);
        }
        g_strfreev (romdesv);
    } else {
        item->desc = g_strdup (description);
    }
}

inline const gchar*
rom_getItemName (struct rom_romItem* item)
{
    return item->name;
}

inline const gchar*
rom_getItemDescription (struct rom_romItem* item)
{
    return item->description;
}

inline const gchar*
rom_getItemDesc (struct rom_romItem* item)
{
    return item->desc;
}

inline const GdkPixbuf*
rom_getItemTile (struct rom_romItem* item)
{
    return item->tile;
}

inline void
rom_setItemTile (struct rom_romItem* item, GdkPixbuf* tile)
{
    item->tile = tile;
}

inline const gboolean
rom_getItemTileLoaded (struct rom_romItem* item)
{
    return item->tileLoaded;
}

inline const gboolean
rom_getItemTileLoading (struct rom_romItem* item)
{
    return item->tileLoading;
}


/* pixbuf async:start */
static void
rom_closeStream_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    g_input_stream_close_finish (G_INPUT_STREAM (source_object), res, NULL);
    g_object_unref (G_INPUT_STREAM (source_object));
}

static void
rom_pixbufRead_cb (GObject *source_object, GAsyncResult *res, struct rom_romItem *item)
{

    GInputStream *stream = G_INPUT_STREAM (source_object);
    GError *error = NULL;

    item->tile = gdk_pixbuf_new_from_stream_finish (res, &error);

    g_input_stream_close_async (stream, G_PRIORITY_HIGH, NULL, (GAsyncReadyCallback) rom_closeStream_cb, NULL);


    if (error) {
        g_warning ("pixbuf stream error:%s\n", error->message);
        g_error_free (error);
        item->tile = NULL;
    }

    item->tileLoaded = TRUE;
    item->tileLoading = FALSE;

    if (ui_tileIsVisible (item)) {
        ui_invalidateDrawingArea ();
    }
}

static void
rom_fileRead_cb (GFile *file, GAsyncResult *res, struct rom_romItem *item)
{
    GError *error = NULL;
    gboolean keepratio = cfg_keyBool ("TILE_KEEP_ASPECT_RATIO");
    GFileInputStream *input = g_file_read_finish (file, res, &error);

    g_object_unref (file);
    file = NULL;

    if (!error) {
        gdk_pixbuf_new_from_stream_at_scale_async ((GInputStream*) input, ui_tileSize_W, ui_tileSize_H, keepratio, NULL, (GAsyncReadyCallback) rom_pixbufRead_cb, item);
    } else {
        g_warning ("input stream error:%s\n", error->message);

        g_error_free (error);

        item->tile = NULL;
        item->tileLoaded = TRUE;
        item->tileLoading = FALSE;

        if (ui_tileIsVisible (item)) {
            ui_invalidateDrawingArea ();

        }
    }
}

static void
rom_fileReadNoScale_cb (GFile *file, GAsyncResult *res, struct rom_romItem *item)
{
    GError *error = NULL;
    GFileInputStream *input = g_file_read_finish (file, res, &error);

    g_object_unref (file);
    file = NULL;

    if (!error) {
        gdk_pixbuf_new_from_stream_async ((GInputStream*) input, NULL, (GAsyncReadyCallback) rom_pixbufRead_cb, item);
    } else {
        g_warning ("input stream error:%s\n", error->message);

        g_error_free (error);

        item->tile = NULL;
        item->tileLoaded = TRUE;
        item->tileLoading = FALSE;

        if (ui_tileIsVisible (item)) {
            ui_invalidateDrawingArea ();
        }
    }
}

void
rom_loadItemAsync (struct rom_romItem* item)
{
    g_assert (item);
    g_assert (!item->tileLoading);
    g_assert (!item->tileLoaded);
    g_assert (!item->tile);

    item->tileLoading = TRUE;

    const gchar *romName = rom_getItemName (item);

    g_assert (g_file_test (rom_tilePath, G_FILE_TEST_IS_DIR));

    gchar *fileNamePng = g_strdup_printf ("%s/%s.%s", rom_tilePath, romName, TILE_EXTENSION_PNG);
    gchar *fileNameJpg = g_strdup_printf ("%s/%s.%s", rom_tilePath, romName, TILE_EXTENSION_JPG);

    // web downloaded (png)
    gchar *fileNameWWW = www_getFileNameWWW (romName);

    GFile *tileFile = NULL;
    if (g_file_test (fileNamePng, G_FILE_TEST_EXISTS)) {
        tileFile = g_file_new_for_path (fileNamePng);
        g_file_read_async (tileFile , G_PRIORITY_HIGH, FALSE, (GAsyncReadyCallback) rom_fileRead_cb, item);
    } else if (g_file_test (fileNameJpg, G_FILE_TEST_EXISTS)) {
        tileFile = g_file_new_for_path (fileNameJpg);
        g_file_read_async (tileFile , G_PRIORITY_HIGH, FALSE, (GAsyncReadyCallback) rom_fileRead_cb, item);
    } else if (g_file_test (fileNameWWW, G_FILE_TEST_EXISTS)) {
        tileFile  = g_file_new_for_path (fileNameWWW);
        g_file_read_async (tileFile , G_PRIORITY_HIGH, FALSE, (GAsyncReadyCallback) rom_fileReadNoScale_cb, item);
    } else {
        if (www_autoDownload) {
            www_download (item);
        } else {
            g_warning ("Tile not found: %s %s\n", romName, FAIL_MSG);
            item->tileLoaded = TRUE;
            item->tile = NULL;
        }
    }

    g_free (fileNamePng);
    g_free (fileNameJpg);
    g_free (fileNameWWW);
}
/* pixbuf async:end */


void
rom_invalidateUselessTile (void)
{
    for (GList *l = rom_romList; l != NULL; l = l->next) {
        struct rom_romItem *item = l->data;
        if (item->tileLoaded) {
            if (!item->tileLoading) {
                if (!ui_tileIsVisible (item)) {
                    // unloading all loaded tile

                    item->tileLoaded = FALSE;
                    item->tileLoading = FALSE;

                    if (item->tile) g_object_unref (item->tile);
                    item->tile = NULL;
                }
            }
        }
    }
}


gint
rom_search (GList* viewModel, gint focus, const gchar* romDes, gboolean forward)
{
    gchar *searchItm = g_utf8_strup (romDes, -1);
    gchar *search;

    if (forward) {
        focus++;
        // forward
        // focus to end
        for (GList *l = g_list_nth (viewModel, focus); l != NULL; l = l->next) {
            struct rom_romItem *item = l->data;

            // description
            search = g_utf8_strup (item->description, -1);
            if (g_strrstr (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);

            // romname
            search = g_utf8_strup (item->name, -1);
            if (g_strrstr (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);

            // clone (romnane + description)
            if (rom_isParent (item->name)) {
                if (g_hash_table_contains (rom_parentTableSearch, item->name)) {
                    search = g_hash_table_lookup (rom_parentTableSearch, item->name);
                    if (g_strrstr (search, searchItm)) {
                        g_free (searchItm);
                        return g_list_position ((GList*) viewModel, l);
                    }
                }
            }

        }

        // start to focus
        for (GList *l = g_list_first (viewModel); l != g_list_nth (viewModel, focus); l = l->next) {
            struct rom_romItem *item = l->data;

            // description
            search = g_utf8_strup (item->description, -1);
            if (g_strrstr (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);

            // romname
            search = g_utf8_strup (item->name, -1);
            if (g_strrstr (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);

            // clone (romnane + description)
            if (rom_isParent (item->name)) {
                if (g_hash_table_contains (rom_parentTableSearch, item->name)) {
                    search = g_hash_table_lookup (rom_parentTableSearch, item->name);
                    if (g_strrstr (search, searchItm)) {
                        g_free (searchItm);
                        return g_list_position ((GList*) viewModel, l);
                    }
                }
            }

        }
    } else {
        focus = posval (--focus);
        // backward
        // focus to start
        for (GList *l = g_list_nth (viewModel, focus); l != NULL; l = l->prev) {
            struct rom_romItem *item = l->data;

            // description
            search = g_utf8_strup (item->description, -1);
            if (g_strrstr (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);

            // name
            search = g_utf8_strup (item->name, -1);
            if (g_strrstr (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);

            // clone (romnane + description)
            if (rom_isParent (item->name)) {
                if (g_hash_table_contains (rom_parentTableSearch, item->name)) {
                    search = g_hash_table_lookup (rom_parentTableSearch, item->name);
                    if (g_strrstr (search, searchItm)) {
                        g_free (searchItm);
                        return g_list_position ((GList*) viewModel, l);
                    }
                }
            }
        }

        // end to focus
        for (GList *l = g_list_last (viewModel); l != g_list_nth (viewModel, focus); l = l->prev) {
            struct rom_romItem *item = l->data;

            // description
            search = g_utf8_strup (item->description, -1);
            if (g_strrstr (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);

            // name
            search = g_utf8_strup (item->name, -1);
            if (g_strrstr (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);

            // clone (romnane + description)
            if (rom_isParent (item->name)) {
                if (g_hash_table_contains (rom_parentTableSearch, item->name)) {
                    search = g_hash_table_lookup (rom_parentTableSearch, item->name);
                    if (g_strrstr (search, searchItm)) {
                        g_free (searchItm);
                        return g_list_position ((GList*) viewModel, l);
                    }
                }
            }

        }
    }

    g_free (searchItm);
    return -1;
}

gint
rom_search_letter (GList* viewModel, gint focus, const gchar* romStartWithLetter, gboolean forward)
{
    gchar *searchItm = g_utf8_strup (romStartWithLetter, -1);

    if (forward) {
        focus++;
        // focus to end
        for (GList *l = g_list_nth (viewModel, focus); l != NULL; l = l->next) {
            struct rom_romItem *item = l->data;
            gchar *search = g_utf8_strup (item->desc, -1);
            if (g_str_has_prefix (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);
        }

        // start to focus
        for (GList *l = g_list_first (viewModel); l != g_list_nth (viewModel, focus); l = l->next) {
            struct rom_romItem *item = l->data;
            gchar *search = g_utf8_strup (item->desc, -1);
            if (g_str_has_prefix (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);
        }
    } else {
        focus = posval (--focus);
        // focus to start
        for (GList *l = g_list_nth (viewModel, focus); l != NULL; l = l->prev) {
            struct rom_romItem *item = l->data;
            gchar *search = g_utf8_strup (item->desc, -1);
            if (g_str_has_prefix (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);
        }

        // end to focus
        for (GList *l = g_list_last (viewModel); l != g_list_nth (viewModel, focus); l = l->prev) {
            struct rom_romItem *item = l->data;
            gchar *search = g_utf8_strup (item->desc, -1);
            if (g_str_has_prefix (search, searchItm)) {
                g_free (searchItm);
                g_free (search);
                return g_list_position ((GList*) viewModel, l);
            }
            g_free (search);
        }
    }

    g_free (searchItm);
    return -1;
}

inline gboolean
rom_getItemPref (const struct rom_romItem *item)
{
    return item->pref;
}

inline void
rom_setItemPref (struct rom_romItem *item, gboolean value)
{
    item->pref = value;
}

inline guint
rom_getItemRank (const struct rom_romItem *item)
{
    return item->rank;
}

inline void
rom_setItemRank (struct rom_romItem *item, guint rank)
{
    item->rank = posval (lim (rank, ROM_MAXRANK));
}

inline guint
rom_getItemNPlay (const struct rom_romItem *item)
{
    return item->nplay;
}

inline void
rom_setItemNPlay (struct rom_romItem *item, guint nplay)
{
    item->nplay = nplay;
}

gboolean
rom_FoundInPath (const gchar* romName, ...)
{
    gboolean foundRom = FALSE;

    gchar *romNameZip;
    gchar *romName7Zip;
    gchar *romNameDir;

    va_list vl;
    va_start (vl, romName);

    gchar *romPath = va_arg (vl, gchar*);

    while (romPath && !foundRom) {
        romNameZip = g_strdup_printf ("%s/%s.%s", romPath, romName, ROM_EXTENSION_ZIP);
        romName7Zip = g_strdup_printf ("%s/%s.%s", romPath, romName, ROM_EXTENSION_7ZIP);
        romNameDir = g_strdup_printf ("%s/%s", romPath, romName);

        if (g_file_test (romName7Zip, G_FILE_TEST_EXISTS)) {
            foundRom = TRUE;
        } else if (g_file_test (romNameZip, G_FILE_TEST_EXISTS)) {
            foundRom = TRUE;
        } else if (g_file_test (romNameDir, G_FILE_TEST_IS_DIR)) {
            foundRom = TRUE;
        } else {
            //g_print ("\nrom not found: %s " FAIL_MSG "\n", romName);
        }

        romPath = va_arg (vl, gchar*);

        g_free (romNameZip);
        g_free (romName7Zip);
        g_free (romNameDir);
    }

    va_end (vl);

    return foundRom;
}
