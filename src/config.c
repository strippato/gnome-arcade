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
#include "config.h"

#define CFG_FILENAME "gnome-arcade.ini"
#define CFG_SECTION  "arcade"

static GHashTable *cfg_default = NULL;
static GHashTable *cfg_config  = NULL;
static GError 	  *err         = NULL;

static void
cfg_fillDefaultConfig (void)
{

	g_hash_table_insert (cfg_default, g_strdup ("MAME_RELEASE"), g_strdup("0.0.0"));
	g_hash_table_insert (cfg_default, g_strdup ("MAME_EXE"), g_strdup (MAME_BIN));
	g_hash_table_insert (cfg_default, g_strdup ("MAME_OPTIONS"), g_strdup ("-autosave -skip_gameinfo -video opengl"));

	g_hash_table_insert (cfg_default, g_strdup ("USE_DARK_THEME"), g_strdup ("1"));

	g_hash_table_insert (cfg_default, g_strdup ("ROM_PATH"), g_strdup ("/usr/share/gnome-arcade/data/rom")); // /usr/share/gnome-arcade/data/rom
	g_hash_table_insert (cfg_default, g_strdup ("CHD_PATH"), g_strdup ("/usr/share/gnome-arcade/data/rom")); // /usr/share/gnome-arcade/data/chd
	g_hash_table_insert (cfg_default, g_strdup ("VIDEO_PATH"), g_strdup ("/usr/share/gnome-arcade/data/rom")); // /usr/share/gnome-arcade/data/video

	g_hash_table_insert (cfg_default, g_strdup ("TILE_SIZE_W"), g_strdup ("210"));
	g_hash_table_insert (cfg_default, g_strdup ("TILE_SIZE_H"), g_strdup ("210"));
	g_hash_table_insert (cfg_default, g_strdup ("TILE_KEEP_ASPECT_RATIO"), g_strdup ("1"));
	g_hash_table_insert (cfg_default, g_strdup ("TILE_TITLE_CENTERED"), g_strdup ("1"));
	g_hash_table_insert (cfg_default, g_strdup ("TILE_PATH"), g_strdup ("/usr/share/gnome-arcade/data/tile"));
	g_hash_table_insert (cfg_default, g_strdup ("TILE_BORDER_DYNAMIC"), g_strdup ("1"));
	g_hash_table_insert (cfg_default, g_strdup ("TILE_SHORT_DESCRIPTION"), g_strdup ("1"));
	g_hash_table_insert (cfg_default, g_strdup ("TILE_SHORT_DESCRIPTION_HIDE_PREFIX"), g_strdup ("1"));
	g_hash_table_insert (cfg_default, g_strdup ("TILE_SHADOW"), g_strdup ("1"));

	g_hash_table_insert (cfg_default, g_strdup ("TILE_PROVIDER"), g_strdup ("http://www.progettoemma.net/snap/%s/0000.png"));
	g_hash_table_insert (cfg_default, g_strdup ("WEB_PATH"), g_build_filename (g_get_home_dir (), "gnome-arcade/data/www", NULL));

	g_hash_table_insert (cfg_default, g_strdup ("TILE_DOWNLOAD"), g_strdup ("1"));
	g_hash_table_insert (cfg_default, g_strdup ("ROM_DOWNLOAD"), g_strdup ("1"));
	g_hash_table_insert (cfg_default, g_strdup ("CHD_DOWNLOAD"), g_strdup ("1"));
	g_hash_table_insert (cfg_default, g_strdup ("VIDEO_DOWNLOAD"), g_strdup ("1"));

	g_hash_table_insert (cfg_default, g_strdup ("JOY_ENABLED"), g_strdup ("1"));

	// SCREEN SAVER MODE:
	// 0 SYSTEM DEFAULT
	// 1 IN GAME DISABLED
	// 2 ALWAY DISABLED
	g_hash_table_insert (cfg_default, g_strdup ("SCREENSAVER_MODE"), g_strdup ("1"));

}

void
cfg_setConfig (const gchar* key, const gchar* data)
{
	g_hash_table_replace (cfg_config, g_strdup (key), g_strdup (data));
}

/*
static void
cfg_print (gchar *key, gchar *value)
{
	g_print ("Key:%s Value:%s\n", key, value);
}

static void
cfg_dump (void)
{
	g_print ("Dumping hashtable...\n");
	g_hash_table_foreach (cfg_default, (GHFunc) cfg_print, NULL);
	g_print ("Done!\n");
}
*/


void
cfg_init (void)
{
	g_assert (!cfg_config);
	g_assert (!cfg_default);

	err = NULL;
	cfg_default = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	cfg_config = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	cfg_fillDefaultConfig ();

	//cfg_dump();
}

void
cfg_free (void)
{
	g_hash_table_unref (cfg_default);
	g_hash_table_unref (cfg_config);

	cfg_default = NULL;
	cfg_config = NULL;
}

