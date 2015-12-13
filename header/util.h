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


/* util.h */
#ifndef UTIL_H
#define UTIL_H

gint posval (gint a);
gint negval (gint a);
gint lim (gint a, gint max);
gint max (gint a, gint b);
gint min (gint a, gint b);
gboolean pointInside (gint x, gint y, gint ax, gint ay, gint bx, gint by);
void logTimer(const gchar* message);

#endif

