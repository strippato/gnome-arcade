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
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gstdio.h>
#include <assert.h>
#include <string.h>

#include "app.h"
#include "global.h"
#include "rom.h"
#include "mame.h"
#include "view.h"
#include "ui.h"
#include "config.h"
#include "pref.h"
#include "util.h"


#define MAME_EXE       cfg_keyStr("MAME_EXE")
#define MAME_OPTIONS   cfg_keyStr("MAME_OPTIONS")

#define MAME_LIST_FULL   "listfull"
#define MAME_LIST_CLONES "listclones"

#define MAME_LIST_FULL_FILE   "listfull.txt"
#define MAME_LIST_CLONES_FILE "listclones.txt"

#define MAME_LIST_BUFSIZE 255

static gboolean mame_mameIsRunning = FALSE;

static gchar*
mame_getVersion (void)
{
    const int MAXSIZE = 80;
    char buf[MAXSIZE];
    gchar *version = NULL;

    gchar *cmdLine = g_strdup_printf ("%s -help", MAME_EXE);

    FILE *file = popen (cmdLine, "r");

    if (fgets (buf, MAXSIZE - 1, file) != NULL) {
        version = g_strndup (buf, strlen (buf) - 1);
    }

    pclose (file);
    g_free (cmdLine);

    return version;
}

static gboolean
mame_dumpTo (gchar *cmdline, gchar *file)
{

    gchar **argv  = NULL;
    GError *error = NULL;
    gchar *stdout = NULL;

    gint status;

    if (!g_shell_parse_argv (cmdline, NULL, &argv, &error)) {
        g_error_free (error);
        g_strfreev (argv);
        return FALSE;
    }

    if (!g_spawn_sync (NULL, argv, NULL, G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, &stdout, NULL, &status, &error)) {

        if (error) g_error_free (error);
        if (stdout) g_free (stdout);

        g_strfreev (argv);
        return FALSE;

    } else {

        g_file_set_contents (file, stdout, -1, &error);

        if (error) g_error_free (error);
        if (stdout) g_free (stdout);

        g_strfreev (argv);
        return TRUE;
    }
}

static gboolean
mame_run (gchar *cmdline, GPid *pid, gint *stdout, gint *stderr) {

    gchar **argv  = NULL;
    GError *error = NULL;
    gboolean runOk = FALSE;

    if (!g_shell_parse_argv (cmdline, NULL, &argv, &error)) {
        g_error_free (error);
        error = NULL;
        return runOk;
    }

    if (!g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, pid, NULL, stdout, stderr, &error)) {
        g_error_free (error);
        error = NULL;
    } else {
        g_print ("mame pid is %lu\n", (gulong)*pid);
        runOk = TRUE;
    }

    g_strfreev (argv);
    argv = NULL;

    return runOk;
}

void
mame_gameList (void)
{

/*
 mame -listclones
Name:            Clone of:
10yard85         10yard
10yardj          10yard
18w2             18w
18wheels         18wheelr
1941j            1941
...

 mame -listfull
...
zoar              "Zoar"
zodiack           "Zodiack"
zokumahj          "Zoku Mahjong Housoukyoku (Japan)"
zokuoten          "Zoku Otenamihaiken (V2.03J)"
zombraid          "Zombie Raid (9/28/95, US)"
zombraidp         "Zombie Raid (9/28/95, US, prototype PCB)"
zombraidpj        "Zombie Raid (9/28/95, Japan, prototype PCB)"
zombrvn           "Zombie Revenge (JPN, USA, EXP, KOR, AUS)"
zoo               "Zoo (Ver. ZO.02.D)"
...
*/

    #define CLONE_OFFSET 17

    gchar* mameVersion = NULL;
    if (g_file_test (MAME_EXE, G_FILE_TEST_IS_EXECUTABLE)) {
        mameVersion = mame_getVersion ();
        g_print ("%s\n", mameVersion);
    } else {
        g_print ("Can't find M.A.M.E. (%s), please edit your config", MAME_EXE);
        g_print (" " FAIL_MSG "\n");
    }

    FILE *file;

    gchar buf[MAME_LIST_BUFSIZE];
    gchar *romNameZip;
    gchar *romName7Zip;
    gchar *romNameDir;
    gchar *cmdLine;

    gint numGame = 0;
    gint numGameSupported = 0;
    gint numClone = 0;
    gint numBios = 0;
    gboolean skipFirst = TRUE;
    gchar* romPath;

    gchar *fileRom = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, MAME_LIST_FULL_FILE, NULL);
    gchar *fileClone = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, MAME_LIST_CLONES_FILE, NULL);

