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
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <vlc/vlc.h>

#include "global.h"
#include "config.h"
#include "vlc.h"

static libvlc_instance_t      *vlc    = NULL;
static libvlc_media_player_t  *vlc_mp = NULL;
static libvlc_media_t         *vlc_m  = NULL;
static libvlc_event_manager_t *vlc_evm = NULL;

static char const *argv[] = { "--no-xlib" };
static int argc = sizeof (argv) / sizeof (*argv);

static const gchar *VLC_BASEURL   = "http://www.progettosnaps.net/videosnaps/mp4";
static const gchar *VLC_EXTENSION = "mp4";

void
vlc_init (void)
{
    g_assert (!vlc);
    vlc = libvlc_new (argc, argv);
    g_print ("video download %s\n", cfg_keyBool ("VIDEO_DOWNLOAD") ? SUCCESS_MSG : FAIL_MSG);
    g_assert (vlc);
}

void
vlc_free (void)
{
    g_assert (vlc);
    libvlc_release (vlc);
    vlc = NULL;
}

static gboolean
vlc_mainThread_cb (void)
{
    if (!vlc)    return FALSE;
    if (!vlc_mp) return FALSE;
    if (!vlc_m)  return FALSE;

    if (libvlc_media_player_is_playing (vlc_mp) == 1) return FALSE;

    // restart player
    libvlc_media_player_set_media (vlc_mp, vlc_m);
    libvlc_media_player_play (vlc_mp);

    // disable callback
    return FALSE;
}

static void
vlc_end_cb (void)
{
    // VLC callback: don't make any call to libvlc !
    // go to main thread
    g_idle_add ((GSourceFunc) vlc_mainThread_cb, NULL);
}

void
vlc_playVideo (const gchar* romName, GtkWidget* widget)
{
    if (vlc_m)   return;
    if (vlc_mp)  return;
    if (vlc_evm) return;

    gchar *videoUrl  = g_strdup_printf ("%s/%s.%s", VLC_BASEURL, romName, VLC_EXTENSION);
    gchar *videoFile = g_strdup_printf ("%s/%s.%s", cfg_keyStr ("VIDEO_PATH"), romName, VLC_EXTENSION);

    if (g_file_test (videoFile, G_FILE_TEST_EXISTS)) {
        vlc_m = libvlc_media_new_path (vlc, videoFile);
    } else {
        if (cfg_keyBool ("VIDEO_DOWNLOAD")) {
            vlc_m = libvlc_media_new_location (vlc, videoUrl);
        } else {
            // don't download
            g_free (videoUrl);
            g_free (videoFile);
            return;
        }
    }
    g_free (videoUrl);
    g_free (videoFile);

    vlc_mp = libvlc_media_player_new_from_media (vlc_m);
    vlc_evm = libvlc_media_player_event_manager (vlc_mp);

    // linux X11
    GdkDisplay *display = gdk_display_get_default ();
    gdk_display_sync (display);
    libvlc_media_player_set_xwindow (vlc_mp, GDK_WINDOW_XID (gtk_widget_get_window (widget)));

    if (libvlc_event_attach (vlc_evm, libvlc_MediaPlayerEndReached, (libvlc_callback_t) vlc_end_cb, NULL) != 0) {
        g_print ("Vlc: can't register callback " FAIL_MSG "\n");
    }

    libvlc_media_player_play (vlc_mp);

}

void
vlc_stopVideo (void)
{
    if (!vlc_m) return;
    if (!vlc_mp) return;

    if (libvlc_media_player_is_playing (vlc_mp) == 0) {
        libvlc_media_player_stop (vlc_mp);
    }

    if (vlc_evm) {
        libvlc_event_detach (vlc_evm, libvlc_MediaPlayerEndReached, (libvlc_callback_t) vlc_end_cb, NULL);
        vlc_evm = NULL;
    };

    libvlc_media_release (vlc_m);
    libvlc_media_player_release (vlc_mp);

    vlc_m = NULL;
    vlc_mp = NULL;
}

