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

#include <gtk/gtk.h>
#include <string.h>
#include <glib/gstdio.h>

#include "global.h"
#include "config.h"
#include "rom.h"
#include "filedownloader.h"

// NOTE: no CHD will be downloaded
static const gchar* ROM_BASEURL = "https://archive.org/download/MAME_0.151_ROMs/MAME_0.151_ROMs.zip/MAME 0.151 ROMs";
static gchar *fd_romPath = NULL;

// forward
static void fd_infoFree (struct fd_copyInfo* copyInfo);

void
fd_init (void)
{
	fd_downloadingItm = 0;
	if (cfg_keyBool ("ROM_DOWNLOAD")) {
    	g_print ("rom download " SUCCESS_MSG "\n");
    	g_print ("rom provider %s\n", ROM_BASEURL);

    	fd_romPath = g_strdup_printf ("%s/roms", cfg_keyStr ("WEB_PATH"));

	    if (g_mkdir_with_parents (fd_romPath, 0700) != 0) {
	    	g_print ("can't create %s " FAIL_MSG "\n", fd_romPath);
	    }
	}
}

void
fd_free (void)
{
	fd_downloadingItm = 0;
	if (cfg_keyBool ("ROM_DOWNLOAD")) {
		g_free (fd_romPath);
		fd_romPath = NULL;
	}
}

// FIXME
/*
static void
fd_progress_cb (void)
{
	g_print(".\n");
}
*/

static void
fd_copyDone_cb (GObject *file, GAsyncResult *res, struct fd_copyInfo *user_data)
{
	GError *err = NULL;

	GStatBuf stats;
	memset (&stats, 0, sizeof (stats));

	struct fd_copyInfo *copyInfo = user_data;

    if (g_file_copy_finish (G_FILE (file), res, &err)) {
		if (g_stat (copyInfo->oFileName, &stats) == 0) {
			if (stats.st_size != 0) {
				// seems legit
				g_print ("download of %s completed %s\n", copyInfo->iFileName, SUCCESS_MSG);
			} else {
				// file downloaed is invalid
				g_print ("download of %s failed %s\n", copyInfo->iFileName, FAIL_MSG);
			    if (!g_file_delete (copyInfo->oFile, NULL, &err)) {
	    			if (err) {
		        		g_print ("can't delete %s: %s %s\n", copyInfo->oFileName, err->message, FAIL_MSG);
			        	g_error_free (err);
			        }
			    } else {
    				g_print ("delete invalid file %s %s\n", copyInfo->oFileName, SUCCESS_MSG);
			    }
			}
		} else {
			// stat failed
			g_print ("download of %s failed %s\n", copyInfo->iFileName, FAIL_MSG);
	        g_print ("stat failed on %s %s\n", copyInfo->oFileName, FAIL_MSG);
		}

	} else {
		g_print ("download of %s failed %s\n", copyInfo->iFileName, FAIL_MSG);
        g_print ("error: %s %s\n", err->message, FAIL_MSG);
        g_error_free (err);
    }

	fd_infoFree (copyInfo);

    fd_downloadingItm--;
}

static void
fd_infoBuild (const gchar* romname, struct fd_copyInfo* copyInfo)
{
	// source URL
	copyInfo->iFileName = g_strdup_printf ("%s/%s.zip", ROM_BASEURL, romname);
    copyInfo->iFile = g_file_new_for_uri (copyInfo->iFileName);

    // dest FILE
	copyInfo->oFileName = g_strdup_printf ("%s/%s.zip", fd_romPath, romname);
	copyInfo->oFile = g_file_new_for_path (copyInfo->oFileName);
}

static void
fd_infoFree (struct fd_copyInfo* copyInfo)
{
    g_object_unref (copyInfo->iFile);
	g_object_unref (copyInfo->oFile);

	g_free (copyInfo->iFileName);
    g_free (copyInfo->oFileName);

    g_free (copyInfo);

    copyInfo = NULL;
}

void
fd_download (const gchar* romname)
{
	fd_downloadingItm++;
    struct fd_copyInfo *copyInfo = g_malloc0 (sizeof (struct fd_copyInfo));

	fd_infoBuild (romname, copyInfo);

	g_print ("downloading %s from %s\n", romname, copyInfo->iFileName);

	// FIXME
	//g_file_copy_async (copyInfo->iFile, copyInfo->oFile, G_FILE_COPY_OVERWRITE, G_PRIORITY_HIGH, NULL, (GFileProgressCallback) fd_progress_cb, NULL, (GAsyncReadyCallback) fd_copyDone_cb , copyInfo);
	g_file_copy_async (copyInfo->iFile, copyInfo->oFile, G_FILE_COPY_OVERWRITE, G_PRIORITY_HIGH, NULL, NULL, NULL, (GAsyncReadyCallback) fd_copyDone_cb , copyInfo);

}

const gchar*
fd_getDownloadPath (void)
{
	return fd_romPath;
}

