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


/* rom.h */
#ifndef ROM_H
#define ROM_H

#define ROM_LEGAL "Downloading a copy of a game you don’t own is not legal"

extern gchar *rom_tilePath;

extern GdkPixbuf *rom_tileNoImage;
extern GdkPixbuf *rom_tileNowShowing;
extern GdkPixbuf *rom_tileLoading;
extern GdkPixbuf *rom_tileFavorite;
extern GdkPixbuf *rom_tileRank;

enum rom_sortOrder {
    ROM_SORT_AZ,
    ROM_SORT_ZA
};

struct rom_romItem {
	// mame
	gchar *name;                //	1941                                      - romname
	gchar *description;	        //	1941: The Counter Attack (World 900227)   - (used this for searching)

	gboolean romFound;  		// 	only for PARENT                           - Parent: rom found; Clone: ignored;

	// view/sort
	gchar *desc;                //  1941: Counter Attack, The                 - (used for sorting/jump to letter)

	// user preference
	gboolean pref;
	guint	 rank;
	guint	 nplay;

	// pixbuf
	gboolean  tileLoading;
	gboolean  tileLoaded;
	GdkPixbuf *tile;
};

extern GList* rom_romList; // all parent games (NOT clone, NOT FILTERED)
extern GHashTable* rom_cloneTable; // only clones (NOT FILTERED) key:Clone, Item:Parent
extern GHashTable* rom_parentTableSearch; // for searching key=PARENT, data=description, romname
extern GHashTable* rom_parentTable;       // key=PARENT, data=description (romname)

#define ROM_EXTENSION_ZIP	"zip"
#define ROM_EXTENSION_7ZIP	"7z"
#define ROM_MAXRANK 5

void rom_init (void);
void rom_free (void);
void rom_load (void);
enum rom_sortOrder rom_getSort (void);
void rom_setSort (enum rom_sortOrder order);

gboolean rom_isClone (const gchar *romName);
gboolean rom_isParent (const gchar *romName);

struct rom_romItem* rom_newItem (void);
struct rom_romItem* rom_getItem (int numGame);

void rom_setItemName (struct rom_romItem* item, gchar* name);
void rom_setItemDescription (struct rom_romItem* item, gchar* description);
void rom_setItemTile (struct rom_romItem* item, GdkPixbuf* tile);
void rom_setItemRomFound (struct rom_romItem* item, gboolean value);
void rom_loadItemAsync (struct rom_romItem* item);

const gchar* rom_getItemName (struct rom_romItem* item);
const gchar* rom_getItemDescription (struct rom_romItem* item);
const gchar* rom_getItemDesc (struct rom_romItem* item);
const GdkPixbuf* rom_getItemTile (struct rom_romItem* item);
const gboolean rom_getItemRomFound (struct rom_romItem* item);
const gboolean rom_getItemTileLoaded (struct rom_romItem* item);
const gboolean rom_getItemTileLoading (struct rom_romItem* item);

const gchar* rom_parentOf (const gchar *romName);
void rom_invalidateUselessTile (void);

gboolean rom_getItemPref (const struct rom_romItem *item);
void rom_setItemPref (struct rom_romItem *item, gboolean value);
guint rom_getItemRank (const struct rom_romItem *item);
void rom_setItemRank (struct rom_romItem *item, guint rank);
guint rom_getItemNPlay (const struct rom_romItem *item);
void rom_setItemNPlay (struct rom_romItem *item, guint nplay);
gint rom_search (GList* viewModel, gint focus, const gchar* romDes, gboolean forward);
gint rom_search_letter (GList* viewModel, gint focus, const gchar* romStartWithLetter, gboolean forward);
gboolean rom_FoundInPath (const gchar* romName, ...);

#endif