#ifdef DEBUG_TIMING
    logTimer ("Start loading romlist");
#endif

    if (g_str_has_prefix (cfg_keyStr ("ROM_PATH"), "~")) {
        romPath = g_strdup_printf ("%s%s", g_get_home_dir (), cfg_keyStr ("ROM_PATH")+1);
    } else {
        romPath = g_strdup_printf ("%s", cfg_keyStr ("ROM_PATH"));
    }

    g_assert (g_file_test (romPath, G_FILE_TEST_IS_DIR));

    if (g_file_test (MAME_EXE, G_FILE_TEST_IS_EXECUTABLE)) {

        if (cfg_keyBool ("ROMLIST_FROM_FILE")) {
            //g_print ("\n%s %s\n", fileRom, fileClone);
            if (!g_file_test (fileRom, G_FILE_TEST_EXISTS)) {
                g_print ("%s not found, rebuilding... ", MAME_LIST_FULL_FILE);
                cmdLine = g_strdup_printf ("%s -" MAME_LIST_FULL, MAME_EXE);
                if (!mame_dumpTo (cmdLine, fileRom)) {
                    g_print (FAIL_MSG "\n");
                }
                g_print (SUCCESS_MSG "\n");
                g_free (cmdLine);
            }
            if (!g_file_test (fileClone, G_FILE_TEST_EXISTS)) {
                g_print ("%s not found, rebuilding... ", MAME_LIST_CLONES_FILE);
                cmdLine = g_strdup_printf ("%s -" MAME_LIST_CLONES, MAME_EXE);
                if (!mame_dumpTo (cmdLine, fileClone)) {
                    g_print (FAIL_MSG "\n");
                }
                g_print (SUCCESS_MSG "\n");
                g_free (cmdLine);
            }

            g_print ("gamelist from %s %s\n", MAME_LIST_FULL_FILE, MAME_LIST_CLONES_FILE);
        } else {
            g_print ("gamelist from %s\n", MAME_EXE);
        }

        g_print ("rom from %s\n", romPath);
        g_print ("tile from %s\n", rom_tilePath);

        // load clone table
        g_print ("loading clonetable");
        cmdLine = g_strdup_printf ("%s -" MAME_LIST_CLONES, MAME_EXE);

        if (cfg_keyBool ("ROMLIST_FROM_FILE")) {
            assert (g_file_test (fileClone, G_FILE_TEST_EXISTS));
            file = g_fopen (fileClone, "r");
        } else {
            file = popen (cmdLine, "r");
        }

        skipFirst = TRUE;
        while (fgets (buf, sizeof (buf), file)) {
            if (skipFirst) {
                skipFirst = FALSE;
                continue;
            }
            numClone++;

            gchar **lineVec;
            gchar *romName;
            gchar *cloneof;

            lineVec = g_strsplit (buf, " ", 2);
            romName = g_strdup (lineVec[0]);
            g_strfreev (lineVec);

            cloneof = g_strdup (g_strstrip (buf + CLONE_OFFSET));
            //g_print ("roms *%s* is clone of *%s*\n", romname, cloneof);

            g_hash_table_insert (rom_cloneTable, romName, cloneof);

            if (numClone % 500 == 0) g_print (".");

            // don't free romname and cloneof
            //g_free (romname);
            //g_free (cloneof);
        }

        g_free (cmdLine);

        if (cfg_keyBool ("ROMLIST_FROM_FILE")) {
            fclose (file);
        } else {
            pclose (file);
        }

        g_print (" " SUCCESS_MSG " (%i)\n", numClone);

        // load romlist
        g_print ("loading romlist");

        cmdLine = g_strdup_printf ("%s -" MAME_LIST_FULL, MAME_EXE);

        if (cfg_keyBool ("ROMLIST_FROM_FILE")) {
            assert (g_file_test (fileRom, G_FILE_TEST_EXISTS));
            file = g_fopen (fileRom, "r");
        } else {
            file = popen (cmdLine, "r");
        }

        skipFirst = TRUE;
        while (fgets (buf, sizeof (buf), file)) {
            if (skipFirst) {
                skipFirst = FALSE;
                continue;
            }

            numGameSupported++;

#ifdef DEBUG_ROM_LIMIT
            if (numGame >= DEBUG_ROM_LIMIT) continue;
#endif

            gchar *tempstr = strstr (buf, " ");
            assert (tempstr);

            gchar *name = g_strndup (buf, tempstr - buf);

            /* show the tile, only if we find the rom */
            romNameZip = g_strdup_printf ("%s%s.%s", romPath, name, ROM_EXTENSION_ZIP);
            romName7Zip = g_strdup_printf ("%s%s.%s", romPath, name, ROM_EXTENSION_7ZIP);
            romNameDir = g_strdup_printf ("%s%s", romPath, name);

            gboolean foundRom = FALSE;

            // clones romset will not be checked for performance reasons
            if (rom_isParent (name)) {
                if (g_file_test (romNameZip, G_FILE_TEST_EXISTS)) {
                    foundRom = TRUE;
                } else if (g_file_test (romName7Zip, G_FILE_TEST_EXISTS)) {
                    foundRom = TRUE;
                } else if (g_file_test (romNameDir, G_FILE_TEST_IS_DIR)) {
                    foundRom = TRUE;
                }
            }

            tempstr = strstr (buf, "\"");
            assert (tempstr);
            tempstr++;
            gchar *nameDes = g_strndup (tempstr, strlen (tempstr) - 2);

            // g_print ("*%s*%zu\n", nameDes, strlen(nameDes));
            if (!rom_filterBios (nameDes)) {
                struct rom_romItem *item = rom_newItem ();

                rom_setItemName (item, name);
                rom_setItemDescription (item, nameDes);
                rom_setItemDesc (item, nameDes);
                rom_setItemTile (item, NULL);
                rom_setItemRank (item, pref_getRank (name));
                rom_setItemPref (item, pref_getPreferred (name));
                rom_setItemNPlay (item, pref_getNPlay (name));
                rom_setItemRomFound (item, foundRom);

                if (foundRom) ++numGame;
            } else {
                if (foundRom) ++numBios;
            }
            g_free (nameDes);
            g_free (name);

            g_free (romNameZip);
            g_free (romName7Zip);
            g_free (romNameDir);

            if (numGameSupported % 600 == 0) g_print (".");

        }
        if (cfg_keyBool ("ROMLIST_FROM_FILE")) {
            fclose (file);
        } else {
            pclose (file);
        }

        g_free (cmdLine);

        g_print (" " SUCCESS_MSG " (%i)\n", numGameSupported);

        g_print ("found %i of %i rom (%i bios skipped)\n", numGame, numGameSupported, numBios);
    }

    g_free (romPath);
    g_free (fileRom);
    g_free (fileClone);

    g_free (mameVersion);
