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
#include "rom.h"
#include "view.h"

// TODO
 
void 
view_init (void)
{

}

void 
view_free (void)
{

}

struct view_model*
view_modelCreate (void)
{

	struct view_model* model = NULL;
	model = g_malloc (sizeof (struct view_model));
	model->curretItem = 0;

	return model;
}

void 
view_modelFree (struct view_model* model)
{
    model->curretItem = 0;

    g_list_free (model->romList);
    model->romList = NULL;

    g_free (model);
    model = NULL;
}


void 
view_modelAdd (struct view_model* model, struct rom_romItem* item)
{
	model->romList = g_list_insert (model->romList, item, 0);
}

static void
view_test (void)
{

	struct rom_romItem* item = NULL;

	view_init ();

	struct view_model* myModel = view_modelCreate ();

	item = rom_getItem (1);
	view_modelAdd (myModel, item);

	view_modelFree (myModel);

	view_free ();

}

