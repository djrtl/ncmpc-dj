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

#include "strfsong.h"
#include "charset.h"
#include "utils.h"

#include <mpd/client.h>

#include <string.h>

static const gchar *
skip(const gchar * p)
{
	gint stack = 0;

	while (*p != '\0') {
		if (*p == '[')
			stack++;
		if (*p == '#' && p[1] != '\0') {
			/* skip escaped stuff */
			++p;
		} else if (stack) {
			if(*p == ']') stack--;
		} else {
			if(*p == '&' || *p == '|' || *p == ']') {
				break;
			}
		}
		++p;
	}

	return p;
}

#ifndef NCMPC_MINI

static char *
concat_tag_values(const char *a, const char *b)
{
	return g_strconcat(a, ", ", b, NULL);
}

static char *
song_more_tag_values(const struct mpd_song *song, enum mpd_tag_type tag,
		     const char *first)
{
	const char *p = mpd_song_get_tag(song, tag, 1);
	char *buffer, *prev;

	if (p == NULL)
		return NULL;

	buffer = concat_tag_values(first, p);
	for (unsigned i = 2; (p = mpd_song_get_tag(song, tag, i)) != NULL;
	     ++i) {
		prev = buffer;
		buffer = concat_tag_values(buffer, p);
		g_free(prev);
	}

	return buffer;
}

#endif /* !NCMPC_MINI */

static char *
song_tag_locale(const struct mpd_song *song, enum mpd_tag_type tag)
{
	const char *value = mpd_song_get_tag(song, tag, 0);
	char *result;
#ifndef NCMPC_MINI
	char *all;
#endif /* !NCMPC_MINI */

	if (value == NULL)
		return NULL;

#ifndef NCMPC_MINI
	all = song_more_tag_values(song, tag, value);
	if (all != NULL)
		value = all;
#endif /* !NCMPC_MINI */

	result = utf8_to_locale(value);

#ifndef NCMPC_MINI
	g_free(all);
#endif /* !NCMPC_MINI */

	return result;
}

static gsize
_strfsong(gchar *s,
	  gsize max,
	  const gchar *format,
	  const struct mpd_song *song,
	  const gchar **last)
{
	const gchar *p, *end;
	gchar *temp;
	gsize n, length = 0;
	gboolean found = FALSE;

	memset(s, 0, max);
	if (song == NULL)
		return 0;

	for (p = format; *p != '\0' && length<max;) {
		/* OR */
		if (p[0] == '|') {
			++p;
			if(!found) {
				memset(s, 0, max);
				length = 0;
			} else {
				p = skip(p);
			}
			continue;
		}

		/* AND */
		if (p[0] == '&') {
			++p;
			if(!found) {
				p = skip(p);
			} else {
				found = FALSE;
			}
			continue;
		}

		/* EXPRESSION START */
		if (p[0] == '[') {
			temp = g_malloc0(max);
			if( _strfsong(temp, max, p+1, song, &p) >0 ) {
				g_strlcat(s, temp, max);
				length = strlen(s);
				found = TRUE;
			}
			g_free(temp);
			continue;
		}

		/* EXPRESSION END */
		if (p[0] == ']') {
			if(last) *last = p+1;
			if(!found && length) {
				memset(s, 0, max);
				length = 0;
			}
			return length;
		}

		/* pass-through non-escaped portions of the format string */
		if (p[0] != '#' && p[0] != '%' && length<max) {
			s[length++] = *p;
			p++;
			continue;
		}

		/* let the escape character escape itself */
		if (p[0] == '#' && p[1] != '\0' && length<max) {
			s[length++] = *(p+1);
			p+=2;
			continue;
		}

		/* advance past the esc character */

		/* find the extent of this format specifier (stop at \0, ' ', or esc) */
		temp = NULL;
		end  = p+1;
		while(*end >= 'a' && *end <= 'z') {
			end++;
		}
		n = end - p + 1;
		if(*end != '%')
			n--;
		else if (strncmp("%file%", p, n) == 0)
			temp = utf8_to_locale(mpd_song_get_uri(song));
		else if (strncmp("%artist%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_ARTIST);
		else if (strncmp("%title%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_TITLE);
		else if (strncmp("%album%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_ALBUM);
		else if (strncmp("%shortalbum%", p, n) == 0) {
			temp = song_tag_locale(song, MPD_TAG_ALBUM);
			if (temp) {
				gchar *temp2 = g_strndup(temp, 25);
				if (strlen(temp) > 25) {
					temp2[24] = '.';
					temp2[23] = '.';
					temp2[22] = '.';
				}
				g_free(temp);
				temp = temp2;
			}
		}
		else if (strncmp("%track%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_TRACK);
		else if (strncmp("%name%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_NAME);
		else if (strncmp("%date%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_DATE);
		else if (strncmp("%genre%", p, n) == 0)
			temp = song_tag_locale(song, MPD_TAG_GENRE);
		else if (strncmp("%shortfile%", p, n) == 0) {
			const char *uri = mpd_song_get_uri(song);
			if (strstr(uri, "://") != NULL)
				temp = utf8_to_locale(uri);
			else
				temp = utf8_to_locale(g_basename(uri));
		} else if (strncmp("%time%", p, n) == 0) {
			unsigned duration = mpd_song_get_duration(song);

			if (duration > 0)  {
				char buffer[32];
				format_duration_short(buffer, sizeof(buffer),
						      duration);
				temp = g_strdup(buffer);
			}
		}

		if( temp == NULL) {
			gsize templen=n;
			/* just pass-through any unknown specifiers (including esc) */
			/* drop a null char in so printf stops at the end of this specifier,
			   but put the real character back in (pseudo-const) */
			if( length+templen > max )
				templen = max-length;
			g_strlcat(s, p,max);
			length+=templen;
		} else {
			gsize templen = strlen(temp);

			found = TRUE;
			if( length+templen > max )
				templen = max-length;
			g_strlcat(s, temp, max);
			length+=templen;
			g_free(temp);
		}

		/* advance past the specifier */
		p += n;
	}

	if(last) *last = p;

	return length;
}

gsize
strfsong(gchar *s, gsize max, const gchar *format,
	 const struct mpd_song *song)
{
	return _strfsong(s, max, format, song, NULL);
}

