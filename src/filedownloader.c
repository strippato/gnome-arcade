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
#include <archive.h>
#include <archive_entry.h>

#include "global.h"
#include "config.h"
#include "rom.h"
#include "view.h"
#include "ui.h"
#include "filedownloader.h"

// 193 romset
//static const gchar* ROM_BASEURL = "https://archive.org/download/MAME_0.193_ROMs_split/MAME_0.193_ROMs_split.zip/MAME 0.193 ROMs (split)";
// 209 romset
//static const gchar* ROM_BASEURL = "https://archive.org/download/MAME_0.209_ROMs_merged/MAME_0.209_ROMs_merged.zip";
// 217 romset
//static const gchar* ROM_BASEURL = "https://archive.org/download/mame0217_nochd";
// 217 is now disabled
// fallback to 209 romset
static const gchar* ROM_BASEURL = "https://archive.org/download/MAME_0.209_ROMs_merged/MAME_0.209_ROMs_merged.zip";



// 193 CHD is now blocked, see https://archive.org/download/MAME_0.193_CHDs_merged/a51site4/
// 185 CHD is now blocked
//#define CHD_NAME_A_G "MAME 0.185 CHDs (merged) (a-g)"
//#define CHD_NAME_H_Z "MAME 0.185 CHDs (merged) (h-z)"
//static const gchar *CHD_FILENAME_A_G = "https://archive.org/download/MAME_0.185_CHDs_Merged/MAME 0.185 CHDs (merged) (a-g).zip/";
//static const gchar *CHD_FILENAME_H_Z = "https://archive.org/download/MAME_0.185_CHDs_Merged/MAME 0.185 CHDs (merged) (h-z).zip/";
// 215 CHD
static const gchar *CHD_FILENAME = "https://archive.org/download/mame0215_chd";

static gchar *fd_romPath = NULL;
static gchar *fd_chdPath = NULL;

// forward
static void fd_infoFree (struct fd_copyInfo* copyInfo);
static void fd_decompress7z (const char *filename7z, const char *outdir);


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
fd_copyCHDDone_cb (GObject *file, GAsyncResult *res, struct fd_copyInfo *user_data)
{
	GError *err = NULL;

	GStatBuf stats;
	memset (&stats, 0, sizeof (stats));

	struct fd_copyInfo *copyInfo = user_data;

    if (g_file_copy_finish (G_FILE (file), res, &err)) {
		if (g_stat (copyInfo->oFileName, &stats) == 0) {
			if (stats.st_size != 0) {
				// seems legit
				g_print ("download of %s (chd) completed %s\n", copyInfo->iFileName, SUCCESS_MSG);
			} else {
				// file downloaed is invalid
				g_print ("download of %s (chd) failed %s\n", copyInfo->iFileName, FAIL_MSG);
			    if (!g_file_delete (copyInfo->oFile, NULL, &err)) {
	    			if (err) {
		        		g_print ("can't delete (chd) %s: %s %s\n", copyInfo->oFileName, err->message, FAIL_MSG);
			        	g_error_free (err);
			        }
			    } else {
    				g_print ("delete invalid file (chd) %s %s\n", copyInfo->oFileName, SUCCESS_MSG);
			    }
			}
		} else {
			// stat failed
			g_print ("download of %s (chd) failed %s\n", copyInfo->iFileName, FAIL_MSG);
	        g_print ("stat failed on (chd) %s %s\n", copyInfo->oFileName, FAIL_MSG);
		}

	} else {
		g_print ("download of %s (chd) failed %s\n", copyInfo->iFileName, FAIL_MSG);
        g_print ("error (chd): %s %s\n", err->message, FAIL_MSG);
        g_error_free (err);
    }


	// decompress 7z
	
	GFile *decompressPath = g_file_get_parent (copyInfo->oFile); 
	gchar *outBasePath = g_file_get_path (decompressPath);       // /home/strippy/gnome-arcade/data/www/chd 

	gchar *outFullPath = g_strdup_printf ("%s/%s", outBasePath, copyInfo->romName);
	
	g_print ("CHD decompressing %s -> %s\n", copyInfo->oFileName, outFullPath);
	fd_decompress7z	(copyInfo->oFileName, outFullPath);
	g_print ("CHD done\n");
	
	g_free (outBasePath);
	g_free (outFullPath);

	g_object_unref (decompressPath);


	fd_infoFree (copyInfo);

    fd_downloadingItm--;

	if (fd_downloadingItm == 0) {
		ui_afterDownload ();
	}
}

