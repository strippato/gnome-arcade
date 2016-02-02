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
#include <gdk/gdkx.h>
#include <string.h>

#include "global.h"
#include "view.h"
#include "rom.h"
#include "app.h"
#include "view.h"
#include "ui.h"
#include "mame.h"
#include "util.h"
#include "config.h"
#include "pref.h"
#include "www.h"
#include "joy.h"
#include "ssaver.h"

#define TILE_MIN_SIZE       150
#define TILE_W_BORDER_MIN   24
#define TILE_H_BORDER       35

#define TILE_COLOR_FOCUS    0.05, 1.00, 0.90
#define TILE_COLOR_SHADOW   0.15, 0.15, 0.15
#define TILE_COLOR_BORDER   0.70, 0.70, 0.70

#define TILE_SHADOW_SIZE    4.5
#define TILE_BORDER_SIZE    1.0
#define TILE_FOCUS_SIZE     3.0

#define TILE_MOUSEOVER_SIZE  1.5
#define TILE_COLOR_MOUSEOVER 1.0, 1.0, 1.0
#define EMBLEM_PADDING       4
//
#define UI_OFFSET_Y         10
#define UI_SCROLL_STEP      50

// muhahah "comic sans MS"
#define TEXT_FONT               "sans-serif"
#define TEXT_SIZE               11
#define TEXT_OFFSET             20
#define TEXT_FONT_COLOR         0.75, 0.75, 0.75
#define TEXT_FONT_COLOR_FOCUS   1.0, 1.0, 1.0

/* layout

    +----------------------Drawing Area------------------------------..
    |                  ^
    |                  |UI_OFFSET_Y
    |<-TILE_W_BORDER-->+<-----TILE_W---------->+                  +--..
    |/TILE_W_BORDER_MIN^                       |                  |
    |                  |TILE_H                 |                  |
    |                  |                       |<-TILE_W_BORDER-->|
    |                  |                       |/TILE_W_BORDER_MIN|
    |                  +-----------------------+                  +--..
    |                  ^
    |                  |TILE_H_BORDER
    |                  |
    |                  |
    |<-TILE_W_BORDER-->+<-----TILE_W---------->+                  +--..
    |/TILE_W_BORDER_MIN^                       |                  |
    |                  |TILE_H                 |                  |
    |                  |                       |<-TILE_W_BORDER-->|
    |                  |                       |/TILE_W_BORDER_MIN|
    |                  +-----------------------+                  +--..
    |
    |
    +----------------------------------------------------------------..

*/

static GtkAdjustment *ui_adjust  = NULL;

static GtkWidget *ui_window      = NULL;
static GtkWidget *ui_drawingArea = NULL;
static GtkWidget *ui_scrollBar   = NULL;
static GtkWidget *ui_playBtn     = NULL;
static GtkWidget *ui_tbSelection = NULL;
static GtkWidget *ui_headerBar   = NULL;
static GtkWidget *ui_infobar     = NULL;

static GdkPixbuf *ui_selRankOn  = NULL;
static GdkPixbuf *ui_selRankOff = NULL;

static GdkPixbuf *ui_selPrefOn  = NULL;
static GdkPixbuf *ui_selPrefOff = NULL;

// current model
static struct view_viewModel *ui_viewModel = NULL;

static guint ui_mouseOver = -1;
static guint ui_mouseOverOld = 0;

static cairo_font_face_t *ui_tileFont = NULL;

static GError *gerror = NULL;
static GdkPixbuf *ui_aboutLogo = NULL;

// forward decl
static void ui_prefManager (gdouble x, gdouble y);
static void ui_rankManager (gdouble x, gdouble y);

__attribute__ ((hot))
static inline void
ui_repaint (void)
{
    /* redraw drawing area */
    gtk_widget_queue_draw (GTK_WIDGET (ui_drawingArea));
    //gdk_window_process_all_updates();
    //while (gtk_events_pending ())
    //        gtk_main_iteration ();
}

gboolean
ui_inSelectState (void)
{
   return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui_tbSelection));
}

static void
ui_headerBarRestore (void)
{
    gtk_header_bar_set_title (GTK_HEADER_BAR (ui_headerBar), APP_NAME);
    gtk_header_bar_set_subtitle (GTK_HEADER_BAR (ui_headerBar), APP_DESCRIPTION);
}

static void
ui_headerBarShowInfo (int romIndex)
{
    struct rom_romItem *item = view_getItem (ui_viewModel, romIndex);

    gchar *subTitle = NULL;

    const gchar *desc = rom_getItemDescription (item);
    const gchar *name = rom_getItemName (item);
    guint played = rom_getItemNPlay (item);

    switch (played) {
    case 0:
        subTitle = g_strdup_printf ("%s never played", name);
        break;
    case 1:
        subTitle = g_strdup_printf ("%s played 1 time", name);
        break;
    default:
        subTitle = g_strdup_printf ("%s played %i times", name, played);
        break;
    }

    gtk_header_bar_set_title (GTK_HEADER_BAR (ui_headerBar), desc);
    gtk_header_bar_set_subtitle (GTK_HEADER_BAR (ui_headerBar), subTitle);
    g_free (subTitle);
}

static void
ui_preference_cb (void)
{
    if (ui_inSelectState ()) {
        struct rom_romItem *item = view_getItem (ui_viewModel, ui_viewModel->focus);
        rom_setItemPref (item, !rom_getItemPref (item));
        pref_setPreferred (rom_getItemName (item), rom_getItemPref (item));
        ui_repaint ();
    }
}

static void
ui_rank_cb (gint i)
{
    struct rom_romItem *item = view_getItem (ui_viewModel, ui_viewModel->focus);

    gint oldRank = rom_getItemRank (item);

    if (i == oldRank) {
        rom_setItemRank (item, --i);
    } else {
        rom_setItemRank (item, i);
    }
    pref_setRank (rom_getItemName (item), rom_getItemRank (item));
    ui_repaint ();
}

static void
ui_select_cb (void)
{
    GtkStyleContext *context = gtk_widget_get_style_context (ui_headerBar);

    if (ui_inSelectState ()) {
        gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (ui_headerBar), FALSE);
        gtk_widget_hide (ui_playBtn);
        gtk_style_context_add_class (context, "selection-mode");
        ui_setPlayBtnState (FALSE);
        ui_headerBarShowInfo (ui_viewModel->focus);
    } else {
        gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (ui_headerBar), TRUE);
        gtk_widget_show (ui_playBtn);
        gtk_style_context_remove_class (context, "selection-mode");
        ui_setPlayBtnState (TRUE);
        ui_headerBarRestore ();
        pref_save ();
    }
    gtk_widget_reset_style (ui_headerBar);
    ui_repaint ();
}

inline static gint
ui_itemOnRowMax (gint width)
{
    return (width - TILE_SHADOW_SIZE) / (ui_tileSize_W + TILE_W_BORDER_MIN);
}

inline static gint
ui_getTileWBorderSize (void)
{
    if (cfg_keyBool ("TILE_BORDER_DYNAMIC")) {
        gint pageWide = gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea));
        gint item = ui_itemOnRowMax (pageWide);
        gint border;

        if (ui_viewModel->romCount <= item) {
            border = (pageWide - (ui_viewModel->romCount * ui_tileSize_W)) / (ui_viewModel->romCount + 1);
        } else {
            border = (pageWide - (item * ui_tileSize_W)) / (item + 1);
        }

        border = border < TILE_W_BORDER_MIN ? TILE_W_BORDER_MIN : border;

        return border;
    } else {

        return TILE_W_BORDER_MIN;
    }
}