gboolean
cfg_createDefaultConfigFile (void)
{
	gboolean created = TRUE;
	gsize len;

	gchar *fileName = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, CFG_FILENAME, NULL);
	gchar *pathName = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, NULL);

	g_print ("writing config file (%s)\n", fileName);

	g_assert (fileName);
	g_assert (pathName);

	GKeyFile* keyFile = g_key_file_new ();

	/* adding default config */
	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init (&iter, cfg_default);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		g_key_file_set_string (keyFile, CFG_SECTION, (gchar*) key, (gchar*) value);
	}

	g_key_file_set_comment (keyFile, CFG_SECTION, "TILE_PROVIDER", "TILE_PROVIDER\n" \
																  	"http://www.progettoemma.net/snap/%s/0000.png\n" \
																  	"http://www.progettoemma.net/snap/%s/title.png\n" \
																  	"http://www.progettoemma.net/snap/%s/flyer.png\n" \
																  	"http://www.progettoemma.net/snap/%s/score.png\n" \
																  	"http://www.progettoemma.net/snap/%s/gameover.png\n" \
																  	"\n" \
																  	"http://mrdo.mameworld.info/mame_artwork/%s.png\n" \
																  	"\n" \
																  	"http://www.mamedb.com/snap/%s.png\n" \
																	"http://www.mamedb.com/titles/%s.png\n" \
																	"http://www.mamedb.com/cabinets/%s.png\n" \
																	"\n" \
																	"http://adb.arcadeitalia.net/media/mame.current/ingames/%s.png\n" \
																	"http://adb.arcadeitalia.net/media/mame.current/titles/%s.png\n" \
																	"http://adb.arcadeitalia.net/media/mame.current/cabinets/%s.png\n" \
							, &err);

	if (err) {
		g_print ("Can't write to output stream: %s\n", err->message);
	    g_error_free (err);
	    err = NULL;
	}

	gchar *data = g_key_file_to_data (keyFile, &len, &err);
	if (data) {
		if (!g_mkdir_with_parents (pathName, 0700)) {

		    GFile *file = g_file_new_for_path (fileName);

		    GFileOutputStream *outStream = g_file_replace (file, NULL, TRUE, G_FILE_CREATE_PRIVATE, NULL, &err);
		    if (outStream) {
		    	if (g_output_stream_write (G_OUTPUT_STREAM (outStream), data, strlen (data), NULL, &err) == -1) {
					g_print ("Can't write to output stream: %s\n", err->message);
				    g_error_free (err);
				    err = NULL;
				    created = FALSE;
		    	}

		    	g_output_stream_close (G_OUTPUT_STREAM (outStream), NULL, NULL);
		    	g_object_unref (outStream);

		    } else {
				g_print ("Can't create output stream: %s\n", err->message);
			    g_error_free (err);
			    err = NULL;
			    created = FALSE;
		    }
	    	g_object_unref (file);


		} else {
			g_print ("Can't create config directory\n");
			created = FALSE;
		}
		g_free (data);

	} else {
		g_print ("Can't convert data to string: %s\n", err->message);
	    g_error_free (err);
	    err = NULL;
	    created = FALSE;
	}

	g_key_file_free (keyFile);
	g_free (fileName);
	g_free (pathName);

	return created;
}

gboolean
cfg_load (void)
{
	gchar *file = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, CFG_FILENAME, NULL);
	g_assert (file);

	GKeyFile *cfg_keyFile = g_key_file_new ();

	g_print ("loading config from %s ", file);

	if (g_key_file_load_from_file (cfg_keyFile, file, G_KEY_FILE_KEEP_COMMENTS, &err)) {
		g_print (SUCCESS_MSG "\n");

		/* loop the default config */
		GHashTableIter iter;
		gpointer key, value;

		g_hash_table_iter_init (&iter, cfg_default);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			if (g_key_file_has_key (cfg_keyFile, CFG_SECTION, (gchar*) key, NULL)) {
				// found in config
				gchar *cfgValue = g_key_file_get_string (cfg_keyFile, CFG_SECTION, (gchar*) key, NULL);

				// check if mame path is valid
				if (g_strcmp0 (key, "MAME_EXE") == 0) {
					if (!g_file_test (cfgValue, G_FILE_TEST_IS_EXECUTABLE)) {
						gchar *filename = g_find_program_in_path ("mame");
						if (filename) {
							if (g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE)) {
								g_print ("Mame (%s) not found, using %s instead. Please, update your config " FAIL_MSG "\n", cfgValue, filename);
								g_free (cfgValue);
								cfgValue = filename;
							}
						}
					}
				}

				g_hash_table_insert (cfg_config, g_strdup ((gchar*) key), cfgValue);
			} else {
				// not found, let's use the default
				value= g_strdup ((gchar*) value);

				// check if mame path is valid
				if (g_strcmp0 (key, "MAME_EXE") == 0) {
					if (!g_file_test (value, G_FILE_TEST_IS_EXECUTABLE)) {
						gchar *filename = g_find_program_in_path ("mame");
						if (filename) {
							if (g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE)) {
								g_print ("Mame (%s) not found, using %s instead. Please, update your config " FAIL_MSG "\n", (gchar*) value, (gchar*) filename);
								g_free (value);
								value = filename;
							}
						}
					}
				}
				g_hash_table_insert (cfg_config, g_strdup ((gchar*) key), value);
			}
		}
		g_key_file_free (cfg_keyFile);
		g_free (file);

		return TRUE;

	} else  {
		g_print ("\n");
		g_print ("Opps, can't read config (%s): %s\n", file, err->message);
		g_error_free (err);

		g_key_file_free (cfg_keyFile);
		g_free (file);
		err = NULL;

		return FALSE;
	}
}

