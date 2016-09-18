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
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <locale.h>
#include <glib/gi18n.h>

#include "global.h"
#include "app.h"
#include "view.h"
#include "ui.h"
#include "rescan.h"
#include "uipref.h"
#include "config.h"
#include "ssaver.h"

const gchar *app_authors[] = { "Strippato <strippato@gmail.com>",
                               NULL };

const gchar *app_artists[] = { "Strippato <strippato@gmail.com>",
                               "Sergio Gridelli <sergio.gridelli@gmail.com> \"The Fox\"® &amp; \"Zio Bosi\"® seal of quality logo",
                               "Sergio Gridelli <sergio.gridelli@gmail.com> \"Gallo UNSUPPORTED\"® logo",
                               NULL };

GtkApplication *app_application;

static gint app_status = 0;

GActionEntry app_entries[] = {
    { "fullscreen", ui_actionFullscreen, NULL, "false", ui_actionChangeFullscreen},
    { "preference", uipref_showDialog, NULL, NULL, NULL},
    { "rescan"    , rescan, NULL, NULL, NULL},
    { "sort"      , ui_actionSort, NULL, "true", NULL },
    { "about"     , ui_showAbout, NULL, NULL, NULL },
    { "quit"      , ui_quit     , NULL, NULL, NULL },
    { NULL }
};


static void
app_activate (GtkApplication *app)
{
    GList *list = gtk_application_get_windows (app);

    if (list) {
        gtk_window_present (GTK_WINDOW (list->data));
    }
}

static void
app_startup (void)
{
    ui_init ();
    // screen saver always disabled
    if (cfg_keyInt ("SCREENSAVER_MODE") == 2) {
        ssaver_suspend ();
    }
}


static void
app_shutdown (GtkApplication *app)
{
    // screen saver always disabled
    //if (cfg_keyInt ("SCREENSAVER_MODE") == 2) {
    //    ssaver_resume ();
    //}
    ui_free ();
}

gint
main (gint argc, gchar *argv[])
{
    setlocale (LC_CTYPE, "");
    bindtextdomain (LOCALE_PACKAGE, LOCALE_DIR);
    textdomain (LOCALE_PACKAGE);

    g_print (COLOR_RED APP_NAME " rel. " APP_VERSION COLOR_RESET"\n");
    g_print (APP_DESCRIPTION "\n\n");
    g_print (APP_COPYRIGHT " " APP_AUTHOR_EMAIL "\n");
    g_print (APP_LICENSE "\n");

    g_print (APP_GNU_WARN "\n\n");

    app_application = gtk_application_new (APP_ID, G_APPLICATION_FLAGS_NONE);

    g_signal_connect (app_application, "activate", G_CALLBACK (app_activate), NULL);
    g_signal_connect (app_application, "shutdown", G_CALLBACK (app_shutdown), NULL);
    g_signal_connect (app_application, "startup", G_CALLBACK (app_startup), NULL);

    g_application_set_default (G_APPLICATION (app_application));
    app_status = g_application_run (G_APPLICATION (app_application), argc, argv);

    g_object_unref (app_application);

    return app_status;
}