inline static gint
ui_itemOnRow (gint width)
{
    gint tileBorder = ui_getTileWBorderSize ();

    return (width - TILE_SHADOW_SIZE) / (ui_tileSize_W + tileBorder);
}

static gboolean
ui_playClicked (void)
{
    if (mame_isRunning ()) return FALSE;

    if (ui_viewModel->romCount > 0) {
        struct rom_romItem  *itm = view_getItem (ui_viewModel, ui_viewModel->focus);

        if (mame_playGame (itm)) {
            guint nplay = rom_getItemNPlay (itm) + 1;
            rom_setItemNPlay (itm, nplay);
            pref_setNPlay (rom_getItemName (itm), nplay);
            pref_save ();
        };

    } else {
        g_print ("romlist is empty %s\n", FAIL_MSG);
    }
    return TRUE;
}

static void
ui_focusAt (int index)
{
    // no rom found
    if (ui_viewModel->romCount <= 0) {
        ui_viewModel->romCount = 0;
        return;
    }

    ui_viewModel->focus = index;

    ui_viewModel->focus = lim (ui_viewModel->focus, ui_viewModel->romCount - 1);
    ui_viewModel->focus = posval (ui_viewModel->focus);

    if (ui_inSelectState ()) {
        ui_headerBarShowInfo (ui_viewModel->focus);
    }

    /*
    g_print ("\n Rom %s\n", rom_getItemName (view_getItem (ui_viewModel, ui_viewModel->focus)));
    iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, rom_cloneTable);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        // do something with key and value
    (g_strcmp0(rom_getItemName (view_getItem (ui_viewModel, ui_viewModel->focus)),value) == 0) {
            g_print ("found %s %s\n", (char*) key, (char*) value);
        }

    }
    //*/

    // play button
    gchar *tips = g_strdup_printf ("Play at %s", rom_getItemDescription (view_getItem (ui_viewModel, ui_viewModel->focus)));
    gtk_widget_set_tooltip_text (ui_playBtn, tips);
    g_free (tips);

}

static void
ui_focusAdd (int step)
{
    if ((ui_viewModel->focus + step) > 0) {
        ui_focusAt (lim (ui_viewModel->focus + step, ui_viewModel->romCount));
    } else {
        ui_focusAt (0);
    }

}

static void
ui_focusPrevRow (void)
{
    gint itmOnRow = ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)));

    if (ui_viewModel->focus >= itmOnRow) {
        ui_focusAdd (-itmOnRow);
    }
}

static void
ui_focusNextRow (void)
{
    gint itmOnRow = ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)));

    if ((ui_viewModel->focus / itmOnRow) < ((ui_viewModel->romCount-1) / itmOnRow)) {
        ui_focusAdd (+itmOnRow);
    }

}

static void
ui_focusNext (void)
{
    ui_focusAdd (+1);
}

static void
ui_focusPrev (void)
{
    ui_focusAdd (-1);
}

static void
ui_drawingAreaShowItem (int item)
{
    /* follow the tile with fucus */
    gint rowNum  = ((item) / ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea))));

    gint uiTop = ui_viewModel->view;
    gint uiBottom = uiTop + gtk_widget_get_allocated_height (GTK_WIDGET (ui_drawingArea));

    gint itemTop = rowNum * (ui_tileSize_H + TILE_H_BORDER) + UI_OFFSET_Y;
    gint itemBottom = itemTop + ui_tileSize_H + TILE_H_BORDER;

    if (rowNum == 0) {
        gtk_adjustment_set_value (GTK_ADJUSTMENT (ui_adjust), 0);
    } else if (itemTop < uiTop) {
        gtk_adjustment_set_value (GTK_ADJUSTMENT (ui_adjust), itemTop - TILE_FOCUS_SIZE);
    } else if (itemBottom > uiBottom) {
        gtk_adjustment_set_value (GTK_ADJUSTMENT (ui_adjust), uiTop + (itemBottom - uiBottom));
    }
}

static gint
ui_getTileIdx (gdouble x, gdouble y, gboolean hitOnlyPixmap)
{
    // no rom loaded
    if (ui_viewModel->romCount <= 0) return -1;

    gint tileBorder = ui_getTileWBorderSize ();
    gint colNum = x / (ui_tileSize_W + tileBorder);
    gint maxCol = ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea))) - 1;
    colNum = lim (colNum, maxCol);
    gint rowNum = (ui_viewModel->view + y - UI_OFFSET_Y) / (ui_tileSize_H + TILE_H_BORDER);
    gint maxRow = (ui_viewModel->romCount / ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)))) - 1;
    maxRow += (ui_viewModel->romCount % ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)))) == 0 ? 0 : 1;
    rowNum = lim (rowNum, maxRow);
    gint idx = colNum + rowNum * ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)));

    if (idx < ui_viewModel->romCount) {
        if (hitOnlyPixmap) {
            /* real collision? */
            const GdkPixbuf* pix = rom_getItemTile (view_getItem (ui_viewModel, idx));

            /* fallback to noimage pixbuf */
            if (!pix) pix = rom_tileNoImage;

            g_assert (pix);

            gint pixbuf_w = gdk_pixbuf_get_width (pix);
            gint pixbuf_h = gdk_pixbuf_get_height (pix);

            gint itemTop = rowNum * (ui_tileSize_H + TILE_H_BORDER) + UI_OFFSET_Y + (ui_tileSize_H - pixbuf_h);
            gint itemLeft = colNum * (ui_tileSize_W + tileBorder) + tileBorder + (ui_tileSize_W - pixbuf_w) / 2;

            gint baseTileX = itemLeft;
            gint baseTileY = itemTop;

            if (!pointInside (x, ui_viewModel->view + y, baseTileX, baseTileY, baseTileX + pixbuf_w, baseTileY + pixbuf_h)) {
                idx = -1;
            }
        }
    } else {
        /* out of tile */
        idx = -1;
    }

    return idx;
}

static gboolean
ui_isFullscreen (void)
{
    GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (ui_window));

    gboolean fullscreen = (gdk_window_get_state (GDK_WINDOW (win)) & GDK_WINDOW_STATE_FULLSCREEN) != 0;

    return fullscreen;
}

static gboolean
ui_drawingAreaConfigureEvent (void)
{
    // no rom found
    if (ui_viewModel->romCount <= 0) {
        gtk_widget_hide (GTK_WIDGET (ui_scrollBar));
        return FALSE;
    }

    int newItemOnRow = ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)));

    gint rowNum  = posval (ui_viewModel->romCount - 1) / newItemOnRow;
    gint upper = (rowNum + 1) * (ui_tileSize_H + TILE_H_BORDER) + UI_OFFSET_Y;

    gint pageHeight = gtk_widget_get_allocated_height (GTK_WIDGET (ui_drawingArea));

    gdouble position = gtk_adjustment_get_value (GTK_ADJUSTMENT (ui_adjust));

    gtk_adjustment_configure (GTK_ADJUSTMENT (ui_adjust), position, 0, upper, UI_SCROLL_STEP, pageHeight, pageHeight);

    if (pageHeight >= upper) {
        gtk_widget_hide (GTK_WIDGET (ui_scrollBar));
    } else {
        gtk_widget_show (GTK_WIDGET (ui_scrollBar));
    }

    return FALSE;
}

