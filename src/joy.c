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

#include <string.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <libevdev/libevdev.h>

#include "global.h"
#include "rom.h"
#include "view.h"
#include "ui.h"
#include "config.h"
#include "pref.h"
#include "joy.h"

#define JOY_DEV 	"/dev/input/by-path/"
#define JOY_PATTERN "-event-joystick"

#define JOY_AUTO_REPEAT_TIMEOUT 500 // 500 ms
#define JOY_AUTO_REPEAT_RATE    120 // 120 ms


GList *joy_list = NULL;
guint  joy_count;

// forward decl
gboolean joy_autoRepeat (gpointer item);

/*
int
main (int argc, char* argv[])
{
	joy_init ();

	GList *l = g_list_first (joy_list);
	struct Tjoy *itm = l->data;
	while (TRUE) {
		joy_debug (itm);
	}

	joy_free ();

	return 0;
}
*/

void
joy_init (void)
{
	struct dirent *dent;

	joy_count = 0;

	if (!cfg_keyBool ("JOY_ENABLED")) {
		g_print ("joystick is disabled %s\n", SUCCESS_MSG);
		return;
	}

	g_print ("searching for joystick...\n");
	DIR *dir = opendir (JOY_DEV);
	if (dir) {
		while ((dent = readdir (dir))) {
			// search for *-event-joystick
        	if (g_str_has_suffix (dent->d_name, JOY_PATTERN)) {

        		// device name
        		gchar *jname = g_strdup_printf ("%s%s", JOY_DEV, dent->d_name);

       		 	int fd = open (jname, O_RDONLY | O_NONBLOCK);

				if (fd >= 0) {
			 		// fd is valid

					GIOChannel *gio = g_io_channel_unix_new (fd);
					if (gio) {
						struct libevdev *dev = NULL;

					 	int rc = libevdev_new_from_fd (fd, &dev);
					 	if (rc >= 0) {
				 			// rc is valid
						 	g_print ("found \"%s\" on bus %#x vendor %#x product %#x\n",
					 			libevdev_get_name (dev),
								libevdev_get_id_bustype (dev),
								libevdev_get_id_vendor (dev),
								libevdev_get_id_product (dev));

					 		struct Tjoy* jitem = g_malloc0 (sizeof (struct Tjoy));

							jitem->name = jname;
							jitem->dev  = dev;
					 		jitem->gio  = gio;

							jitem->MAX_ABS_X = libevdev_get_abs_maximum (jitem->dev, ABS_X);
							jitem->MIN_ABS_X = libevdev_get_abs_minimum (jitem->dev, ABS_X);

							jitem->MAX_ABS_Y = libevdev_get_abs_maximum (jitem->dev, ABS_Y);
							jitem->MIN_ABS_Y = libevdev_get_abs_minimum (jitem->dev, ABS_Y);

					 		jitem->watch = g_io_add_watch (jitem->gio, G_IO_IN, (GIOFunc) joy_event, NULL);
					 		jitem->utimeX = 0;
					 		jitem->utimeY = 0;

							jitem->up    = FALSE;
							jitem->down  = FALSE;
							jitem->left  = FALSE;
							jitem->right = FALSE;
							jitem->callback = FALSE;

					 		g_io_channel_unref (jitem->gio);

							joy_list = g_list_append (joy_list, jitem);
							joy_count++;

					 	} else {
							g_print ("can't create joypad evevt device %s\n", FAIL_MSG);
					 		libevdev_free (dev);
							g_free (jname);
							close (fd);
					 	}
					} else {
				        g_print ("can't create GIO channel %s\n", FAIL_MSG);
						g_free (jname);
					}
				} else {
					g_print ("can't open joypad device %s\n", FAIL_MSG);
					g_free (jname);
				}
            }
        }

		closedir (dir);

		if (joy_count > 0) {
			g_print ("%i joystick(s) found %s\n", joy_count, SUCCESS_MSG);
		} else {
			g_print ("no joystick found %s\n", FAIL_MSG);
		}
	} else {
		g_print ("no joystick found %s\n", FAIL_MSG);
	}
}

