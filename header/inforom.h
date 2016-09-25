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


/* inforom.h */
#ifndef INFOROM_H
#define INFOROM_H

struct inforom_info {

	gchar *name;
	gchar *description;
	gchar *manufacturer;
	gchar *year;
	gchar *romOf;
	gchar *srcFile;
	gboolean chd;
};

struct inforom_info *inforom_build (const gchar *romName, const gchar* description, const gchar* manufacturer, const gchar* year, const gchar* romOf, const gchar* srcFile,gboolean chd);
void inforom_free (struct inforom_info *info);
void inforom_show (struct rom_romItem *item);

#endif