static gboolean
ui_barValueChanged (GtkWidget *widget, GtkAdjustment *adj)
{
    ui_viewModel->view = gtk_adjustment_get_value (GTK_ADJUSTMENT (adj));

    /* tooltips */
    gint width  = gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea));
    gint row = posval (ui_viewModel->view - UI_OFFSET_Y) / (ui_tileSize_H + TILE_H_BORDER);
    gint idx = row * ui_itemOnRow (width);

    gtk_widget_set_tooltip_markup (ui_scrollBar, rom_getItemDesc (view_getItem (ui_viewModel, idx)));
    ui_invalidateDrawingArea ();

    return TRUE;
}

gboolean
ui_cmdPlay (void)
{
    ui_playClicked ();
    return FALSE;
}

void
ui_cmdUp (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;
    ui_focusPrevRow ();
    ui_drawingAreaShowItem (ui_viewModel->focus);
    ui_invalidateDrawingArea ();
}

void
ui_cmdDown (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;
    ui_focusNextRow ();
    ui_drawingAreaShowItem (ui_viewModel->focus);
    ui_invalidateDrawingArea ();
}

void
ui_cmdLeft (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;
    ui_focusPrev ();
    ui_drawingAreaShowItem (ui_viewModel->focus);
    ui_invalidateDrawingArea ();
}

void
ui_cmdRight (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;
    ui_focusNext ();
    ui_drawingAreaShowItem (ui_viewModel->focus);
    ui_invalidateDrawingArea ();
}

void
ui_cmdHome (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;
    ui_focusAt (0);
    ui_drawingAreaShowItem (ui_viewModel->focus);
    ui_invalidateDrawingArea ();
}

void
ui_cmdEnd (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;
    ui_focusAt (ui_viewModel->romCount - 1);
    ui_drawingAreaShowItem (ui_viewModel->focus);
    ui_invalidateDrawingArea ();
}

void
ui_cmdPreference (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;

    struct rom_romItem *item = view_getItem (ui_viewModel, ui_viewModel->focus);
    rom_setItemPref (item, !rom_getItemPref (item));
    pref_setPreferred (rom_getItemName (item), rom_getItemPref (item));
    ui_repaint ();
}

void
ui_cmdRankUp (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;

    struct rom_romItem *item = view_getItem (ui_viewModel, ui_viewModel->focus);
    gint oldRank = rom_getItemRank (item);

    rom_setItemRank (item, ++oldRank);
    pref_setRank (rom_getItemName (item), rom_getItemRank (item));
    ui_repaint ();
}

void
ui_cmdRankDown (void)
{
    if (mame_isRunning ()) return;
    if (ui_viewModel->romCount <= 0) return;

    struct rom_romItem *item = view_getItem (ui_viewModel, ui_viewModel->focus);
    gint oldRank = rom_getItemRank (item);

    rom_setItemRank (item, --oldRank);
    pref_setRank (rom_getItemName (item), rom_getItemRank (item));
    ui_repaint ();
}

static gboolean
ui_drawingAreaKeyPressEvent (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    struct rom_romItem *item;
    if (mame_isRunning ()) return FALSE;

    switch (event->keyval) {
    case GDK_KEY_Up:
        ui_cmdUp ();
        break;

    case GDK_KEY_Down:
        ui_cmdDown ();
        break;

    case GDK_KEY_Right:
        ui_cmdRight ();
        break;

    case GDK_KEY_Left:
        ui_cmdLeft ();
        break;

    case GDK_KEY_Home:
        ui_cmdHome ();
        break;

    case GDK_KEY_End:
        ui_cmdEnd ();
        break;


    case GDK_KEY_space:
        if (!ui_inSelectState ()) {
            ui_playClicked ();
        } else {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            int rnk = rom_getItemRank (item) + 1;
            if (rnk > ROM_MAXRANK) rnk = 0;
            rom_setItemRank (item, rnk);
            pref_setRank (rom_getItemName (item), rom_getItemRank (item));
            ui_invalidateDrawingArea ();
        }
        break;

    case GDK_KEY_KP_Enter:
    case GDK_KEY_Return:
        if (!ui_inSelectState ()) {
            ui_playClicked ();
        } else {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemPref (item, !rom_getItemPref (item));
            pref_setPreferred (rom_getItemName (item), rom_getItemPref (item));
            ui_invalidateDrawingArea ();
        }
        break;

    case GDK_KEY_plus:
    case GDK_KEY_KP_Add:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemRank (item, rom_getItemRank (item) + 1);
            pref_setRank (rom_getItemName (item), rom_getItemRank (item));
            ui_invalidateDrawingArea ();
        }
       break;

    case GDK_KEY_minus:
    case GDK_KEY_KP_Subtract:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemRank (item, rom_getItemRank (item) - 1);
            pref_setRank (rom_getItemName (item), rom_getItemRank (item));
            ui_invalidateDrawingArea ();
        }
        break;

    case GDK_KEY_0:
    case GDK_KEY_KP_0:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemRank (item, 0);
            pref_setRank (rom_getItemName (item), 0);
            ui_invalidateDrawingArea ();
        }
        break;

    case GDK_KEY_1:
    case GDK_KEY_KP_1:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemRank (item, 1);
            pref_setRank (rom_getItemName (item), 1);
            ui_invalidateDrawingArea ();
        }
        break;

    case GDK_KEY_2:
    case GDK_KEY_KP_2:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemRank (item, 2);
            pref_setRank (rom_getItemName (item), 2);
            ui_invalidateDrawingArea ();
        }
        break;

    case GDK_KEY_3:
    case GDK_KEY_KP_3:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemRank (item, 3);
            pref_setRank (rom_getItemName (item), 3);
            ui_invalidateDrawingArea ();
        }
        break;
/*
    case GDK_KEY_4:
    case GDK_KEY_KP_4:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemRank (item, 4);
            pref_setRank (rom_getItemName (item), 4);
            ui_invalidateDrawingArea ();
        }
        break;

    case GDK_KEY_5:
    case GDK_KEY_KP_5:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemRank (item, 5);
            pref_setRank (rom_getItemName (item), 5);
            ui_invalidateDrawingArea ();
        }
        break;
*/
    case GDK_KEY_KP_Multiply:
    case GDK_KEY_asterisk:
        if (ui_inSelectState ()) {
            item = view_getItem (ui_viewModel, ui_viewModel->focus);
            rom_setItemPref (item, !rom_getItemPref (item));
            pref_setPreferred (rom_getItemName (item), rom_getItemPref (item));
            ui_invalidateDrawingArea ();
        }
        break;

    case GDK_KEY_Page_Up:
        if (ui_viewModel->romCount <= 0) return FALSE; // non rom found
        gtk_adjustment_set_value (GTK_ADJUSTMENT (ui_adjust), gtk_adjustment_get_value (GTK_ADJUSTMENT (ui_adjust)) - gtk_adjustment_get_page_increment (GTK_ADJUSTMENT (ui_adjust)));
        break;

    case GDK_KEY_Page_Down:
        if (ui_viewModel->romCount <= 0) return FALSE; // non rom found
        gtk_adjustment_set_value (GTK_ADJUSTMENT (ui_adjust), gtk_adjustment_get_value (GTK_ADJUSTMENT (ui_adjust)) + gtk_adjustment_get_page_increment (GTK_ADJUSTMENT (ui_adjust)));
        break;

    case GDK_KEY_F11:
    /*
        if (ui_isFullscreen ()) {
            gtk_window_unfullscreen (GTK_WINDOW (ui_window));
        } else {
            gtk_window_fullscreen (GTK_WINDOW (ui_window));
        }
    */
        break;

    case GDK_KEY_Escape:
        if (ui_inSelectState ()) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ui_tbSelection), FALSE);
            break;
        }

        if (ui_isFullscreen ()) {
            g_action_group_activate_action (G_ACTION_GROUP (app_application), "fullscreen", NULL);
        }
        break;

    // FIXME TODO

    //  case GDK_KEY_KP_7:
    //      if (!ui_inSelectState ()) {
    //          view_test1 ();
    //      }
    //      break;

    //  case GDK_KEY_KP_9:
    //      if (!ui_inSelectState ()) {
    //          view_test2 ();
    //      }
    //      break;

    }
    return FALSE;
}


