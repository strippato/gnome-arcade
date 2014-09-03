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
#include "pref.h"

#define PREF_FILENAME "gnome-arcade-preference.ini"
#define PREF_FLAGS G_KEY_FILE_KEEP_COMMENTS

#define PREF_KG_PREFFERED "FAVOURITE"
#define PREF_KG_RANK      "RANK"
#define PREF_KG_NPLAY     "NPLAY"

static gchar* pref_fileName = NULL;
static GKeyFile *pref_keyFile = NULL;

void
pref_init (void)
{
	g_assert (!pref_fileName);
	g_assert (!pref_keyFile);

	pref_fileName = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, PREF_FILENAME, NULL);
	pref_keyFile = g_key_file_new ();
}

void
pref_free (void)
{
	g_assert (pref_fileName);
	g_assert (pref_keyFile);

	g_free (pref_fileName);
 	g_key_file_free (pref_keyFile);

	pref_fileName = NULL;
	pref_keyFile = NULL;
}

void
pref_load (void)
{
	GError *error = NULL;
	g_assert (pref_fileName);
	g_assert (pref_keyFile);

	g_print ("loading preference %s", pref_fileName);
	if (!g_key_file_load_from_file (pref_keyFile, pref_fileName, PREF_FLAGS, &error)) {
    	g_error_free (error);
  		g_print (" " FAIL_MSG "\n");
  	} else {
  		g_print (" " SUCCESS_MSG "\n");
  	}

}

guint
pref_getNPlay (const char* key)
{
	if (!pref_keyFile) return 0;

	return g_key_file_get_integer (pref_keyFile, key, PREF_KG_NPLAY, NULL);
}

gboolean
pref_getPreferred (const char* key)
{
	if (!pref_keyFile) return FALSE;

	return g_key_file_get_boolean (pref_keyFile, key, PREF_KG_PREFFERED, NULL);
}

gint
pref_getRank (const char* key)
{
	if (!pref_keyFile) return 0;

	return g_key_file_get_integer (pref_keyFile, key, PREF_KG_RANK, NULL);
}

void
pref_save (void)
{
	GError *error = NULL;
	gsize size;

	if (pref_keyFile) {
		gchar *data = g_key_file_to_data (pref_keyFile, &size, &error);
		if (!data) {
	    	g_warning ("%s", error->message);
	    	g_error_free (error);
		} else {
			if (g_file_set_contents (pref_fileName, data, size,  &error)) {
				g_print ("preferrence saved " SUCCESS_MSG "\n");

			} else {
		    	g_warning ("%s", error->message);
		    	g_error_free (error);
			}

 			g_free (data);
		}

	}

}

void
pref_setNPlay (const char* key, guint value)
{
	g_key_file_set_integer (pref_keyFile, key, PREF_KG_NPLAY, value);
}

void
pref_setPreferred (const char* key, gboolean value)
{
	g_key_file_set_boolean (pref_keyFile, key, PREF_KG_PREFFERED, value);
}

void
pref_setRank (const char* key, gint rank)
{
	g_key_file_set_integer (pref_keyFile, key, PREF_KG_RANK, rank);
}
