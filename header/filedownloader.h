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


/* filedownloader.h */
#ifndef FD_H
#define FD_H

struct fd_copyInfo {
	// input
	gchar *iFileName;
	GFile *iFile;

	// output
	gchar *oFileName;
	GFile *oFile;
};

gint fd_downloadingItm;

void fd_init (void);
void fd_free (void);
void fd_downloadRom (const gchar* romName);

void fd_findAndDownloadChd (const gchar* romName);

const gchar* fd_getDownloadPathRom (void);
const gchar* fd_getDownloadPathChd (void);

#endif