static gboolean
ui_drawingAreaScrollEvent (GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
    gdouble factor;

    if (mame_isRunning ()) return FALSE;

    if (event->state & GDK_CONTROL_MASK) {
        // hard
        factor = 2.0;
    } else if (event->state & GDK_SHIFT_MASK) {
        // soft
        factor = 0.5;
    } else {
        // normal
        factor = 1.0;
    }

    gdouble position = gtk_adjustment_get_value (GTK_ADJUSTMENT (ui_adjust));
    switch (event->direction) {
    case GDK_SCROLL_UP:
        gtk_adjustment_set_value (GTK_ADJUSTMENT (ui_adjust), position - factor * gtk_adjustment_get_step_increment (GTK_ADJUSTMENT (ui_adjust)));
        break;
    case GDK_SCROLL_DOWN:
        gtk_adjustment_set_value (GTK_ADJUSTMENT (ui_adjust), position + factor * gtk_adjustment_get_step_increment (GTK_ADJUSTMENT (ui_adjust)));
        break;
    default:
        // nothing to do
        break;
    }

    return TRUE;
}

__attribute__ ((hot))
static gboolean
ui_drawingAreaMotionNotifyEvent (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    if (mame_isRunning ()) return FALSE;

    ui_mouseOver = -1;
    gint tileIdx = ui_getTileIdx (event->x, event->y, TRUE);

    if (tileIdx >= 0) {
        ui_mouseOver = tileIdx;
    }

    if (tileIdx != ui_mouseOverOld) {
        ui_mouseOverOld = tileIdx;
        ui_invalidateDrawingArea ();
    }

    return TRUE;

}

static gboolean
ui_drawingAreaButtonPress (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    gint tileIdx;

    if (mame_isRunning ()) return FALSE;

    switch (event->type) {

    case GDK_2BUTTON_PRESS:
        tileIdx = ui_getTileIdx (event->x, event->y, TRUE);
        if (tileIdx >= 0) {
            ui_focusAt (tileIdx);

            if (!ui_inSelectState ()) {
                ui_drawingAreaShowItem (ui_viewModel->focus);
                ui_playClicked ();
            }
            ui_repaint ();
        }

        break;

    case GDK_BUTTON_PRESS:
        switch (event->button) {

        case 1: // left
        case 3: // right
            tileIdx = ui_getTileIdx (event->x, event->y, TRUE);
            if (tileIdx >= 0) {
                ui_focusAt (tileIdx);

                if (ui_inSelectState ()) {
                    ui_prefManager (event->x, event->y);
                    ui_rankManager (event->x, event->y);
                } else {
                    ui_drawingAreaShowItem (ui_viewModel->focus);
                }
                ui_repaint ();
            }
            break;

        case 2: // middle
            break;

        }
        break;

    default:
        break;
    }

    return TRUE;
}

