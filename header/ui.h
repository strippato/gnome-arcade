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


/* ui.h */
#ifndef UI_H
#define UI_H

gint ui_tileSize_W;
gint ui_tileSize_H;

void ui_init (void);
void ui_free (void);

gboolean ui_drawingAreaDraw (GtkWidget *widget, cairo_t *cr);

void ui_setFocus (void);

void ui_freePixbuffer (void);
void ui_loadPixbuffer (void);

void ui_setPlayBtnState (gboolean state);
void ui_setDropBtnState (gboolean state);
void ui_setToolBarState (gboolean state);
void ui_setScrollBarState (gboolean state);

gboolean ui_tileIsVisible (struct rom_romItem *item);

void ui_actionFullscreen (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
void ui_actionChangeFullscreen (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
void ui_quit (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
void ui_actionSort (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
void ui_showAbout (GSimpleAction *action, GVariant *parameter, gpointer user_data);
void ui_setView (struct view_viewModel *view);
void ui_setDefaultView (struct view_viewModel *view);
unsigned int ui_getWindowXid (void);

void ui_cmdUp (void);
void ui_cmdDown (void);
void ui_cmdLeft (void);
void ui_cmdRight (void);
void ui_cmdHome (void);
void ui_cmdEnd (void);
void ui_cmdPreference (void);
void ui_cmdRankUp (void);
void ui_cmdRankDown (void);
gboolean ui_cmdPlay (void);

gboolean ui_inSelectState (void);
gboolean ui_invalidateDrawingArea (void);

void ui_afterDownload (void);
void ui_downloadWarn (const gchar* text);
gboolean ui_downloadAsk (void);
void ui_progress_cb (void);

void ui_showInfobar (void);
void ui_feedback (void);

#endif

