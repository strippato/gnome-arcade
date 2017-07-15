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
#include <libgen.h>

#include "global.h"
#include "config.h"
#include "rom.h"
#include "view.h"
#include "ui.h"
#include "filedownloader.h"

// ROMSET 185
static const gchar* ROM_BASEURL = "https://archive.org/download/MAME_0.185_ROMs_split/MAME_0.185_ROMs_split.zip/MAME 0.185 ROMs (split)";

// CHD 185
#define CHD_AG_NAME "MAME 0.185 CHDs (merged) (a-g)"
#define CHD_HZ_NAME "MAME 0.185 CHDs (merged) (h-z)"

static const gchar *CHD_AG = "https://archive.org/download/MAME_0.185_CHDs_Merged/" CHD_AG_NAME ".zip/";
static const gchar *CHD_HZ = "https://archive.org/download/MAME_0.185_CHDs_Merged/" CHD_HZ_NAME ".zip/";

static gchar *fd_romPath = NULL;
static gchar *fd_chdPath = NULL;

// forward
static void fd_infoFree (struct fd_copyInfo* copyInfo);

void
fd_init (void)
{
	fd_downloadingItm = 0;

	if (cfg_keyBool ("ROM_DOWNLOAD")) {
    	g_print ("rom download " SUCCESS_MSG "\n");
    	g_print ("rom provider %s\n", ROM_BASEURL);
    	fd_romPath = g_strdup_printf ("%s/rom", cfg_keyStr ("WEB_PATH"));

	    if (g_mkdir_with_parents (fd_romPath, 0700) != 0) {
	    	g_print ("can't create %s " FAIL_MSG "\n", fd_romPath);
	    }
	}

	if (cfg_keyBool ("CHD_DOWNLOAD")) {
		g_print ("chd download " SUCCESS_MSG "\n");
    	fd_chdPath = g_strdup_printf ("%s/chd", cfg_keyStr ("WEB_PATH"));

	    if (g_mkdir_with_parents (fd_chdPath, 0700) != 0) {
	    	g_print ("can't create %s " FAIL_MSG "\n", fd_chdPath);
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

	if (cfg_keyBool ("CHD_DOWNLOAD")) {
		g_free (fd_chdPath);
		fd_chdPath = NULL;
	}
}


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

	if (fd_downloadingItm == 0) {
		ui_afterDownload ();
	}
}

static void
fd_infoBuildRom (const gchar* romName, struct fd_copyInfo* copyInfo)
{
	// source URL
	copyInfo->iFileName = g_strdup_printf ("%s/%s.zip", ROM_BASEURL, romName);
    copyInfo->iFile = g_file_new_for_uri (copyInfo->iFileName);

    // dest FILE
	copyInfo->oFileName = g_strdup_printf ("%s/%s.zip", fd_romPath, romName);
	copyInfo->oFile = g_file_new_for_path (copyInfo->oFileName);
}

static void
fd_infoBuildChd (const gchar* romName, const gchar* srcRom, const gchar* destRom, struct fd_copyInfo* copyInfo)
{
	// source URL
	copyInfo->iFileName = g_strdup (srcRom);
    copyInfo->iFile = g_file_new_for_uri (copyInfo->iFileName);

    // dest FILE
	copyInfo->oFileName = g_strdup (destRom);
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
fd_downloadRom (const gchar* romName)
{
	fd_downloadingItm++;

    struct fd_copyInfo *copyInfo = g_malloc0 (sizeof (struct fd_copyInfo));

	fd_infoBuildRom (romName, copyInfo);

	g_print ("downloading %s (rom) from %s\n", romName, copyInfo->iFileName);

	// FIXME
	//g_file_copy_async (copyInfo->iFile, copyInfo->oFile, G_FILE_COPY_OVERWRITE, G_PRIORITY_HIGH, NULL, (GFileProgressCallback) ui_progress_cb, NULL, (GAsyncReadyCallback) fd_copyDone_cb , copyInfo);
	g_file_copy_async (copyInfo->iFile, copyInfo->oFile, G_FILE_COPY_OVERWRITE, G_PRIORITY_HIGH, NULL, NULL, NULL, (GAsyncReadyCallback) fd_copyDone_cb , copyInfo);
}

static void
fd_downloadChd (const gchar* romName, const gchar* srcRom, const gchar* destRom)
{
	fd_downloadingItm++;

    struct fd_copyInfo *copyInfo = g_malloc0 (sizeof (struct fd_copyInfo));

	fd_infoBuildChd (romName, srcRom, destRom, copyInfo);

	g_print ("downloading %s (chd) from %s\n", romName, copyInfo->iFileName);

	// base dir must exist
	gchar *dlpath = g_strdup (destRom);
	gchar *dname  = dirname (dlpath);

	if (!g_file_test (dname, G_FILE_TEST_IS_DIR)) {
	    g_mkdir_with_parents (dname, 0700);
	}
	g_free (dlpath);

	// FIXME
	//g_file_copy_async (copyInfo->iFile, copyInfo->oFile, G_FILE_COPY_OVERWRITE, G_PRIORITY_HIGH, NULL, (GFileProgressCallback) ui_progress_cb, NULL, (GAsyncReadyCallback) fd_copyDone_cb , copyInfo);
	g_file_copy_async (copyInfo->iFile, copyInfo->oFile, G_FILE_COPY_OVERWRITE, G_PRIORITY_HIGH, NULL, NULL, NULL, (GAsyncReadyCallback) fd_copyDone_cb , copyInfo);
}

const gchar*
fd_getDownloadPathRom (void)
{
	return fd_romPath;
}

const gchar*
fd_getDownloadPathChd (void)
{
	return fd_chdPath;
}

void
fd_findAndDownloadChd (const gchar* romName)
{
// mame -listxml area51|grep "<disk name=\""
//	<disk name="area51" sha1="3b303bc37e206a6d733935

	const gchar* chdLink = NULL;
	const gchar* chdName = NULL;

	GError *error = NULL;
	gchar  *buf = NULL;

	switch (romName[0]) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
		chdLink = CHD_AG;
		chdName = CHD_AG_NAME;
		break;

	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
		chdLink = CHD_HZ;
		chdName = CHD_HZ_NAME;
		break;

	default:
		chdLink = NULL;
		chdName = NULL;
	}

	if (chdLink && chdName) {
		g_print ("searching for CHD in %s\n", chdLink);
		GFile *zipInfo = g_file_new_for_uri (chdLink);

	 	if (g_file_load_contents (zipInfo, NULL, &buf, 0, NULL, &error)) {
			gchar  *findMe = g_strdup_printf (">%s/%s/", chdName, romName);
			gchar **strvec = g_strsplit (buf , findMe, -1);

			unsigned int i = 0;
			for (gchar **ptr = strvec; *ptr; ptr++, i++) {
				if (i > 1) {
					// skip invalid
					// skip directory name

					// split for </a>
					gchar **strveca = g_strsplit (*ptr, "</a>", -1);

					g_print ("CHD found: %s\n", *strveca);

					// path building
					gchar *srcRom  = g_strdup_printf ("%s%s/%s/%s", chdLink, chdName, romName, *strveca);
					gchar *destRom = g_strdup_printf ("%s/%s/%s", fd_chdPath, romName, *strveca);

					// FIXME: don't download all set of CHD

					// start async downloading
					fd_downloadChd (romName, srcRom, destRom);

					g_free (srcRom);
					g_free (destRom);

					g_strfreev (strveca);
				}
			}

			g_free (findMe);
			g_strfreev (strvec);
	 	}

	 	if (error) {
	 		g_print ("CHD load error: %s\n", error->message);
	 		g_error_free (error);
	 		error = NULL;
	 	}

	 	g_free (buf);
	 	g_object_unref (zipInfo);
	}
}