__attribute__ ((hot))
gboolean
ui_drawingAreaDraw (GtkWidget *widget, cairo_t *cr)
{
    gint x, y;

    const GdkPixbuf *pix = NULL;

    if (ui_viewModel->romCount <= 0) {
        return TRUE;
    }

    gint tileBorder = ui_getTileWBorderSize ();

    gboolean centerTitle = cfg_keyBool ("TILE_TITLE_CENTERED");
    gboolean showShadow = cfg_keyBool ("TILE_SHADOW");

    gint width  = gtk_widget_get_allocated_width (GTK_WIDGET (widget));
    gint height = gtk_widget_get_allocated_height (GTK_WIDGET (widget));

    /* get right tile base */
    gint row = posval (ui_viewModel->view - UI_OFFSET_Y) / (ui_tileSize_H + TILE_H_BORDER);

    gint idx = row * ui_itemOnRow (width);

    gint ui_base;

    if (ui_viewModel->view > UI_OFFSET_Y) {
        ui_base = UI_OFFSET_Y + (ui_viewModel->view - UI_OFFSET_Y) % (ui_tileSize_H + TILE_H_BORDER);
    } else {
        ui_base = ui_viewModel->view % (ui_tileSize_H + TILE_H_BORDER);
    }


    for (y=0; -ui_base + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) <= height; ++y) {

        for (x=0; (x + 1) * (ui_tileSize_W + tileBorder) + TILE_SHADOW_SIZE <= width; ++x) {

            struct rom_romItem *item = view_getItem (ui_viewModel, idx);

            if (rom_getItemTileLoaded (item)) {
                /* cached */
                pix = rom_getItemTile (item);

                /* fallback to noimage pixbuf */
                if (!pix) pix = rom_tileNoImage;

            } else {
                if (!rom_getItemTileLoading (item)) {
                    /* pixbuf must be load */
                    rom_loadItemAsync (item);
                }
                pix = rom_tileLoading;
            }

            gint pixbuf_w = gdk_pixbuf_get_width (pix);
            gint pixbuf_h = gdk_pixbuf_get_height (pix);

            gint diffX = (ui_tileSize_W - pixbuf_w) / 2;
            gint diffY = (ui_tileSize_H - pixbuf_h) / 2;

            if (showShadow) {
                /* black shadow */
                cairo_set_line_width (cr, 1);
                cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

                /* set color for rectangle */
                cairo_set_source_rgb (cr, TILE_COLOR_SHADOW);

                /* draw the rectangle */
                cairo_rectangle (cr, diffX + tileBorder + x * (ui_tileSize_W + tileBorder) + TILE_SHADOW_SIZE, - ui_base + 2 * diffY + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) + TILE_SHADOW_SIZE, pixbuf_w, pixbuf_h);

                cairo_fill (cr);
            }

            /* border */
            cairo_set_line_width (cr, TILE_BORDER_SIZE);
            cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

            /* set color for rectangle */
            cairo_set_source_rgb (cr, TILE_COLOR_BORDER);

            /* draw the rectangle */
            cairo_rectangle (cr, diffX + tileBorder + x * (ui_tileSize_W + tileBorder) - TILE_BORDER_SIZE / 2.0, - ui_base + 2 * diffY + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) - TILE_BORDER_SIZE / 2.0, pixbuf_w + TILE_BORDER_SIZE, + pixbuf_h + TILE_BORDER_SIZE);

            /* stroke the rectangle */
            cairo_stroke (cr);

            /* pixbuf */
            gdk_cairo_set_source_pixbuf (cr, pix, diffX + tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base + 2 * diffY + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER));

            if (mame_isRunning ()) {
                if (ui_viewModel->focus != idx) {
                    cairo_set_operator (cr,CAIRO_OPERATOR_HSL_LUMINOSITY);
                } else {
                    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
                }
            } else {
                cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
            }
            cairo_paint (cr);

            cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

            /* emblem */
            if ((ui_inSelectState ()) && (ui_mouseOver == idx)) {
                /* emblem fav */
                if (rom_getItemPref (item)) {
                    gdk_cairo_set_source_pixbuf (cr, ui_selPrefOn, EMBLEM_PADDING + diffX + tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base + EMBLEM_PADDING + 2 * diffY + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER));
                } else {
                    gdk_cairo_set_source_pixbuf (cr, ui_selPrefOff, EMBLEM_PADDING + diffX + tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base + EMBLEM_PADDING + 2 * diffY + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER));
                }
                cairo_paint (cr);
                //cairo_fill (cr);

                /* emblem rank */
                for (int i = 0; i < ROM_MAXRANK; ++i) {
                    if (i < rom_getItemRank (item)) {
                        gdk_cairo_set_source_pixbuf (cr, ui_selRankOn, (i * gdk_pixbuf_get_width (ui_selRankOn)) + EMBLEM_PADDING + diffX + tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base - EMBLEM_PADDING + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) + ui_tileSize_H - gdk_pixbuf_get_height (ui_selRankOn));
                    } else {
                        gdk_cairo_set_source_pixbuf (cr, ui_selRankOff, (i * gdk_pixbuf_get_width (ui_selRankOff)) + EMBLEM_PADDING + diffX + tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base - EMBLEM_PADDING + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) + ui_tileSize_H - gdk_pixbuf_get_height (ui_selRankOff));
                    }
                    cairo_paint (cr);
                    //cairo_fill (cr);
                }

            } else {
                /* emblem fav */
                if (rom_getItemPref (item)) {
                    gdk_cairo_set_source_pixbuf (cr, rom_tileFavorite, EMBLEM_PADDING + diffX + tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base + EMBLEM_PADDING + 2 * diffY + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER));
                    cairo_paint (cr);
                    //cairo_fill (cr);
                }

                /* emblem rank */
                if (rom_getItemRank (item) > 0) {
                    for (int i = 0; i < rom_getItemRank (item); ++i) {
                        gdk_cairo_set_source_pixbuf (cr, rom_tileRank, (i * gdk_pixbuf_get_width (rom_tileRank)) + EMBLEM_PADDING + diffX + tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base - EMBLEM_PADDING + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) + ui_tileSize_H - gdk_pixbuf_get_height (rom_tileRank));
                        cairo_paint (cr);
                        //cairo_fill (cr);
                    }
                }
            }

            /* draw some text */
            cairo_set_font_face (cr, ui_tileFont);
            cairo_set_font_size (cr, TEXT_SIZE);

            if (ui_viewModel->focus == idx || ui_mouseOver == idx) {
                cairo_set_source_rgb (cr, TEXT_FONT_COLOR_FOCUS);
            } else {
                cairo_set_source_rgb (cr, TEXT_FONT_COLOR);
            }

            const gchar *desc = (ui_viewModel->focus == idx ? rom_getItemDescription (item) : rom_getItemDesc (item));

            gchar *title = g_strdup (desc);
            gint numChar = strlen (desc);

            cairo_text_extents_t extents;
            cairo_text_extents (cr, title, &extents);

            gdouble textLimit;

            if (centerTitle) {
                textLimit = ui_tileSize_W + tileBorder;
            } else {
                textLimit = ui_tileSize_W - diffX;
            }

            while ((extents.width + extents.x_bearing > textLimit) && (numChar > 1)) {
                g_free (title);
                numChar--;

                gchar* baseTitle = g_strndup (desc, numChar);
                title = g_strjoin (NULL, baseTitle, "…", NULL);
                g_free (baseTitle);

                cairo_text_extents (cr, title, &extents);
            }

            cairo_text_extents (cr, title, &extents);

            if (centerTitle) {
                cairo_move_to (cr, tileBorder + x * (ui_tileSize_W + tileBorder) + ui_tileSize_W / 2.0 - (extents.width / 2.0 + extents.x_bearing), - ui_base + TEXT_OFFSET + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) + ui_tileSize_H);
            } else {
                cairo_move_to (cr, diffX + tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base + TEXT_OFFSET + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) + ui_tileSize_H);
            }

            cairo_show_text (cr, title);

            g_free (title);
            title = NULL;

            /* focus border */
            if (ui_viewModel->focus == idx) {
                /* set the line width */

                cairo_set_line_width (cr, TILE_FOCUS_SIZE);
                cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

                /* set color for rectangle */
                cairo_set_source_rgb (cr, TILE_COLOR_FOCUS);

                /* draw the rectangle */
                cairo_rectangle (cr, diffX + tileBorder + x * (ui_tileSize_W + tileBorder) - TILE_FOCUS_SIZE / 2.0, - ui_base + 2 * diffY + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) - TILE_FOCUS_SIZE / 2.0, pixbuf_w + TILE_FOCUS_SIZE / 2.0, pixbuf_h + TILE_FOCUS_SIZE / 2.0);

                /* stroke the rectangle */
                cairo_stroke (cr);

                /* now showing... */
                if (mame_isRunning ()) {
                    gdk_cairo_set_source_pixbuf (cr, rom_tileNowShowing, tileBorder + x * (ui_tileSize_W + tileBorder), - ui_base + TEXT_OFFSET + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) + ui_tileSize_H / 2 - gdk_pixbuf_get_height (rom_tileNowShowing) + diffY*2);

                    cairo_paint (cr);
                }
            } else if (ui_mouseOver == idx) {
                /* set the line width */
                cairo_set_line_width (cr, TILE_MOUSEOVER_SIZE);
                cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

                /* set color for rectangle */
                cairo_set_source_rgb (cr, TILE_COLOR_MOUSEOVER);

                /* draw the rectangle */
                cairo_rectangle (cr, diffX + tileBorder + x * (ui_tileSize_W + tileBorder) - TILE_MOUSEOVER_SIZE / 2.0, - ui_base + 2 * diffY + UI_OFFSET_Y + y * (ui_tileSize_H + TILE_H_BORDER) - TILE_MOUSEOVER_SIZE / 2.0, pixbuf_w + TILE_MOUSEOVER_SIZE / 2.0, pixbuf_h + TILE_MOUSEOVER_SIZE / 2.0);

                /* stroke the rectangle */
                cairo_stroke (cr);
            }

            ++idx;
            if (idx >= ui_viewModel->romCount) {
                return TRUE;
            }
        }
    }
    return TRUE;
}