#ifdef DEBUG_TIMING
    logTimer ("end loading romlist");
#endif

}

void
mame_quit (GPid pid)
{
    g_print ("quitting from mame...\n");

    mame_mameIsRunning = FALSE;

    g_spawn_close_pid (pid);

    ui_setPlayBtnState (TRUE);
    ui_setToolBarState (TRUE);
    ui_setScrollBarState (TRUE);
    ui_invalidateDrawingArea ();
    ui_setFocus ();
}

gboolean
mame_playGame (struct rom_romItem *item)
{
    GPid pid;
    gboolean played = FALSE;
    const gchar *romName;
    gchar *cmdline;
    gchar *romPath;
    gchar *romPathQuoted;

    if (g_str_has_prefix (cfg_keyStr ("ROM_PATH"), "~")) {
        romPath = g_strdup_printf ("%s%s", g_get_home_dir (), cfg_keyStr ("ROM_PATH") + 1);
    } else {
        romPath = g_strdup_printf ("%s", cfg_keyStr ("ROM_PATH"));
    }
    romPathQuoted = g_shell_quote (romPath);

    romName = rom_getItemName (item);
    cmdline = g_strdup_printf ("%s %s -rompath %s %s", MAME_EXE, cfg_keyStr("MAME_OPTIONS"), romPathQuoted, romName);

    /* free pixbuff cache, so mame can use more memory */
    rom_invalidateUselessTile ();

    g_print ("playing %s\n", romName);

    if (mame_run (cmdline, &pid, NULL, NULL)) {
        played = TRUE;
        ui_setPlayBtnState (FALSE);
        ui_setToolBarState (FALSE);
        ui_setScrollBarState (FALSE);

        mame_mameIsRunning = TRUE;
        g_child_watch_add (pid, (GChildWatchFunc) mame_quit, &pid);
    } else {
        g_print ("oops, something goes wrong again...\n");
        mame_mameIsRunning = FALSE;
    }
    g_free (cmdline);
    g_free (romPath);
    g_free (romPathQuoted);
    return played;
}

inline gboolean
mame_isRunning (void)
{
    return mame_mameIsRunning;
}
