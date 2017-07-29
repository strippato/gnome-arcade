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
#include "blacklist.h"

#define BLIST_FILE "blacklist.ini"
#define BLIST_LIST_BUFSIZE 255

static GHashTable *blist_skipTable = NULL;

void
blist_init (void)
{
    int numSkip = 0;
    gchar buf[BLIST_LIST_BUFSIZE];

    g_assert(!blist_skipTable);
    blist_skipTable = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_assert(blist_skipTable);

    // load
    gchar *fileBList = g_build_filename (APP_RESOURCE, BLIST_FILE, NULL);

    if (!g_file_test (fileBList, G_FILE_TEST_EXISTS)) {
        g_print ("blacklist file not found (%s) %s\n", fileBList, FAIL_MSG);
    } else {
        g_print ("loading blacklist %s", fileBList);

        FILE *file = g_fopen (fileBList, "r");

        while (fgets (buf, sizeof (buf), file)) {

            switch (buf[0]) {
            
            // skip
            case '#':
            case '[':
            case '\x0d':
            case '\x0a':
            case ' ':            
                break;

            default:
                numSkip++;

                gchar **lineVec;
                gchar *romName;

                lineVec = g_strsplit (buf, " ", 2);
                romName = g_strndup (lineVec[0], strlen (lineVec[0]) - 2);

                g_strfreev (lineVec);

                //g_print ("-> %s (%li)\n", romName, strlen(romName));
                g_hash_table_insert (blist_skipTable, romName, NULL);
                break;
            }
        }

        g_print ("... found %i item %s\n", numSkip, SUCCESS_MSG);

        fclose (file);
    }

    g_free (fileBList);

}

void
blist_free (void)
{
    g_hash_table_destroy (blist_skipTable);
    blist_skipTable = NULL;
}

gboolean
blist_skipRom (const gchar *romName)
{
    if (g_hash_table_contains (blist_skipTable, romName)) {
        return TRUE;
    } else {
        return FALSE;
    }
}