void
ui_showAbout (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    gtk_show_about_dialog (GTK_WINDOW (ui_window),
                         "logo", ui_aboutLogo,
                         "program-name", APP_NAME,
                         "title", APP_NAME,
                         "version", APP_VERSION,
                         "authors", app_authors,
                         "artists", app_artists,
//                         "documenters", app_documenters,
//                         "translator-credits", APP_TRANSLATORS,
                         "comments", APP_DESCRIPTION,
                         "copyright", APP_COPYRIGHT,
                         "website", APP_WEB,
                         "license-type", GTK_LICENSE_GPL_3_0,
                         NULL);
}

void
ui_quit (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
    if (ui_inSelectState ()) pref_save ();

    GApplication *app = user_data;

    g_application_quit (app);
}

void
ui_actionSort (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
    GVariant *state = g_action_get_state (G_ACTION (simple));

    g_action_change_state (G_ACTION (simple), g_variant_new_boolean (!g_variant_get_boolean (state)));
    g_variant_unref (state);

    rom_setSort (rom_getSort () == ROM_SORT_AZ ? ROM_SORT_ZA : ROM_SORT_AZ);
}

void
ui_actionFullscreen (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
    GVariant *state = g_action_get_state (G_ACTION (simple));
    g_action_change_state (G_ACTION (simple), g_variant_new_boolean (!g_variant_get_boolean (state)));
    g_variant_unref (state);
}

void
ui_actionChangeFullscreen (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
    if (g_variant_get_boolean (parameter)) {
        gtk_window_fullscreen (GTK_WINDOW (ui_window));
    } else {
        gtk_window_unfullscreen (GTK_WINDOW (ui_window));
    }
    g_simple_action_set_state (simple, parameter);
}


