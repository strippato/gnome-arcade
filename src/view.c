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
#include "view.h"
#include "rom.h"
#include "ui.h"


static struct view_viewModel* modelUser; // user set
static struct view_viewModel* modelPref; // user favourite rom

static void
view_modelAdd (struct view_viewModel* model, struct rom_romItem* item)
{
	model->romList = g_list_insert (model->romList, item, -1);
	model->romCount++;
}

static struct view_viewModel*
view_modelCreate (void)
{

	struct view_viewModel* model = NULL;
	model = g_malloc (sizeof (struct view_viewModel));
	model->romList = NULL;
	model->romCount = 0;
	model->focus = 0;
	model->view = 0;

	return model;
}

void
view_init (void)
{
	g_print ("creating model ");
	modelUser = view_modelCreate (); // user romset
	modelPref = view_modelCreate (); // favorite rom

    for (GList *l = rom_romList; l != NULL; l = l->next) {
        struct rom_romItem *item = l->data;

        // only parent
        if (rom_isParent (item->name)) {
	        if (item->romFound) {
	        	view_modelAdd (modelUser, item);
	        }

	        if (item->pref) {
	        	view_modelAdd (modelPref, item);
	        }
        }
    }

	g_print (SUCCESS_MSG "\n");

	ui_setDefaultView (modelUser);
}

static void
view_modelFree (struct view_viewModel* model)
{
	model->romCount = 0;
	model->focus = 0;
	model->view = 0;

    g_list_free (model->romList);
    model->romList = NULL;

    g_free (model);
    model = NULL;
}

void
view_free (void)
{
	view_modelFree (modelUser);
	view_modelFree (modelPref);
}


inline struct rom_romItem*
view_getItem (struct view_viewModel *view, int numGame)
{
    g_assert (numGame < view->romCount);

    return g_list_nth_data (view->romList, numGame);
}

void
view_gotoDefaultView (void)
{
	ui_setView (modelUser);
}

//FIXME
// void
// view_test1 (void)
// {
// 	ui_setView (modelUser);
// }

// void
// view_test2 (void)
// {
// 	ui_setView (modelPref);
// }
