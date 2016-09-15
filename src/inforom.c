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
#include <gtk/gtk.h>
#include <stdarg.h>

#include "global.h"
#include "app.h"
#include "inforom.h"

struct inforom_info*
inforom_build (const gchar *romName,
                const gchar* description,
                const gchar* manufacturer,
                const gchar* year,
                const gchar* romOf,
                const gchar* srcFile,
                gboolean chd)
{
    struct inforom_info *info = g_malloc0 (sizeof (struct inforom_info));
    info->name = g_strdup (romName);
    info->description = g_strdup (description);
    info->manufacturer = g_strdup (manufacturer);
    info->romOf = g_strdup (romOf);
    info->year = g_strdup (year);
    info->srcFile = g_strdup (srcFile);
    info->chd = chd;

    return info;
}

void
inforom_free (struct inforom_info *info)
{
    g_free (info->name);
    g_free (info->description);
    g_free (info->manufacturer);
    g_free (info->romOf);
    g_free (info->year);
    g_free (info->srcFile);
    info->chd = FALSE;

    g_free (info);
    info = NULL;
}

