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

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <glib/gstdio.h>

#include "global.h"
#include "util.h"
#include "app.h"
#include "filter.h"

#define FILTER_FILE "filter.txt"
#define FILTER_LIST_BUFSIZE 255

static GHashTable *filter_skipTable = NULL;

void
filter_init (void)
{
    int numSkip = 0;
    gchar buf[FILTER_LIST_BUFSIZE];

    g_assert(!filter_skipTable);
    filter_skipTable = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_assert(filter_skipTable);

    // load
    gchar *fileFilter = g_build_filename (APP_RESOURCE, FILTER_FILE, NULL);

    if (!g_file_test (fileFilter, G_FILE_TEST_EXISTS)) {
        g_print ("Filter file not found (%s) %s\n", fileFilter, FAIL_MSG);
    } else {
        g_print ("Loading filter %s", fileFilter);

        FILE *file = g_fopen (fileFilter, "r");

        gboolean skipLine;
        while (fgets (buf, sizeof (buf), file)) {

            if (g_str_has_prefix (buf, "#")) {
                // skip comments
                skipLine = TRUE;
            } else if (g_str_has_prefix (buf, "[")) {
                // skip sections
                skipLine = TRUE;
            } else if (buf[0] == '\x0d') {
                // skip empty line
                skipLine = TRUE;
            } else if (buf[0] == ' ') {
                // skip space
                skipLine = TRUE;
            } else {
                skipLine = FALSE;
            }
            if (!skipLine) {
                numSkip++;

                gchar **lineVec;
                gchar *romName;

                lineVec = g_strsplit (buf, " ", 2);
                romName = g_strndup (lineVec[0], strlen (lineVec[0]) - 2);

                g_strfreev (lineVec);
                //g_print ("-> %s (%li)\n", romName, strlen(romName));
                g_hash_table_insert (filter_skipTable, romName, NULL);
            }

        }

        g_print ("... found %i item %s\n", numSkip, SUCCESS_MSG);

        fclose (file);
    }

    g_free (fileFilter);

}

void
filter_free (void)
{
    g_hash_table_destroy (filter_skipTable);
    filter_skipTable = NULL;
}

gboolean
filter_skipRom (const gchar *romName)
{
    if (g_hash_table_contains (filter_skipTable, romName)) {
        return TRUE;
    } else {
        return FALSE;
    }
}