static void
fd_infoBuildRom (const gchar* romName, struct fd_copyInfo* copyInfo)
{

	copyInfo->romName = g_strdup (romName);

	// source URL
	copyInfo->iFileName = g_strdup_printf ("%s/%s.%s", ROM_BASEURL, romName, ROM_EXTENSION_ZIP);
    copyInfo->iFile = g_file_new_for_uri (copyInfo->iFileName);

    // dest FILE
	copyInfo->oFileName = g_strdup_printf ("%s/%s.%s", fd_romPath, romName, ROM_EXTENSION_ZIP);
	copyInfo->oFile = g_file_new_for_path (copyInfo->oFileName);
}

static void
fd_infoBuildChd (const gchar* romName, const gchar* srcRom, const gchar* destRom, struct fd_copyInfo* copyInfo)
{
	copyInfo->romName = g_strdup (romName);

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

    g_free (copyInfo->romName);

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
	g_file_copy_async (copyInfo->iFile, copyInfo->oFile, G_FILE_COPY_OVERWRITE, G_PRIORITY_HIGH, NULL, NULL, NULL, (GAsyncReadyCallback) fd_copyCHDDone_cb , copyInfo);
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

		g_print ("downloading CHD %s from in %s\n", romName, CHD_FILENAME);

		// path building
		gchar *srcRom  = g_strdup_printf ("%s/%s.7z", CHD_FILENAME, romName);
		gchar *destRom = g_strdup_printf ("%s/%s.7z", fd_chdPath, romName);

		//g_print ("src for CHD in %s\n", srcRom);
		//g_print ("dst for CHD in %s\n", destRom);

		// FIXME: don't download all set of CHD
		// start async downloading
		fd_downloadChd (romName, srcRom, destRom);

		g_free (srcRom);
		g_free (destRom);

 		break;

	}


}


static int
copy_data (struct archive *ar, struct archive *aw)
{
	const void *buff;
	size_t size;
	la_int64_t offset;

	for (;;) {
		int r = archive_read_data_block (ar, &buff, &size, &offset);
		if (r == ARCHIVE_EOF) return ARCHIVE_OK;
		if (r < ARCHIVE_OK)   return r;
		
		r = archive_write_data_block (aw, buff, size, offset);
		
		if (r < ARCHIVE_OK) {
			g_print ("%s\n", archive_error_string (aw));
			return r;
		}
	}
}

static void 
fd_decompress7z (const char *filename7z, const char *outpath)
{
	struct archive_entry *entry;

	int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_SECURE_SYMLINKS | ARCHIVE_EXTRACT_UNLINK | ARCHIVE_EXTRACT_SECURE_NODOTDOT;

	struct archive *a = archive_read_new ();
	archive_read_support_format_all (a);
	archive_read_support_filter_all (a);
	
	struct archive *ext = archive_write_disk_new ();
	archive_write_disk_set_options (ext, flags);
	archive_write_disk_set_standard_lookup (ext);
	
	int r = archive_read_open_filename (a, filename7z, 10240);
	
	if (r != ARCHIVE_OK) {
    	g_print ("can't decompress %s " FAIL_MSG "\n", filename7z);
		goto QUIT;
	}
	
	for (;;) {
		r = archive_read_next_header (a, &entry);
		if (r == ARCHIVE_EOF) break;

		if (r < ARCHIVE_OK)  g_print ("%s\n", archive_error_string (a));
		if (r < ARCHIVE_WARN) goto QUIT;

		gchar *outname = g_strdup_printf ("%s/%s", outpath, archive_entry_pathname (entry));	

		archive_entry_set_pathname (entry, outname);
		g_free (outname);

		r = archive_write_header (ext, entry);

		if (r < ARCHIVE_OK) {
			g_print ("%s\n", archive_error_string (ext));
		} else if (archive_entry_size (entry) > 0) {
			r = copy_data (a, ext);
			if (r < ARCHIVE_OK) g_print("%s\n", archive_error_string (ext));
			if (r < ARCHIVE_WARN) goto QUIT;
		}

		r = archive_write_finish_entry (ext);

		if (r < ARCHIVE_OK) g_print ("%s\n", archive_error_string (ext));
		if (r < ARCHIVE_WARN) goto QUIT;
	}

QUIT:	
	archive_read_close (a);
	archive_read_free  (a);
	archive_write_close (ext);
	archive_write_free  (ext);
	

}

