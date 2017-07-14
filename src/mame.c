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
#include <glib.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <assert.h>
#include <string.h>

#include "app.h"
#include "global.h"
#include "rom.h"
#include "inforom.h"
#include "mame.h"
#include "view.h"
#include "ui.h"
#include "config.h"
#include "pref.h"
#include "util.h"
#include "joy.h"
#include "ssaver.h"
#include "blacklist.h"
#include "www.h"
#include "filedownloader.h"


#define MAME_EXE       cfg_keyStr ("MAME_EXE")
#define MAME_OPTIONS   cfg_keyStr ("MAME_OPTIONS")

#define MAME_LIST_FULL   "listfull"
#define MAME_LIST_CLONES "listclones"


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

        // skip the first line:
        // Name:             Description:
        gchar* outbuf = g_strrstr (stdout, ":\n") + 2;

        g_file_set_contents (file, outbuf, -1, &error);

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

    if (!g_shell_parse_argv (cmdline, NULL, &argv, &error)) {
        g_error_free (error);
        error = NULL;
        return FALSE;
    }

    if (!g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, pid, NULL, stdout, stderr, &error)) {
        g_error_free (error);
        error = NULL;
        g_strfreev (argv);
        return FALSE;
    } else {
        g_print ("mame pid is %lu\n", (gulong) *pid);
        g_strfreev (argv);
        return TRUE;
    }

}

static void
mame_dumpLine (struct rom_romItem *item, FILE* stream)
{
    fprintf (stream, "%-18s\"%s\"\n", item->name, item->description);
}

