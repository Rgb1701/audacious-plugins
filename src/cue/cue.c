/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2009 William Pitcock <nenolod@dereferenced.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* #define AUD_DEBUG 1 */

#include <glib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <audacious/plugin.h>

#include <libcue/libcue.h>

typedef struct {
    gint tuple_type;
    gint pti;
} TuplePTIMap;

TuplePTIMap pti_map[] = {
    { FIELD_ARTIST, PTI_PERFORMER },
    { FIELD_TITLE, PTI_TITLE },
};

static void
tuple_attach_cdtext(Tuple *tuple, Track *track, gint tuple_type, gint pti)
{
    Cdtext *cdtext;
    const gchar *text;

    g_return_if_fail(tuple != NULL);
    g_return_if_fail(track != NULL);

    cdtext = track_get_cdtext(track);
    if (cdtext == NULL)
        return;

    text = cdtext_get(pti, cdtext);
    if (text == NULL)
        return;

    tuple_associate_string(tuple, tuple_type, NULL, text);

    g_print("Attached [%s] to tuple [%p]\n", text, tuple);
}

static void
playlist_load_cue(const gchar * filename, gint pos)
{
    gchar *uri = NULL, *buffer;
    gsize len;
    Cd *cd;
    gint iter, plen;
    gint playlist;

    uri = g_filename_to_uri(filename, NULL, NULL);

    aud_vfs_file_get_contents(uri ? uri : filename, &buffer, &len);

    g_free(uri); uri = NULL;

    if (buffer == NULL)
        return;

    g_print("Got CUE [%s]\n", buffer);
    cd = cue_parse_string(buffer);

    if (cd == NULL)
        return;

    playlist = aud_playlist_get_active();
    for (iter = 0, plen = cd_get_ntrack(cd); iter < plen; iter++)
    {
        gchar *fn, *uri;
        Track *t;
        glong begin, length;
        Tuple *tu;
        gint iter2;

	t = cd_get_track(cd, 1 + iter);
        fn = track_get_filename(t);
        begin = (track_get_start(t) * 10);
        length = (track_get_length(t) * 10);
        uri = aud_construct_uri(fn, filename);
        tu = tuple_new();

        g_print("Adding track %d [%ld] - [%ld]\n", iter + 1, begin, begin + length);

        for (iter2 = 0; iter2 < G_N_ELEMENTS(pti_map); iter2++)
        {
            tuple_attach_cdtext(tu, t, pti_map[iter2].tuple_type, pti_map[iter2].pti);
        }

        tuple_associate_int(tu, FIELD_LENGTH, NULL, length);

        aud_playlist_entry_insert(playlist, pos + iter, uri, tu);
        aud_playlist_entry_set_segmentation(playlist, pos + iter, begin, begin + length);
    }

    g_free(buffer);
}

static void
playlist_save_cue(const gchar *filename, gint pos)
{
    AUDDBG("todo\n");
}

PlaylistContainer plc_cue = {
    .name = "cue Playlist Format",
    .ext = "cue",
    .plc_read = playlist_load_cue,
    .plc_write = playlist_save_cue,
};

static void init(void)
{
    aud_playlist_container_register(&plc_cue);
}

static void cleanup(void)
{
    aud_playlist_container_unregister(&plc_cue);
}

DECLARE_PLUGIN(cue, init, cleanup, NULL, NULL, NULL, NULL, NULL, NULL);
