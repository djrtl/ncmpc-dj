/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "playlist.h"

#include <string.h>

void
playlist_init(struct mpdclient_playlist *playlist)
{
	playlist->version = 0;
	playlist->list = g_ptr_array_sized_new(1024);
}

void
playlist_clear(struct mpdclient_playlist *playlist)
{
	guint i;

	playlist->version = 0;

	for (i = 0; i < playlist->list->len; ++i) {
		struct mpd_song *song = playlist_get(playlist, i);

		mpd_song_free(song);
	}

	g_ptr_array_set_size(playlist->list, 0);
}

gint
mpdclient_playlist_free(struct mpdclient_playlist *playlist)
{
	if (playlist->list != NULL) {
		playlist_clear(playlist);
		g_ptr_array_free(playlist->list, TRUE);
	}

	memset(playlist, 0, sizeof(*playlist));
	return 0;
}

const struct mpd_song *
playlist_get_song(const struct mpdclient_playlist *playlist, gint idx)
{
	if (idx < 0 || (guint)idx >= playlist_length(playlist))
		return NULL;

	return playlist_get(playlist, idx);
}

void
playlist_move(struct mpdclient_playlist *playlist,
	      unsigned dest, unsigned src)
{
	struct mpd_song *song;

	assert(playlist != NULL);
	assert(src < playlist_length(playlist));
	assert(dest < playlist_length(playlist));
	assert(src != dest);

	song = playlist_get(playlist, src);

	if (src < dest) {
		memmove(&playlist->list->pdata[src],
			&playlist->list->pdata[src + 1],
			sizeof(playlist->list->pdata[0]) * (dest - src));
		playlist->list->pdata[dest] = song;
	} else {
		memmove(&playlist->list->pdata[dest + 1],
			&playlist->list->pdata[dest],
			sizeof(playlist->list->pdata[0]) * (src - dest));
		playlist->list->pdata[dest] = song;
	}
}

const struct mpd_song *
playlist_lookup_song(const struct mpdclient_playlist *playlist, unsigned id)
{
	for (guint i = 0; i < playlist_length(playlist); ++i) {
		struct mpd_song *song = playlist_get(playlist, i);
		if (mpd_song_get_id(song) == id)
			return song;
	}

	return NULL;
}

gint
playlist_get_index(const struct mpdclient_playlist *playlist,
		   const struct mpd_song *song)
{
	for (guint i = 0; i < playlist_length(playlist); ++i) {
		if (playlist_get(playlist, i) == song)
			return (gint)i;
	}

	return -1;
}

gint
playlist_get_index_from_id(const struct mpdclient_playlist *playlist,
			   unsigned id)
{
	for (guint i = 0; i < playlist_length(playlist); ++i) {
		const struct mpd_song *song = playlist_get(playlist, i);
		if (mpd_song_get_id(song) == id)
			return (gint)i;
	}

	return -1;
}

gint
playlist_get_index_from_file(const struct mpdclient_playlist *playlist,
			     const gchar *filename)
{
	for (guint i = 0; i < playlist_length(playlist); ++i) {
		const struct mpd_song *song = playlist_get(playlist, i);

		if (strcmp(mpd_song_get_uri(song), filename) == 0)
			return (gint)i;
	}

	return -1;
}

gint
playlist_get_id_from_uri(const struct mpdclient_playlist *playlist,
			 const gchar *uri)
{
	for (guint i = 0; i < playlist_length(playlist); ++i) {
		const struct mpd_song *song = playlist_get(playlist, i);

		if (strcmp(mpd_song_get_uri(song), uri) == 0)
			return mpd_song_get_id(song);
	}

	return -1;
}