static void
mame_dumpSortedRomList (const gchar *fileRomList)
{

    g_print ("writing romlist to disk... ");

    if (g_unlink (fileRomList)) {
        g_print ("can't delete romlist (%s) ", fileRomList);

        g_print (FAIL_MSG "\n");
    } else {
        FILE *stream = g_fopen (fileRomList, "w");
        g_list_foreach (rom_romList, (GFunc) mame_dumpLine, stream);
        fclose (stream);

        g_print (SUCCESS_MSG "\n");
    }
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

    gboolean romDownload = cfg_keyBool ("ROM_DOWNLOAD");

    gchar *fileRom = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, MAME_LIST_FULL_FILE, NULL);
    gchar *fileClone = g_build_filename (g_get_user_config_dir (), APP_DIRCONFIG, MAME_LIST_CLONES_FILE, NULL);

    gchar* mameVersion = NULL;
    if (g_file_test (MAME_EXE, G_FILE_TEST_IS_EXECUTABLE)) {

        mameVersion = mame_getVersion ();
        g_print ("%s\n", mameVersion);

        if (g_strcmp0 (mameVersion, cfg_keyStr ("MAME_RELEASE")) != 0) {
            g_print ("version mismatch, need a rebuild\n");
            // delete romlist/clonelist
            if (g_file_test (fileRom, G_FILE_TEST_EXISTS)) {
                g_print ("deleting %s ", fileRom);
                if (g_remove (fileRom) == 0) {
                    g_print (SUCCESS_MSG "\n");
                } else {
                    g_print (FAIL_MSG "\n");
                }
            }

            if (g_file_test (fileClone, G_FILE_TEST_EXISTS)) {
                g_print ("deleting %s ", fileClone);
                if (g_remove (fileClone) == 0) {
                    g_print (SUCCESS_MSG "\n");
                } else {
                    g_print (FAIL_MSG "\n");
                }

            }

            cfg_setConfig ("MAME_RELEASE", mameVersion);
            cfg_saveConfig ();
        }
    } else {
        g_print ("Can't find M.A.M.E. (%s), please edit your config", MAME_EXE);
        g_print (" " FAIL_MSG "\n");
    }

    FILE *file;

    gchar buf[MAME_LIST_BUFSIZE];
    gchar *cmdLine;

    gint numGame = 0;
    gint numGameSupported = 0;
    gint numClone = 0;

    gchar* romPath = g_strdup (cfg_keyStr ("ROM_PATH"));
    gchar* romChd = g_strdup (cfg_keyStr ("CHD_PATH"));

    g_assert (g_file_test (romPath, G_FILE_TEST_IS_DIR));
    g_assert (g_file_test (romChd, G_FILE_TEST_IS_DIR));

    gboolean romListCreated = FALSE;

    if (g_file_test (MAME_EXE, G_FILE_TEST_IS_EXECUTABLE)) {

        if (!g_file_test (fileRom, G_FILE_TEST_EXISTS)) {
            g_print ("%s not found, rebuilding... ", MAME_LIST_FULL_FILE);
            cmdLine = g_strdup_printf ("%s -" MAME_LIST_FULL, MAME_EXE);
            if (!mame_dumpTo (cmdLine, fileRom)) {
                g_print (FAIL_MSG "\n");
            } else {
                romListCreated = TRUE;
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

        g_print ("rom from %s\n", romPath);
        g_print ("chd from %s\n", romChd);
        g_print ("tile from %s\n", rom_tilePath);

        // load clone table
        g_print ("loading clonetable");
        cmdLine = g_strdup_printf ("%s -" MAME_LIST_CLONES, MAME_EXE);

        assert (g_file_test (fileClone, G_FILE_TEST_EXISTS));
        file = g_fopen (fileClone, "r");

        while (fgets (buf, sizeof (buf), file)) {

            gchar **lineVec;
            gchar *romName;
            gchar *cloneof;

            lineVec = g_strsplit (buf, " ", 2);
            romName = g_strdup (lineVec[0]);
            g_strfreev (lineVec);

            if (!blist_skipRom (romName)) {
                numClone++;

                cloneof = g_strdup (g_strstrip (buf + CLONE_OFFSET));

                g_hash_table_insert (rom_cloneTable, romName, cloneof);

                if (numClone % 150 == 0) g_print (".");

                // don't free romname and cloneof
                //g_free (romname);
                //g_free (cloneof);
            } else {
                g_free (romName);
            }
        }

        g_free (cmdLine);

        fclose (file);

        g_print (" " SUCCESS_MSG " (%i)\n", numClone);

        // load romlist
        g_print ("loading romlist");

        cmdLine = g_strdup_printf ("%s -" MAME_LIST_FULL, MAME_EXE);

        assert (g_file_test (fileRom, G_FILE_TEST_EXISTS));
        file = g_fopen (fileRom, "r");

        while (fgets (buf, sizeof (buf), file)) {

            gchar *tempstr = strstr (buf, " ");
            assert (tempstr);

            gchar *name = g_strndup (buf, tempstr - buf);

            if (!blist_skipRom (name)) {

                /* we must show the tile, only if we find the rom */
                gboolean foundRom = FALSE;

                tempstr = strstr (buf, "\"");
                assert (tempstr);
                tempstr++;
                gchar *nameDes = g_strndup (tempstr, strlen (tempstr) - 2);

                struct rom_romItem *item = rom_newItem ();

                rom_setItemName (item, name);
                rom_setItemDescription (item, nameDes);
                rom_setItemTile (item, NULL);
                rom_setItemRank (item, pref_getRank (name));
                rom_setItemPref (item, pref_getPreferred (name));
                rom_setItemNPlay (item, pref_getNPlay (name));

                // clones romset will not be checked for performance reasons
                if (rom_isParent (name)) {
                    numGameSupported++;
                    if (romDownload) {
                        // autoownloading rom, don't check the rom on disk
                        foundRom = TRUE;
                    } else {
                        if (rom_FoundInPath (name, cfg_keyStr ("ROM_PATH"), cfg_keyStr ("CHD_PATH"), NULL)) {
                            foundRom = TRUE;
                        }
                    }
                    if (numGameSupported % 150 == 0) g_print (".");
                } else {
                    // clone
                    gchar *parent = g_hash_table_lookup (rom_cloneTable, name);

                    if (parent) {
                        // clone (clonetablesearch)
                        if (!g_hash_table_contains (rom_parentTableSearch, parent)) {
                            // insert clone
                            gchar *data = g_strjoin ("\n", nameDes, name, NULL);
                            gchar *dataUcase = g_utf8_strup (data, -1);
                            g_free (data);

                            g_hash_table_insert (rom_parentTableSearch, g_strdup (parent), dataUcase);
                        } else {
                            // add another clone
                            gchar *olddata = g_hash_table_lookup (rom_parentTableSearch, parent);
                            gchar *newdata = g_strjoin ("\n", olddata, nameDes, name, NULL);
                            g_hash_table_remove (rom_parentTableSearch, parent);

                            gchar *dataUcase = g_utf8_strup (newdata, -1);
                            g_free (newdata);

                            g_hash_table_insert (rom_parentTableSearch, g_strdup (parent), dataUcase);
                        }

                        // clone (clonetable)
                        if (!g_hash_table_contains (rom_parentTable, parent)) {
                            // insert clone
                            gchar *data = g_strdup_printf ("%s [%s]", nameDes, name);

                            g_hash_table_insert (rom_parentTable, g_strdup (parent), data);
                        } else {
                            // add another clone
                            gchar *olddata = g_hash_table_lookup (rom_parentTable, parent);
                            gchar *newdata = g_strdup_printf ("%s\n%s [%s]", olddata, nameDes, name);
                            g_hash_table_remove (rom_parentTable, parent);

                            g_hash_table_insert (rom_parentTable, g_strdup (parent), newdata);
                        }
                    }
                }

                rom_setItemRomFound (item, foundRom);
                if (foundRom) ++numGame;

                g_free (nameDes);
            }
            g_free (name);

        }

        fclose (file);
        g_free (cmdLine);

        g_print (" " SUCCESS_MSG " (%i)\n", numGameSupported);
        g_print ("found %i of %i rom\n", numGame, numGameSupported);
    }

    rom_romList = g_list_reverse (rom_romList);

    if (romListCreated) {
        // sort and redump to disk
        rom_setSort (ROM_SORT_AZ);

        mame_dumpSortedRomList (fileRom);
    }

    g_free (romPath);
    g_free (romChd);
    g_free (fileRom);
    g_free (fileClone);

    g_free (mameVersion);
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

    joy_init (); // restart joypad

    if (cfg_keyInt ("SCREENSAVER_MODE") == 1) {
        ssaver_resume ();
    }
}

gboolean
mame_playGame (struct rom_romItem* item, const char* clone)
{
    GPid pid;
    gboolean played = FALSE;
    gchar *romPath;
    const gchar *romName;

    if (g_strcmp0 (cfg_keyStr ("ROM_PATH"), cfg_keyStr ("CHD_PATH")) == 0) {
        romPath = g_strdup (cfg_keyStr ("ROM_PATH"));
    } else {
        romPath = g_strjoin (";", cfg_keyStr ("ROM_PATH"), cfg_keyStr ("CHD_PATH"), NULL);
    }
    if (cfg_keyBool ("ROM_DOWNLOAD")) {
        if (fd_getDownloadPathRom ()) {
            gchar *newRomPath = g_strjoin (";", romPath, fd_getDownloadPathRom (), NULL);
            g_free (romPath);
            romPath = newRomPath;
        }
    }
    if (cfg_keyBool ("CHD_DOWNLOAD")) {
        if (fd_getDownloadPathChd ()) {
            gchar *newRomPath = g_strjoin (";", romPath, fd_getDownloadPathChd (), NULL);
            g_free (romPath);
            romPath = newRomPath;
        }
    }

    gchar *romPathQuoted = g_shell_quote (romPath);

    // clone ?
    if (clone) {
        romName = clone;
    } else {
        romName = rom_getItemName (item);
    }
    gchar *cmdline = g_strdup_printf ("%s %s -rompath %s %s", MAME_EXE, cfg_keyStr ("MAME_OPTIONS"), romPathQuoted, romName);

    /* free pixbuff cache, so mame can use more memory */
    rom_invalidateUselessTile ();

    g_print ("playing %s\n", romName);

    gchar *romOf = NULL; // bios/parent
    gboolean usrWantContinue = TRUE;

    if (cfg_keyBool ("ROM_DOWNLOAD")) {

        if (rom_isParent (romName)) {

            // get chd
            if (cfg_keyBool ("CHD_DOWNLOAD")) {
                if (mame_needChd (romName)) {
                    g_print ("WARNING: THIS ROM NEED ONE OR MORE CHD\n");
                    if (!rom_FoundInPath (romName, fd_getDownloadPathChd (), cfg_keyStr ("CHD_PATH"), NULL)) {
                        usrWantContinue = ui_downloadAsk ();
                        if (usrWantContinue) {
                            fd_findAndDownloadChd (romName);
                        }
                    }
                }
            }

            if (usrWantContinue) {
                // get the bios
                romOf = mame_getRomOf (romName);
                if (romOf) {
                    if (!rom_FoundInPath (romOf, fd_getDownloadPathRom (), cfg_keyStr ("ROM_PATH"), NULL)) {
                        fd_downloadRom (romOf);
                    }
                }

                // get the rom
                if (!rom_FoundInPath (romName, fd_getDownloadPathRom (), cfg_keyStr ("ROM_PATH"), NULL)) {
                    fd_downloadRom (romName);
                }

                // get device(s) bios
                gchar **vbios = mame_getDeviceRomOf (romName);
                if (vbios) {
                    for (int i = 0; vbios[i]; ++i) {
                        if (!rom_FoundInPath (vbios[i], fd_getDownloadPathRom (), cfg_keyStr ("ROM_PATH"), NULL)) {
                            fd_downloadRom (vbios[i]);
                        }
                    }
                }
                g_free (romOf);
                g_strfreev (vbios);
                romOf = NULL;
                vbios = NULL;
            }

        } else {

            // get chd
            if (cfg_keyBool ("CHD_DOWNLOAD")) {
                if (mame_needChd (rom_parentOf (romName))) {
                    g_print ("WARNING: THIS ROM NEED ONE OR MORE CHD\n");
                    if (!rom_FoundInPath (rom_parentOf (romName), fd_getDownloadPathChd (), cfg_keyStr ("CHD_PATH"), NULL)) {
                        usrWantContinue = ui_downloadAsk ();
                        if (usrWantContinue) {
                            fd_findAndDownloadChd (rom_parentOf (romName));
                        }
                    }
                }
            }

            if (usrWantContinue) {
                // get the bios (parent)
                romOf = mame_getRomOf (rom_parentOf (romName));
                if (romOf) {
                    if (!rom_FoundInPath (romOf, fd_getDownloadPathRom (), cfg_keyStr ("ROM_PATH"), NULL)) {
                        fd_downloadRom (romOf);
                    }
                }

                // get the rom (parent)
                if (!rom_FoundInPath (rom_parentOf (romName), fd_getDownloadPathRom (), cfg_keyStr ("ROM_PATH"), NULL)) {
                    fd_downloadRom (rom_parentOf (romName));
                }

                // get the rom (clone)
                if (!rom_FoundInPath (romName, fd_getDownloadPathRom (), cfg_keyStr ("ROM_PATH"), NULL)) {
                    fd_downloadRom (romName);
                }

                // get the device(s) bios (clone)
                gchar **vbios = mame_getDeviceRomOf (romName);
                if (vbios) {
                    for (int i = 0; vbios[i]; ++i) {
                        if (!rom_FoundInPath (vbios[i], fd_getDownloadPathRom (), cfg_keyStr ("ROM_PATH"), NULL)) {
                            fd_downloadRom (vbios[i]);
                        }
                    }
                }
                g_free (romOf);
                g_strfreev (vbios);
                romOf = NULL;
                vbios = NULL;
            }
        }

        if (fd_downloadingItm != 0) {
            ui_downloadWarn (ROM_LEGAL);
        }
    }

    // u can't play while downloading rom
    if (fd_downloadingItm != 0) {
        ui_setPlayBtnState (FALSE);
        ui_setDropBtnState (FALSE);
        ui_setToolBarState (FALSE);
    } else if (usrWantContinue) {
        if (mame_run (cmdline, &pid, NULL, NULL)) {
            // disable the screen saver
            if (cfg_keyInt ("SCREENSAVER_MODE") == 1) {
                ssaver_suspend ();
            }

            // detach joypad
            joy_free ();

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
    }
    return played;
}

gboolean
mame_isRunning (void)
{
    return mame_mameIsRunning;
}


gchar*
mame_getRomOf (const gchar* romName)
{
    // return the romof attribute
    //<machine name="neocup98" sourcefile="neodriv.hxx" romof="neogeo">

    struct inforom_info *info = mame_getInfoRom (romName);

    gchar *romOf = g_strdup (info->romOf);

    mame_freeInfoRom (info);

    return romOf;
}


gchar**
mame_getDeviceRomOf (const gchar* romName)
{
/*
// return the machine[] names with device (isdevice="yes") associate with some "<rom name=xxxx"
// return: ["mie", "jvs13551", NULL]


<mame build="0.176 (unknown)" debug="no" mameconfig="10">
    <machine name="hotd2" sourcefile="naomi.cpp" romof="hod2bios">
        <description>House of the Dead 2 (USA)</description>
    </machine>
...
    <machine name="mie" sourcefile="src/mame/machine/mie.cpp" isdevice="yes" runnable="no">
        <description>Sega 315-6146 MIE</description>
        <rom name="315-6146.bin" size="2048" crc="9b197e35" sha1="864d14d58732dd4e2ee538ccc71fa8df7013ba06" region="mie" offset="0"/>
        <chip type="cpu" tag=":mie" name="Z80" clock="16000000"/>
    </machine>
    <machine name="z80" sourcefile="src/devices/cpu/z80/z80.cpp" isdevice="yes" runnable="no">
        <description>Z80</description>
    </machine>
    <machine name="mie_jvs" sourcefile="src/mame/machine/mie.cpp" isdevice="yes" runnable="no">
        <description>JVS (MIE)</description>
    </machine>
    <machine name="jvs13551" sourcefile="src/mame/machine/jvs13551.cpp" isdevice="yes" runnable="no">
        <description>Sega 837-13551 I/O Board</description>
        <rom name="sp5001.bin" size="32768" crc="2f17e21a" sha1="ac227ef3ca52ef17321bd60e435dba147645d8b8" region="jvs13551" offset="0"/>
        <rom name="sp5001-b.bin" size="32768" crc="121693cd" sha1="c9834aca671aff5e283ac708788c2a0f4a5bdecc" region="jvs13551" offset="0"/>
        <rom name="sp5002-a.bin" size="32768" crc="a088df8c" sha1="8237e9b18b8367d3f5b99b8f29c528a55c2e0fbf" region="jvs13551" offset="0"/>
        <rom name="315-6215.bin" size="32768" crc="d7c97e40" sha1="b1ae8db332f869c4fdbbae15967baeca0bc7f57d" region="jvs13551" offset="0"/>
        <input players="1" coins="2">
        </input>
    </machine>
*/

    const int MAXSIZE = 255;
    char buf[MAXSIZE];

    gboolean machineDevice = FALSE;
    gchar *line     = NULL;
    gchar *retname  = NULL;
    gchar *namelist = NULL;

    const gchar *findmachine = "<machine name=\"";
    const gchar *finddevice  = "isdevice=\"yes\"";

    const gchar *findromname = "<rom name=\"";

    const gchar *endtag      = "</machine>";

    gchar *cmdLine = g_strdup_printf ("%s -lx %s", MAME_EXE, romName);

    FILE *file = popen (cmdLine, "r");

    while (fgets (buf, MAXSIZE - 1, file)) {
        line = g_strndup (buf, strlen (buf) - 1);

        if (g_strrstr (line, endtag)) {
            machineDevice = FALSE;
        }

        if (machineDevice) {
            if (g_strrstr (line, findromname)) {
                // <rom name="sp5001.bin" size="32768" crc="2f17e21a" sha1="ac227ef3ca52ef17321bd60e435dba147645d8b8" region="jvs13551" offset="0"/>
                machineDevice = FALSE;
                gchar* oldlist = namelist;
                namelist = g_strjoin ("|", retname, oldlist, NULL);
                g_free (oldlist);
            }
        }

        if (g_strrstr (line, findmachine) && g_strrstr (line, finddevice)) {
            // <machine name="jvs13551" sourcefile="src/mame/machine/jvs13551.cpp" isdevice="yes" runnable="no">
            machineDevice = TRUE;
            gchar *name = g_strrstr (line, findmachine);
            if (name) {
                gchar **lineVec = g_strsplit (name, "\"", -1);

                if (retname) {
                    g_free (retname);
                    retname = NULL;
                }
                retname = g_strdup (lineVec[1]);
                g_strfreev (lineVec);
            }
        }
        g_free (line);
    }

    pclose (file);
    g_free (cmdLine);

    if (retname) {
        g_free (retname);
    }

    if (namelist) {
        gchar** retvec = g_strsplit (namelist, "|", -1);
        g_free (namelist);

        return retvec;
    } else {

        return NULL;
    }

}

gboolean
mame_needChd (const gchar* romName)
{
    struct inforom_info *info = mame_getInfoRom (romName);

    gboolean needChd = info->chd;

    mame_freeInfoRom (info);

    return needChd;
}

struct inforom_info*
mame_getInfoRom (const gchar* romName)
{
    // description
    const gchar *TAG_DESCRIPTION = "<description>";
    // chd
    const gchar *TAG_CHD = "<disk name=\"";
    // src
    const gchar *TAG_SRC = "sourcefile=\"";
    // romof
    const gchar *TAG_ROMOF = "romof=\"";
    // year
    const gchar *TAG_YEAR = "<year>";
    // manufacturer
    const gchar *TAG_MANUFACTURER = "<manufacturer>";

    gboolean foundMachine = FALSE;
    gchar *romof   = NULL;
    gchar *srcfile = NULL;
    gchar *description = NULL;
    gchar *year = NULL;
    gchar *manufacturer = NULL;
    gboolean needChd = FALSE;

    const int MAXSIZE = 255;
    char buf[MAXSIZE];

    gchar **lineVec = NULL;

    gchar *cmdLine = g_strdup_printf ("%s -lx %s", MAME_EXE, romName);
    FILE *file = popen (cmdLine, "r");


    // romof
    gchar *machineTag = g_strdup_printf ("<machine name=\"%s\"", romName);

    while (fgets (buf, MAXSIZE - 1, file)) {

        gchar *line = g_strndup (buf, strlen (buf) - 1);
        if (foundMachine) {

            // description
            if (!description) {
                // now searching for description
                gchar* tagdesc = g_strrstr (line, TAG_DESCRIPTION);
                if (tagdesc) {
                    lineVec = g_strsplit (tagdesc, ">", -1);
                    description = g_strndup (lineVec[1], strlen (lineVec[1]) - strlen (TAG_DESCRIPTION));
                    g_strfreev (lineVec);
                }
            }

            // year
            if (!year) {
                // now searching for year
                gchar* tagyear = g_strrstr (line, TAG_YEAR);
                if (tagyear) {
                    lineVec = g_strsplit (tagyear, ">", -1);
                    year = g_strndup (lineVec[1], strlen (lineVec[1]) - strlen (TAG_YEAR));
                    g_strfreev (lineVec);
                }
            }

            // manufacturer
            if (!manufacturer) {
                // now searching for manufacturer
                gchar* tagmanufacturer = g_strrstr (line, TAG_MANUFACTURER);
                if (tagmanufacturer) {
                    lineVec = g_strsplit (tagmanufacturer, ">", -1);
                    manufacturer = g_strndup (lineVec[1], strlen (lineVec[1]) - strlen (TAG_MANUFACTURER));
                    g_strfreev (lineVec);
                }
            }

        }

        if (g_strrstr (line, machineTag)) {
            foundMachine = TRUE;

            // srcfile
            if (!srcfile) {
                // now searching for src
                gchar* tagsrc = g_strrstr (line, TAG_SRC);
                if (tagsrc) {
                    lineVec = g_strsplit (tagsrc, "\"", -1);
                    srcfile = g_strdup (lineVec[1]);
                    g_strfreev (lineVec);
                }
            }

            // romof
            if (!romof) {
                // now searching for romof
                gchar* tagrom = g_strrstr (line, TAG_ROMOF);
                if (tagrom) {
                    lineVec = g_strsplit (tagrom, "\"", -1);
                    romof = g_strdup (lineVec[1]);
                    g_strfreev (lineVec);
                }
            }
        }
        g_free (line);

        // Chd
        if (g_strrstr (buf, TAG_CHD)) {
             needChd = TRUE;
        }
    }

    pclose (file);

    struct inforom_info *info = inforom_build (romName, description, manufacturer, year, romof, srcfile, needChd);

    g_free (cmdLine);

    g_free (machineTag);
    g_free (romof);
    g_free (srcfile);
    g_free (description);
    g_free (year);
    g_free (manufacturer);

    return info;
}

void
mame_freeInfoRom (struct inforom_info *info)
{
    inforom_free (info);
}

