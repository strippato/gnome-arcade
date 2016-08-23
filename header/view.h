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


/* config.h */
#ifndef VIEW_H
#define VIEW_H

struct view_viewModel
{
	GList *romList;
	gint   romCount;

    guint  focus;
    gint   view; // drawingarea offset
};

void view_init (void);
void view_free (void);
void view_gotoDefaultView (void);
struct rom_romItem* view_getItem (struct view_viewModel *view, int numGame);


#endif

