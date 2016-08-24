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
#include "filedownloader.h"

// NOTE: no CHD will be downloaded
static const gchar* ROM_BASEURL = "https://archive.org/download/MAME_0.151_ROMs/MAME_0.151_ROMs.zip/MAME 0.151 ROMs";
static gchar *fd_romPath = NULL;

void
fd_init (void)
{
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
	if (cfg_keyBool ("ROM_DOWNLOAD")) {
		g_free (fd_romPath);
		fd_romPath = NULL;
	}
}

static void
fd_progress_cb (void)
{
	g_print(".");
}

gboolean
fd_download (const gchar* romname)
{
    GError *err  = NULL;
    gboolean res = FALSE;
	GStatBuf stats;

	gchar *finame = g_strdup_printf ("%s/%s.zip", ROM_BASEURL, romname);
	gchar *foname = g_strdup_printf ("%s/%s.zip", fd_romPath, romname);

	g_print ("downloading %s from %s ", romname, finame);

    GFile *ifile = g_file_new_for_uri (finame);
	GFile *ofile = g_file_new_for_path (foname);

	memset (&stats, 0, sizeof (stats));

	if (g_file_copy (ifile, ofile, G_FILE_COPY_OVERWRITE, NULL, (GFileProgressCallback) fd_progress_cb, NULL, &err)) {

		if (g_stat (foname, &stats) == 0) {
			if (stats.st_size != 0) {
				// seems legit
				g_print (" " SUCCESS_MSG "\n");

				res = TRUE;

			} else {
				// file downloaed is invalid
				g_print (" " FAIL_MSG "\n");
			    if (!g_file_delete (ofile, NULL, &err)) {
	    			if (err) {
		        		g_print ("can't delete %s: %s %s\n", foname, err->message, FAIL_MSG);
			        	g_error_free (err);
			        }
			    }
			}
		} else {
			// stat failed
			g_print (" " FAIL_MSG "\n");
	        g_print ("stat failed on %s %s\n", foname, FAIL_MSG);
		}

	} else {
		g_print (" " FAIL_MSG "\n");
	    if (err) {
	        g_print ("donwnload error: %s %s\n", err->message, FAIL_MSG);
	        g_error_free (err);
	    }
	}

	g_free (finame);
    g_free (foname);

    g_object_unref (ifile);
	g_object_unref (ofile);

    return res;
}

const gchar*
fd_getDownloadPath (void)
{
	return fd_romPath;
}