gboolean
cfg_configFileExist (void)
{
	gboolean exist;
	gchar *file = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, CFG_FILENAME, NULL);
	if (g_file_test (file, G_FILE_TEST_EXISTS)) {
		exist = TRUE;
	} else {
		exist = FALSE;
	}
	g_free (file);

	return exist;
}

const gchar*
cfg_keyStr (const gchar* key)
{
	g_assert (cfg_config);
	return (gchar*) g_hash_table_lookup (cfg_config, key);
}

gint
cfg_keyInt (const gchar* key)
{
	g_assert (cfg_config);
	gpointer out = g_hash_table_lookup (cfg_config, key);

	if (out) {
		return (gint) strtod (out, NULL);
	} else {
		return 0;
	}
}

gdouble
cfg_keyDbl (const gchar* key)
{
	g_assert (cfg_config);

	return strtod (g_hash_table_lookup (cfg_config, key), NULL);
}

gboolean
cfg_keyBool (const gchar* key)
{
	// 0 -> FALSE
	// else TRUE
	g_assert (cfg_config);

	return strtod ((gchar*) g_hash_table_lookup (cfg_config, key), NULL) == 0? FALSE: TRUE;
}

gboolean
cfg_saveConfig (void)
{
	gboolean saved = FALSE;
	gsize len;

	gchar *fileName = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, CFG_FILENAME, NULL);
	gchar *pathName = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, NULL);

	g_print ("saving config file (%s)\n", fileName);

	g_assert (fileName);
	g_assert (pathName);

	GKeyFile* keyFile = g_key_file_new ();

	/* adding config */
	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init (&iter, cfg_config);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		g_key_file_set_string (keyFile, CFG_SECTION, (gchar*) key, (gchar*) value);
	}

	g_key_file_set_comment (keyFile, CFG_SECTION, "TILE_PROVIDER", "TILE_PROVIDER\n" \
																  	"http://www.progettoemma.net/snap/%s/0000.png\n" \
																  	"http://www.progettoemma.net/snap/%s/title.png\n" \
																  	"http://www.progettoemma.net/snap/%s/flyer.png\n" \
																  	"http://www.progettoemma.net/snap/%s/score.png\n" \
																  	"http://www.progettoemma.net/snap/%s/gameover.png\n" \
																  	"\n" \
																  	"http://mrdo.mameworld.info/mame_artwork/%s.png\n" \
																  	"\n" \
																  	"http://www.mamedb.com/snap/%s.png\n" \
																	"http://www.mamedb.com/titles/%s.png\n" \
																	"http://www.mamedb.com/cabinets/%s.png\n" \
																	"\n" \
																	"http://adb.arcadeitalia.net/media/mame.current/ingames/%s.png\n" \
																	"http://adb.arcadeitalia.net/media/mame.current/titles/%s.png\n" \
																	"http://adb.arcadeitalia.net/media/mame.current/cabinets/%s.png\n" \
																, &err);

	if (err) {
		g_print ("Can't write to output stream: %s\n", err->message);
	    g_error_free (err);
	    err = NULL;
	}

	gchar *data = g_key_file_to_data (keyFile, &len, &err);
	if (data) {
		if (!g_mkdir_with_parents (pathName, 0700)) {

		    GFile *file = g_file_new_for_path (fileName);

		    GFileOutputStream *outStream = g_file_replace (file, NULL, TRUE, G_FILE_CREATE_PRIVATE, NULL, &err);
		    if (outStream) {
		    	if (g_output_stream_write (G_OUTPUT_STREAM (outStream), data, strlen (data), NULL, &err) == -1) {
					g_print ("Can't write to output stream: %s\n", err->message);
				    g_error_free (err);
				    err = NULL;
				    saved = FALSE;
		    	} else {
					saved = TRUE;
		    	}

		    	g_output_stream_close (G_OUTPUT_STREAM (outStream), NULL, NULL);
		    	g_object_unref (outStream);

		    } else {
				g_print ("Can't create output stream: %s\n", err->message);
			    g_error_free (err);
			    err = NULL;
			    saved = FALSE;
		    }
	    	g_object_unref (file);


		} else {
			g_print ("Can't create config directory\n");
			saved = FALSE;
		}
		g_free (data);

	} else {
		g_print ("Can't convert data to string: %s\n", err->message);
	    g_error_free (err);
	    err = NULL;
	    saved = FALSE;
	}

	g_key_file_free (keyFile);
	g_free (fileName);
	g_free (pathName);

	return saved;
}

