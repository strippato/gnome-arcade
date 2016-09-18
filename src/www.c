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


#include "global.h"
#include "rom.h"
#include "view.h"
#include "ui.h"
#include "config.h"
#include "util.h"
#include "www.h"

#define WWW_EXTENSION_PNG  "png"

static const gchar* www_webProvider = NULL;

void
www_init (void)
{
    www_downloadingItm = 0;
    /*
    if (g_str_has_prefix (cfg_keyStr ("WEB_PATH"), "~")) {
        www_tilePath = g_strdup_printf ("%s%s/", g_get_home_dir (), cfg_keyStr ("WEB_PATH") + 1);
    } else {
        www_tilePath = g_strdup_printf ("%s/", cfg_keyStr ("WEB_PATH"));
    }
    */
    www_tilePath = g_strdup (cfg_keyStr ("WEB_PATH"));

    if (cfg_keyBool ("TILE_DOWNLOAD")) {
		www_autoDownload = TRUE;
    } else {
		www_autoDownload = FALSE;
    }
	g_print ("tile download %s\n", www_autoDownload ? SUCCESS_MSG : FAIL_MSG);

	www_webProvider = cfg_keyStr ("TILE_PROVIDER");
	if (www_autoDownload) {
		g_print ("tile provider %s\n", www_webProvider);
	}

    if (g_mkdir_with_parents (www_tilePath, 0700) != 0) {
    	g_print ("can't create %s " FAIL_MSG "\n", www_tilePath);
    }
}

void
www_free (void)
{
    g_assert (www_downloadingItm == 0);
	g_free (www_tilePath);
    www_tilePath = NULL;
    www_autoDownload = FALSE;
    www_webProvider = NULL;
}

inline static void
www_closeStream_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    g_input_stream_close_finish (G_INPUT_STREAM (source_object), res, NULL);
    g_object_unref (G_INPUT_STREAM (source_object));
}

static void
www_pixbufRead_cb (GObject *source_object, GAsyncResult *res, struct rom_romItem *item)
{
    GError *error = NULL;
    GInputStream *stream = G_INPUT_STREAM (source_object);
    item->tile = gdk_pixbuf_new_from_stream_finish (res, &error);

    g_input_stream_close_async (stream, G_PRIORITY_HIGH, NULL, (GAsyncReadyCallback) www_closeStream_cb, NULL);

	gchar* localName = www_getFileNameWWW (item->name);

    if (!error) {
    	g_print ("%s fetched %s \n", item->name, SUCCESS_MSG);
		if (gdk_pixbuf_save (item->tile, localName, "png", &error, NULL)) {
			g_print ("%s saved %s \n", item->name, SUCCESS_MSG);
		} else {
			g_print ("%s not saved %s \n", item->name, FAIL_MSG);
	        g_warning ("can't save: %s\n", error->message);
	        g_error_free (error);
		}
    } else {
        g_warning ("pixbuf stream error:%s\n", error->message);
        g_error_free (error);
        item->tile = NULL;
    }

    item->tileLoaded = TRUE;
    item->tileLoading = FALSE;

    g_free (localName);

    if (ui_tileIsVisible (item)) {
        ui_invalidateDrawingArea ();
    }
    www_downloadingItm--;
}

static void
www_fileRead_cb (GFile *file, GAsyncResult *res, struct rom_romItem *item)
{
    GError *error = NULL;
    gboolean keepratio = cfg_keyBool ("TILE_KEEP_ASPECT_RATIO");
    GFileInputStream *input = g_file_read_finish (file, res, &error);

    g_object_unref (file);
    file = NULL;

    if (!error) {
        gdk_pixbuf_new_from_stream_at_scale_async ((GInputStream*) input, ui_tileSize_W, ui_tileSize_H, keepratio, NULL, (GAsyncReadyCallback) www_pixbufRead_cb, item);
    } else {
        g_print ("stream error [%s]: %s " FAIL_MSG "\n", item->name, error->message);

        g_error_free (error);

        item->tile = NULL;
        item->tileLoaded = TRUE;
        item->tileLoading = FALSE;

        if (ui_tileIsVisible (item)) {
            ui_invalidateDrawingArea ();
        }
        www_downloadingItm--;
    }
}

void
www_download (struct rom_romItem* item)
{
    www_downloadingItm++;

	gchar *fileNameWeb = g_strdup_printf (www_webProvider, item->name);
	g_print ("fetching(%i) [%s] %s\n", www_downloadingItm, item->name, fileNameWeb);

	GFile *tileFile = g_file_new_for_uri (fileNameWeb);

    g_file_read_async (tileFile, G_PRIORITY_HIGH, FALSE, (GAsyncReadyCallback) www_fileRead_cb, item);

	g_free (fileNameWeb);
}

inline gchar*
www_getFileNameWWW (const gchar* romName)
{
	return g_strdup_printf ("%s/%s.%s", www_tilePath, romName, WWW_EXTENSION_PNG);
}
