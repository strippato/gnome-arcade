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
#include "vlc.h"

static libvlc_instance_t     *vlc    = NULL;
static libvlc_media_player_t *vlc_mp = NULL;
static libvlc_media_t        *vlc_m  = NULL;

static char const *argv[] = { "--no-xlib" };
static int argc = sizeof (argv) / sizeof (*argv);

static const gchar *VLC_BASEURL   = "http://www.progettosnaps.net/videosnaps/mp4/";
static const gchar *VLC_EXTENSION = ".mp4";

void
vlc_init (void)
{
    g_assert (!vlc);

    vlc = libvlc_new (argc, argv);
}

void
vlc_free (void)
{
    g_assert (vlc);

    libvlc_release (vlc);
    vlc = NULL;
}

void
vlc_playVideo (const gchar* romName, GtkWidget* widget)
{
    if (vlc_m) return;
    if (vlc_mp) return;

    gchar *videoUrl = g_strdup_printf ("%s%s%s", VLC_BASEURL, romName, VLC_EXTENSION);

    vlc_m = libvlc_media_new_location (vlc, videoUrl);
    vlc_mp = libvlc_media_player_new_from_media (vlc_m);

    //char *title = libvlc_media_get_meta (vlc_m, libvlc_meta_Title);
    //g_print ("vlc title:%s url:%s\n", title, videoUrl);

    libvlc_media_player_set_xwindow (vlc_mp, GDK_WINDOW_XID (gtk_widget_get_window (widget)));

    libvlc_media_player_play (vlc_mp);

    libvlc_media_release (vlc_m);
    vlc_m = NULL;

    g_free (videoUrl);
}

void
vlc_stopVideo (void)
{
    if (!vlc_mp) return;

    libvlc_media_player_stop (vlc_mp);
    libvlc_media_player_release (vlc_mp);
    vlc_mp = NULL;
}
