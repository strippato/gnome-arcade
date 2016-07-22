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

#include "global.h"
#include "util.h"

inline gint
posval (gint a)
{
    return (a > 0? a: 0);
}

inline gint
negval (gint a)
{
    return (a < 0? a: 0);
}

inline gint
lim (gint a, gint max)
{
    return (a >= max? max :a);
}

inline gint
min (gint a, gint b)
{
    return (a <= b? a: b);
}

inline gint
max (gint a, gint b)
{
    return (a >= b? a: b);
}

__attribute__ ((hot))
inline gboolean
pointInside (gint x, gint y, gint ax, gint ay, gint bx, gint by)
{
    /* standard AABB test */
    if ((x < ax) || (x > bx) || (y < ay) || (y > by)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

inline void
logTimer (const gchar* message)
{
    GTimeVal time;

    g_get_current_time (&time);
    gchar *date = g_time_val_to_iso8601(&time);
    g_print ("*%s* %s\n", date, message);
    g_free (date);
}