static void
joy_listFreeItem (struct Tjoy *item)
{
	if (!cfg_keyBool ("JOY_ENABLED")) return;

	g_io_channel_shutdown (item->gio, TRUE, NULL);
	close (g_io_channel_unix_get_fd (item->gio));

	//g_io_channel_unref (item->gio); // item->gio is owned by watch, unref is useless in this case
	g_source_remove (item->watch);  // this also unref item->gio

	g_free (item->name);
	libevdev_free(item->dev);
    g_free (item);

    item = NULL;
}

void
joy_free (void)
{
	if (!cfg_keyBool ("JOY_ENABLED")) return;

    g_list_foreach (joy_list, (GFunc) joy_listFreeItem, NULL);
    g_list_free (joy_list);

	joy_list = NULL;
	joy_count = 0;
}

void
joy_debug (struct Tjoy *item)
{
	struct input_event ev;
 	do {
        int rc = libevdev_next_event (item->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0) {
			g_print ("Event: %s %s %d\n",
            	libevdev_event_type_get_name (ev.type),
                libevdev_event_code_get_name (ev.type, ev.code),
            	ev.value);
		}
 	} while (libevdev_has_event_pending (item->dev) != 0);
}

gboolean
joy_debugFull (void)
{
  	GList *iter;
  	for (iter = joy_list; iter != NULL; iter = g_list_next (iter)) {
		g_print ("*joy[%s]*\n", ((struct Tjoy *)(iter->data))->name);
		joy_debug (iter->data);
  	}
	return TRUE;

}

