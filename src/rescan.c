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
#include <string.h>

#include "global.h"
#include "rescan.h"
//#include "view.h"
//#include "rom.h"
//#include "ui.h"


void
rescan (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	// TODO
	g_print("rescanning...\n");

	g_print("building mame romlist\n");


	g_print("rescanning done\n");
}