/**
 * Written in 2016 by David H. Wei <https://github.com/spikeh>
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include <deadbeef/deadbeef.h>
#include <string.h>

DB_functions_t *deadbeef;

#define DEFAULT_KEY "loved"
#define DEFAULT_VAL "\xE2\x9D\xA4"

static int
do_loveit (DB_plugin_action_t *action, int ctx)
{
    DB_playItem_t *track = NULL;

    if (ctx == DDB_ACTION_CTX_SELECTION) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr();
        if (plt) {
            track = deadbeef->plt_get_first(plt, PL_MAIN);
            while (track) {
                if (deadbeef->pl_is_selected(track)) {
                    break;
                }
                DB_playItem_t *next = deadbeef->pl_get_next(track, PL_MAIN);
                deadbeef->pl_item_unref(track);
                track = next;
            }
            deadbeef->plt_unref(plt);
        }
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        track = deadbeef->streamer_get_playing_track();
    }

    if (track) {
        deadbeef->conf_lock();
        const char *key = deadbeef->conf_get_str_fast("loveit.key", "");
        const char *val = deadbeef->conf_get_str_fast("loveit.val", "");
        deadbeef->conf_unlock();
        deadbeef->pl_lock();
        if (!(deadbeef->pl_meta_exists(track, key))) {
            deadbeef->pl_add_meta(track, key, val);

            ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_TRACKINFOCHANGED);
            ev->track = track;
            deadbeef->pl_item_ref(ev->track);
            deadbeef->event_send((ddb_event_t *)ev, 0, 0);

            const char *dec_name = deadbeef->pl_find_meta_raw(track, ":DECODER");
            if (dec_name) {
                deadbeef->pl_item_ref(track);
                DB_decoder_t **all_decoders = deadbeef->plug_get_decoder_list();
                DB_decoder_t *dec = NULL;
                for (int i = 0; all_decoders[i]; ++i) {
                    if (!strcmp(all_decoders[i]->plugin.id, dec_name)) {
                        dec = all_decoders[i];
                        if (dec->write_metadata) {
                            dec->write_metadata(track);
                        }
                        break;
                    }
                }
                deadbeef->pl_item_unref(track);
            }

            ddb_playlist_t *plt = deadbeef->plt_get_curr();
            if (plt) {
                deadbeef->plt_modified(plt);
                deadbeef->plt_unref(plt);
            }
            deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, 0, 0, 0);
        }   else    {
            deadbeef->pl_delete_meta(track, key);
        }
        deadbeef->pl_item_unref(track);
        deadbeef->pl_unlock();
    }
    return 0;
}

static DB_plugin_action_t loveit_action = {
    .title = "Toggle Love",
    .name = "loveit",
    .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_ADD_MENU,
    .callback2 = do_loveit,
    .next = NULL
};

static DB_plugin_action_t *
loveit_get_actions (DB_playItem_t *it)
{
    return &loveit_action;
};

static const char settings_dlg[] =
    "property \"Metadata key\" entry loveit.key \""DEFAULT_KEY"\";"
    "property \"Metadata value\" entry loveit.val \""DEFAULT_VAL"\";"
;

DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 5,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.name = "LoveIt",
    .plugin.descr = "Mark loved tracks.",
    .plugin.copyright =
        "Written in 2016 by David H. Wei <https://github.com/spikeh>\n"
        "To the extent possible under law, the author(s) have dedicated all "
        "copyright\n"
        "and related and neighboring rights to this software to the public "
        "domain\n"
        "worldwide. This software is distributed without any warranty.\n"
        "\n"
        "You should have received a copy of the CC0 Public Domain Dedication "
        "along\n"
        "with this software. If not, see "
        "<http://creativecommons.org/publicdomain/zero/1.0/>."
    ,
    .plugin.website = "https://github.com/spikeh/deadbeef-loveit",
    .plugin.configdialog = settings_dlg,
    .plugin.get_actions = loveit_get_actions,
};

DB_plugin_t * loveit_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
