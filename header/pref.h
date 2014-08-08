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


/* pref.h */
#ifndef PREF_H
#define PREF_H

void pref_init (void);
void pref_free (void);
void pref_load (void);

gboolean pref_getPreferred (const char* key);
void pref_setPreferred (const char* key, gboolean value);

gint pref_getRank (const char* key);
void pref_setRank (const char* key, gint rank);

void pref_save (void);


#endif