gboolean
joy_event (void)
{
	if (!cfg_keyBool ("JOY_ENABLED")) return FALSE;

  	for (GList *iter = joy_list; iter != NULL; iter = g_list_next (iter)) {

  		struct Tjoy *joy = iter->data;
		struct input_event ev;

	 	do {
	        int rc = libevdev_next_event (joy->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
	        if (rc == 0) {
	        	// catch EV_ABS
	        	switch (ev.type) {
	        	case EV_ABS:
	        		switch (ev.code) {
	        		case ABS_X:
	        			if (ev.value <= joy->MIN_ABS_X) {
	        				joy->left = TRUE;
							joy->right = FALSE;
	        				if (!joy->callback) {
	        					joy->callback = TRUE;
		        				joy->utimeX = g_get_monotonic_time ();
        						g_timeout_add (JOY_AUTO_REPEAT_TIMEOUT, (GSourceFunc) joy_autoRepeat, joy);
        					}
    	    				ui_cmdLeft ();
	        			} else if (ev.value >= joy->MAX_ABS_X) {
	        				joy->right = TRUE;
							joy->left  = FALSE;
	        				if (!joy->callback) {
	        					joy->callback = TRUE;
		        				joy->utimeX = g_get_monotonic_time ();
        						g_timeout_add (JOY_AUTO_REPEAT_TIMEOUT, (GSourceFunc) joy_autoRepeat, joy);
        					}
    	    				ui_cmdRight ();
	        			} else {
	        				joy->utimeX = 0;
							joy->left  = FALSE;
							joy->right = FALSE;
	        			}
	        			break;

	        		case ABS_Y:
	        			if (ev.value <= joy->MIN_ABS_Y) {
	        				joy->up = TRUE;
							joy->down  = FALSE;
	        				if (!joy->callback) {
	        					joy->callback = TRUE;
	        					joy->utimeY = g_get_monotonic_time ();
        						g_timeout_add (JOY_AUTO_REPEAT_TIMEOUT, (GSourceFunc) joy_autoRepeat, joy);
        					}
							ui_cmdUp ();
	        			} else if (ev.value >= joy->MAX_ABS_Y) {
	        				joy->down = TRUE;
							joy->up  = FALSE;
	        				if (!joy->callback) {
	        					joy->callback = TRUE;
		        				joy->utimeY = g_get_monotonic_time ();
        						g_timeout_add (JOY_AUTO_REPEAT_TIMEOUT, (GSourceFunc) joy_autoRepeat, joy);
        					}
							ui_cmdDown ();
	        			} else {
	        				joy->utimeY = 0;
							joy->up    = FALSE;
							joy->down  = FALSE;
	        			}
	        			break;

	        		case ABS_HAT0X:
	        			if (ev.value <= -1) {
	        				joy->left = TRUE;
							joy->right = FALSE;
	        				if (!joy->callback) {
	        					joy->callback = TRUE;
	        					joy->utimeX = g_get_monotonic_time ();
        						g_timeout_add (JOY_AUTO_REPEAT_TIMEOUT, (GSourceFunc) joy_autoRepeat, joy);
        					}
    	    				ui_cmdLeft ();
	        			} else if (ev.value >= 1) {
	        				joy->right = TRUE;
							joy->left  = FALSE;
	        				if (!joy->callback) {
	        					joy->callback = TRUE;
	        					joy->utimeX = g_get_monotonic_time ();
        						g_timeout_add (JOY_AUTO_REPEAT_TIMEOUT, (GSourceFunc) joy_autoRepeat, joy);
        					}
							ui_cmdRight ();
						} else {
	        				joy->utimeX = 0;
							joy->left  = FALSE;
							joy->right = FALSE;
						}
	        			break;

	        		case ABS_HAT0Y:
	        			if (ev.value <= -1) {
	        				joy->up = TRUE;
							joy->down  = FALSE;
	        				if (!joy->callback) {
	        					joy->callback = TRUE;
	        					joy->utimeY = g_get_monotonic_time ();
        						g_timeout_add (JOY_AUTO_REPEAT_TIMEOUT, (GSourceFunc) joy_autoRepeat, joy);
        					}
        					ui_cmdUp ();
	        			} else if (ev.value >= 1) {
	        				joy->down = TRUE;
							joy->up  = FALSE;
	        				if (!joy->callback) {
	        					joy->callback = TRUE;
		        				joy->utimeY = g_get_monotonic_time ();
        						g_timeout_add (JOY_AUTO_REPEAT_TIMEOUT, (GSourceFunc) joy_autoRepeat, joy);
        					}
	        				ui_cmdDown ();
	        			} else {
	        				joy->utimeY = 0;
							joy->up    = FALSE;
							joy->down  = FALSE;
	        			}
	        			break;

	        		default:
	        			break;
	        		}

	        		break;
    			case EV_KEY:
	        		switch (ev.code) {

    				case BTN_THUMB2: // rank++
	        			if (ev.value == 1) {
	        				ui_cmdRankUp ();
	        				pref_save ();
	        			}
    					break;

	        		case BTN_THUMB: // rank--
	        			if (ev.value == 1) {
	        				ui_cmdRankDown ();
	        				pref_save ();
	        			}
    					break;

	        		case BTN_TOP: // pref
	        			if (ev.value == 1) {
	        				ui_cmdPreference ();
	        				pref_save ();
	        			}
	        			break;

	        		case BTN_TRIGGER: // play
	        			if (ev.value == 1) {
	        				if (!ui_inSelectState ()) {
	        					g_idle_add ((GSourceFunc) ui_cmdPlay, NULL);
	        				}
	        			}
	        			break;

	        		default:
	        			break;
	        		}

    				break;
	        	default:
	        		break;
	        	}
        		/*
				g_print ("Event: %s %s %d\n",
	            	libevdev_event_type_get_name (ev.type),
	                libevdev_event_code_get_name (ev.type, ev.code),
	            	ev.value);
	            /*/
			}
	 	} while (libevdev_has_event_pending (joy->dev) != 0);

  	}
	return TRUE;
}


gboolean
joy_autoRepeat (gpointer item)
{
	struct Tjoy *itm = item;
	gboolean repeat = FALSE;

	if (itm->utimeX > 0) {
		if (g_get_monotonic_time () - itm->utimeX >= (JOY_AUTO_REPEAT_TIMEOUT * 1000)) {
			if (itm->left)  ui_cmdLeft ();
			if (itm->right) ui_cmdRight ();

			if (itm->left || itm->right) repeat = TRUE;
		}
	}

	if (itm->utimeY > 0) {
		if (g_get_monotonic_time () - itm->utimeY >= (JOY_AUTO_REPEAT_TIMEOUT * 1000)) {
			if (itm->up)    ui_cmdUp ();
			if (itm->down)  ui_cmdDown ();

			if (itm->up || itm->down) repeat = TRUE;
		}
	}

	if (repeat) {
		g_timeout_add (JOY_AUTO_REPEAT_RATE, (GSourceFunc) joy_autoRepeat, item);
	} else {
		itm->callback = FALSE;
	}

	return G_SOURCE_REMOVE;
}