void
ui_init (void)
{
    const gchar* cssFile = APP_RESOURCE APP_CSS;

    ui_mouseOver = -1;
    ui_mouseOverOld = 0;

    g_print ("resource path is %s\n", APP_RESOURCE);

    /* CSS */
    GtkCssProvider* cssProvider = gtk_css_provider_new ();
    GdkDisplay* display = gdk_display_get_default ();
    GdkScreen* screen = gdk_display_get_default_screen (display);
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (cssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_css_provider_load_from_path (cssProvider, g_filename_to_utf8 (cssFile, strlen (cssFile), NULL, NULL, &gerror), NULL);
    g_object_unref (cssProvider);

    /* main window */
    ui_window = gtk_application_window_new (app_application);

    GtkBuilder *builder = gtk_builder_new ();

    g_action_map_add_action_entries (G_ACTION_MAP (app_application), app_entries, -1, app_application);

    gtk_builder_add_from_string (builder,
                               "<interface>"
                               "  <menu id='app-menu'>"
                               "    <section>"
//                               "      <item>"
//                               "        <attribute name='label' translatable='yes'>_Alphabetical order</attribute>"
//                               "        <attribute name='action'>app.sort</attribute>"
//                               "        <attribute name='accel'>&lt;Primary&gt;a</attribute>"
//                               "      </item>"
                               "      <item>"
                               "          <attribute name='label' translatable='yes'>_Fullscreen</attribute>"
                               "          <attribute name='action'>app.fullscreen</attribute>"
                               "          <attribute name='accel'>F11</attribute>"
                               "      </item>"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "          <attribute name='label' translatable='yes'>_Preferences</attribute>"
                               "          <attribute name='action'>app.preference</attribute>"
                               "        <attribute name='accel'>&lt;Primary&gt;p</attribute>"
                               "      </item>"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "          <attribute name='label' translatable='yes'>Re_scan</attribute>"
                               "          <attribute name='action'>app.rescan</attribute>"
                               "        <attribute name='accel'>&lt;Primary&gt;s</attribute>"
                               "      </item>"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name='label' translatable='yes'>Abo_ut arcade</attribute>"
                               "        <attribute name='action'>app.about</attribute>"
                               "        <attribute name='accel'>&lt;Primary&gt;u</attribute>"
                               "      </item>"
                               "      <item>"
                               "        <attribute name='label' translatable='yes'>_Quit</attribute>"
                               "        <attribute name='action'>app.quit</attribute>"
                               "        <attribute name='accel'>&lt;Primary&gt;q</attribute>"
                               "      </item>"
                               "    </section>"
                               "  </menu>"
                               "  <menu id='menubar'>"
                               "    <submenu>"
                               "      <attribute name='label' translatable='yes'>_Edit</attribute>"
                               "      <section>"
                               "        <item>"
                               "          <attribute name='label' translatable='yes'>_Copy</attribute>"
                               "          <attribute name='action'>win.copy</attribute>"
                               "          <attribute name='accel'>&lt;Primary&gt;c</attribute>"
                               "        </item>"
                               "        <item>"
                               "          <attribute name='label' translatable='yes'>_Paste</attribute>"
                               "          <attribute name='action'>win.paste</attribute>"
                               "          <attribute name='accel'>&lt;Primary&gt;v</attribute>"
                               "        </item>"
                               "      </section>"
                               "    </submenu>"
                               "  </menu>"
                               "</interface>", -1, NULL);

    gtk_application_set_app_menu (GTK_APPLICATION (app_application), G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));
    g_object_unref (builder);

    gtk_window_set_title (GTK_WINDOW (ui_window), APP_NAME);

    /* config */
    cfg_init ();

    /* create config */
    if (!cfg_configFileExist ()) {
        g_print ("config file not found!\n");
        cfg_createDefaultConfigFile ();
    }

    /* load config */
    g_assert (cfg_load ());

    ui_tileSize_W = MAX (cfg_keyInt ("TILE_SIZE_W"), TILE_MIN_SIZE);
    ui_tileSize_H = MAX (cfg_keyInt ("TILE_SIZE_H"), TILE_MIN_SIZE);

    // need extra space (1.5 * TILE_W_BORDER_MIN) for scrollbar
    gtk_window_set_default_size (GTK_WINDOW (ui_window), 4 * (ui_tileSize_W + TILE_W_BORDER_MIN) + TILE_W_BORDER_MIN + 1.5 * TILE_W_BORDER_MIN, UI_OFFSET_Y + 2.7 * (ui_tileSize_H + TILE_H_BORDER));

    gtk_window_set_position (GTK_WINDOW (ui_window), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_from_file (GTK_WINDOW (ui_window), APP_RESOURCE APP_ICON, &gerror);

    /* default icon */
    gtk_window_set_default_icon_from_file (APP_RESOURCE APP_ICON, &gerror);

    /* header bar */
    ui_headerBar = gtk_header_bar_new ();
    ui_headerBarRestore ();
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (ui_headerBar), TRUE);

    /* play button */
    ui_playBtn = gtk_button_new_with_mnemonic ("_Play");
    ui_setPlayBtnState (FALSE);
    gtk_button_set_focus_on_click (GTK_BUTTON (ui_playBtn), FALSE);
    gtk_header_bar_pack_start (GTK_HEADER_BAR (ui_headerBar), ui_playBtn);
    g_signal_connect (ui_playBtn, "clicked", G_CALLBACK (ui_playClicked), ui_headerBar);

    /* selection toolbar */
    ui_tbSelection = gtk_toggle_button_new ();
    ui_setToolBarState (FALSE);
    gtk_button_set_focus_on_click (GTK_BUTTON (ui_tbSelection), FALSE);
    gtk_widget_set_tooltip_text (ui_tbSelection, "Selection");
    GtkWidget *imgSelect = gtk_image_new ();
    gtk_image_set_from_icon_name (GTK_IMAGE (imgSelect), "object-select-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (ui_tbSelection), GTK_WIDGET (imgSelect));
    gtk_header_bar_pack_end (GTK_HEADER_BAR (ui_headerBar), ui_tbSelection);

    /* connect "toggled" event to the button */
    g_signal_connect (G_OBJECT (ui_tbSelection), "toggled", G_CALLBACK (ui_select_cb), NULL);

    /* selection rank on */
    g_assert(!ui_selRankOn);
    ui_selRankOn = gdk_pixbuf_new_from_file (APP_RESOURCE APP_SELECT_RANK_ON, NULL);
    g_assert(ui_selRankOn);

    /* selection rank off */
    g_assert(!ui_selRankOff);
    ui_selRankOff = gdk_pixbuf_new_from_file (APP_RESOURCE APP_SELECT_RANK_OFF, NULL);
    g_assert(ui_selRankOff);

    /* selection fav on */
    g_assert(!ui_selPrefOn);
    ui_selPrefOn = gdk_pixbuf_new_from_file (APP_RESOURCE APP_SELECT_FAV_ON, NULL);
    g_assert(ui_selPrefOn);

    /* selection fav off */
    g_assert(!ui_selPrefOff);
    ui_selPrefOff = gdk_pixbuf_new_from_file (APP_RESOURCE APP_SELECT_FAV_OFF, NULL);
    g_assert(ui_selPrefOff);

    /* cut off the tilte bar */
    gtk_style_context_add_class (gtk_widget_get_style_context (ui_headerBar), "titlebar");
    gtk_window_set_titlebar (GTK_WINDOW (ui_window), (ui_headerBar));

    /* vbox  */
    GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (ui_window), vbox);

    /* infobar */
    ui_infobar = gtk_info_bar_new ();

    gtk_box_pack_start (GTK_BOX (vbox), ui_infobar, FALSE, FALSE, 0);
    gtk_info_bar_set_message_type (GTK_INFO_BAR (ui_infobar), GTK_MESSAGE_ERROR);
    GtkWidget *label = gtk_label_new ("No games found on your computer.\n" \
                "This may depend on mame executable not found or invalid rompath.\n" \
                "Please configure and restart gnome-arcade.");
    gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (ui_infobar))), label, FALSE, FALSE, 0);

    gtk_widget_realize(ui_infobar);
    gtk_widget_set_no_show_all (ui_infobar, TRUE);

    /* hbox  */
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_end (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    /* drawing area */
    ui_drawingArea = gtk_drawing_area_new ();
    gtk_widget_set_size_request (ui_drawingArea, 2 * TILE_W_BORDER_MIN + ui_tileSize_W, UI_OFFSET_Y + ui_tileSize_H + TILE_H_BORDER);
    gtk_box_pack_start (GTK_BOX (hbox), ui_drawingArea, TRUE, TRUE, 0);
    gtk_widget_set_hexpand (ui_drawingArea, TRUE);
    gtk_widget_set_vexpand (ui_drawingArea, TRUE);
    gtk_widget_realize (ui_drawingArea);

    /* scrollbar & adj widget*/
    ui_adjust = gtk_adjustment_new (0, 0, 0, 0, 0, 0);
    ui_scrollBar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, ui_adjust);

    /* hide */
    gtk_box_pack_end (GTK_BOX (hbox), ui_scrollBar, FALSE, FALSE, 0);

    g_signal_connect (ui_scrollBar, "value_changed", G_CALLBACK (ui_barValueChanged), ui_adjust);

    /* dark theme */
    if (cfg_keyBool ("USE_DARK_THEME")) {
        g_object_set (gtk_settings_get_default (), "gtk-application-prefer-dark-theme", TRUE, NULL);
    }

    /* user preference */
    pref_init ();

    pref_load ();

    /* romlist */
    rom_init ();

    rom_load ();

    rom_setSort (ROM_SORT_AZ);

    /* view */
    view_init ();

    /* web scrapper */
    www_init ();

    /* joy support */
    joy_init ();

    /* screen saver */
    ssaver_init ();

    ui_focusAdd (0);

    ui_aboutLogo = gdk_pixbuf_new_from_file (APP_RESOURCE APP_ICON_ABOUT, NULL);

    /* loading font */
    ui_tileFont = cairo_toy_font_face_create (TEXT_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    g_assert (ui_tileFont);

    /* signal */
    g_signal_connect (G_OBJECT (ui_drawingArea), "draw", G_CALLBACK (ui_drawingAreaDraw), NULL);
    g_signal_connect (G_OBJECT (ui_drawingArea), "button_press_event", G_CALLBACK (ui_drawingAreaButtonPress), NULL);
    g_signal_connect (G_OBJECT (ui_drawingArea), "scroll_event", G_CALLBACK (ui_drawingAreaScrollEvent), NULL);

    /* keyboard navigation */
    gtk_widget_set_can_focus (GTK_WIDGET (ui_drawingArea), TRUE);
    g_signal_connect (G_OBJECT (ui_drawingArea), "key_press_event", G_CALLBACK (ui_drawingAreaKeyPressEvent), NULL);
    g_signal_connect (G_OBJECT (ui_drawingArea), "motion_notify_event", G_CALLBACK (ui_drawingAreaMotionNotifyEvent), NULL);

    gtk_widget_add_events (GTK_WIDGET (ui_drawingArea), GDK_SCROLL_MASK);
    //gtk_widget_add_events (GTK_WIDGET (ui_drawingArea), GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events (GTK_WIDGET (ui_drawingArea), GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events (GTK_WIDGET (ui_drawingArea), GDK_POINTER_MOTION_MASK);

    g_signal_connect (G_OBJECT (ui_drawingArea), "configure-event", G_CALLBACK (ui_drawingAreaConfigureEvent), NULL);

    gtk_widget_show_all (GTK_WIDGET (ui_window));

    //g_idle_add ((GSourceFunc) joy_debugFull, NULL);
    //g_timeout_add (1000, (GSourceFunc) joy_debugFull, NULL);

}

void
ui_free (void)
{
    ui_mouseOver = -1;

    /* screen saver */
    ssaver_free ();

    /* user preference*/
    pref_free ();

    /*view */
    view_free ();

    /* free romlist */
    rom_free ();

    /* web scrapper */
    www_free ();

    /* joy */
    joy_free ();

    /* free config */
    cfg_free ();

    /* cairo font */
    cairo_font_face_destroy (ui_tileFont);
    ui_tileFont = NULL;

    /* select rank on */
    g_object_unref (ui_selRankOn);
    ui_selRankOn = NULL;

    /* select rank off */
    g_object_unref (ui_selRankOff);
    ui_selRankOff = NULL;

    /* select fav on */
    g_object_unref (ui_selPrefOn);
    ui_selPrefOn = NULL;

    /* select fav off */
    g_object_unref (ui_selPrefOff);
    ui_selPrefOff = NULL;

    g_object_unref (ui_aboutLogo);
    ui_aboutLogo = NULL;

}

void
ui_setPlayBtnState (gboolean state)
{
    gtk_widget_set_sensitive (ui_playBtn, state);
}

void
ui_setToolBarState (gboolean state)
{
    gtk_widget_set_sensitive (ui_tbSelection, state);
}

void
ui_setScrollBarState (gboolean state)
{
    gtk_widget_set_sensitive (ui_scrollBar, state);
}

