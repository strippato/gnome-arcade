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


/* joy.h */
#ifndef JOY_H
#define JOY_H

struct Tjoy {
	gchar           *name;  // name
	struct libevdev *dev;   // /etc/device
	GIOChannel      *gio;   // g_io channel
	guint            watch; // watch (event id)

	// joy controls
	gint MAX_ABS_X;
	gint MIN_ABS_X;

	gint MAX_ABS_Y;
	gint MIN_ABS_Y;

	// autorepeat X, Y
	gint64 utimeX;
	gint64 utimeY;
	gboolean callback;

	// move
	gboolean up;
	gboolean down;
	gboolean left;
	gboolean right;
};

extern GList *joy_list;
extern guint  joy_count;

void joy_init  (void);
void joy_free  (void);
gboolean joy_event (void);
void joy_debug (struct Tjoy *item);
gboolean joy_debugFull (void);



#endif