void
ui_setFocus (void)
{
    gtk_window_present (GTK_WINDOW (ui_window));
}

gboolean
ui_tileIsVisible (struct rom_romItem *item)
{
    gint index = g_list_index (ui_viewModel->romList, item);

    if (index >= 0) {
        gint uiTop  = ((index) / ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea))));
        gint itemTop = uiTop * (ui_tileSize_H + TILE_H_BORDER) + UI_OFFSET_Y;

        if (itemTop > ui_viewModel->view + gtk_widget_get_allocated_height (GTK_WIDGET (ui_drawingArea)) || itemTop + ui_tileSize_H + TILE_H_BORDER < ui_viewModel->view) {
            //g_print ("%i is NOT visible\n", index);
            return FALSE;
        } else {
            //g_print ("%i is visible\n", index);
            return TRUE;
        }
    } else {
        //g_print ("NOT in list\n");
        return FALSE;
    }
}

inline static gint
ui_getTileCol (gint idx)
{
    gint maxCol = ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)));

    return idx % maxCol;
}

inline static gint
ui_getTileRow (gint idx)
{
    gint maxCol = ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)));

    return idx / maxCol;
}

static gint
ui_getTilePixbufW (gint idxModel)
{
    g_assert (idxModel >= 0);

    const GdkPixbuf* pix = rom_getItemTile (view_getItem (ui_viewModel, idxModel));

    if (!pix) pix = rom_tileNoImage;

    g_assert (pix);

    return gdk_pixbuf_get_width (pix);
}

inline static gint
ui_getTilePixbufH (gint idxModel)
{
    g_assert (idxModel >= 0);

    const GdkPixbuf* pix = rom_getItemTile (view_getItem (ui_viewModel, idxModel));

    if (!pix) pix = rom_tileNoImage;

    g_assert (pix);

    return gdk_pixbuf_get_height (pix);
}

inline static gdouble
ui_getTileX (gint idxModel)
{
    g_assert (idxModel >= 0);

    gint tileBorder = ui_getTileWBorderSize ();
    gint pixbuf_w = ui_getTilePixbufW (idxModel);

    return ui_getTileCol (idxModel) * (ui_tileSize_W + tileBorder) + tileBorder + (ui_tileSize_W - pixbuf_w) / 2;

}

inline static gdouble
ui_getTileY (gint idxModel)
{
    g_assert (idxModel >= 0);

    gint rowNum = ui_getTileRow (idxModel);
    gint pixbuf_h = ui_getTilePixbufH (idxModel);

    return rowNum * (ui_tileSize_H + TILE_H_BORDER) + UI_OFFSET_Y + (ui_tileSize_H - pixbuf_h);
}

static void
ui_prefManager (gdouble x, gdouble y)
{
    gint idx = ui_getTileIdx (x, y, TRUE);

    if (idx >=0) {
        gint fav_w = gdk_pixbuf_get_width (ui_selPrefOn);
        gint fav_h = gdk_pixbuf_get_height (ui_selPrefOn);

        gint baseTileX = ui_getTileX (idx) + EMBLEM_PADDING;
        gint baseTileY = ui_getTileY (idx) + EMBLEM_PADDING;

        if (pointInside (x, y + ui_viewModel->view, baseTileX, baseTileY, baseTileX + fav_w, baseTileY + fav_h)) {
            ui_preference_cb ();
        }
    }
}

static void
ui_rankManager (gdouble x, gdouble y)
{
    gint idx = ui_getTileIdx (x, y, TRUE);

    if (idx >=0) {
        gint fav_w = gdk_pixbuf_get_width (ui_selRankOn);
        gint fav_h = gdk_pixbuf_get_height (ui_selRankOn);

        for (gint i = 0; i < ROM_MAXRANK; ++i) {
            gint baseTileX = ui_getTileX (idx) + EMBLEM_PADDING + fav_w * i;
            gint baseTileY = ui_getTileY (idx) - EMBLEM_PADDING - fav_h + ui_getTilePixbufH (idx);

            if (pointInside (x, y + ui_viewModel->view, baseTileX, baseTileY, baseTileX + fav_w, baseTileY + fav_h)) {
                ui_rank_cb (i+1);
                return;
            }
        }
    }
}


void
ui_setDefaultView (struct view_viewModel *view)
{
    ui_viewModel = view;
    ui_mouseOver = -1;
    ui_mouseOverOld = 0;

    /* no rom, no play... */
    if (ui_viewModel->romCount <= 0) {
        ui_setPlayBtnState (FALSE);
        ui_setToolBarState (FALSE);
    } else {
        ui_setPlayBtnState (TRUE);
        ui_setToolBarState (TRUE);
    }

    gtk_widget_hide (GTK_WIDGET (ui_scrollBar));
}

void
ui_setView (struct view_viewModel *view)
{
    ui_viewModel = view;
    ui_mouseOver = -1;
    ui_mouseOverOld = 0;

    /* no rom, no play... */
    if (ui_viewModel->romCount <= 0) {
        ui_setPlayBtnState (FALSE);
        ui_setToolBarState (FALSE);
    } else {
        ui_setPlayBtnState (TRUE);
        ui_setToolBarState (TRUE);
    }

    if (ui_viewModel->romCount <= 0) {
        gtk_widget_hide (GTK_WIDGET (ui_scrollBar));
        return ;
    }

    ui_repaint ();

    g_object_freeze_notify (G_OBJECT (ui_adjust));
    g_object_freeze_notify (G_OBJECT (ui_drawingArea));
    g_object_freeze_notify (G_OBJECT (ui_scrollBar));

    ui_focusAt (ui_viewModel->focus);

    int newItemOnRow = ui_itemOnRow (gtk_widget_get_allocated_width (GTK_WIDGET (ui_drawingArea)));
    gint rowNum  = posval (ui_viewModel->romCount - 1) / newItemOnRow;
    gint upper = (rowNum + 1) * (ui_tileSize_H + TILE_H_BORDER) + UI_OFFSET_Y;
    gint pageHeight = gtk_widget_get_allocated_height (GTK_WIDGET (ui_drawingArea));

    gtk_adjustment_configure (GTK_ADJUSTMENT (ui_adjust), 0, (float) ui_viewModel->view, upper, UI_SCROLL_STEP, pageHeight, pageHeight);

    ui_drawingAreaConfigureEvent ();

    g_object_thaw_notify (G_OBJECT(ui_scrollBar));
    g_object_thaw_notify (G_OBJECT(ui_drawingArea));
    g_object_thaw_notify (G_OBJECT(ui_adjust));

    ui_viewModel = view;

    ui_invalidateDrawingArea ();
}

__attribute__ ((hot))
gboolean
ui_invalidateDrawingArea (void)
{
    GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (ui_drawingArea));
    if (win) {
        gdk_window_invalidate_rect (win, NULL, FALSE);
    }
    return TRUE;
}

void
ui_showInfobar (void)
{
    gtk_widget_set_no_show_all (ui_infobar, FALSE);
    gtk_widget_show_all (GTK_WIDGET (ui_infobar));
}

unsigned int
ui_getWindowXid (void)
{
    GdkWindow* win = gtk_widget_get_window (GTK_WIDGET (ui_window));
    return GDK_WINDOW_XID (win);
}

