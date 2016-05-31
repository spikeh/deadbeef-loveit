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
#include <stdlib.h>

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

#define DEFAULT_KEY "loved"
#define DEFAULT_VAL "\xE2\x9D\xA4"

static int
do_loveit (DB_plugin_action_t *action, int ctx)
{
    DB_playItem_t *it = NULL;
    ddb_playlist_t *plt = NULL;
    int num = 0;

    if (ctx == DDB_ACTION_CTX_SELECTION) {
        plt = deadbeef->plt_get_curr ();
        if (plt) {
            num = deadbeef->plt_getselcount (plt);
            it = deadbeef->plt_get_first (plt, PL_MAIN);
            while (it) {
                if (deadbeef->pl_is_selected (it)) {
                    break;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
            }
            deadbeef->plt_unref (plt);
        }
    } else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        it = deadbeef->streamer_get_playing_track ();
        plt = deadbeef->plt_get_curr ();
        num = 1;
    }
    if (!it || !plt || num < 1) {
        goto out;
    }

    int count = 0;
    
    deadbeef->conf_lock();
    const char *key = deadbeef->conf_get_str_fast("loveit.key", "");
    const char *val = deadbeef->conf_get_str_fast("loveit.val", "");
    deadbeef->conf_unlock();
    
    while (it) {
        if (deadbeef->pl_is_selected (it) || ctx == DDB_ACTION_CTX_NOWPLAYING) {
            deadbeef->pl_lock ();
            if (!(deadbeef->pl_meta_exists(it, key))) {
                deadbeef->pl_add_meta(it, key, val);
            }   else    {
                deadbeef->pl_delete_meta(it, key);
            }

            ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_TRACKINFOCHANGED);
            ev->track = it;
            deadbeef->pl_item_ref(ev->track);
            deadbeef->event_send((ddb_event_t *)ev, 0, 0);

            const char *dec = deadbeef->pl_find_meta_raw (it, ":DECODER");
            char decoder_id[100];
            if (dec) {
                strncpy (decoder_id, dec, sizeof (decoder_id));
            }
            int match = it && dec;
            deadbeef->pl_unlock ();
            if (match) {
                int is_subtrack = deadbeef->pl_get_item_flags (it) & DDB_IS_SUBTRACK;
                if (is_subtrack) {
                    continue;
                }
                DB_decoder_t *dec = NULL;
                DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
                for (int i = 0; decoders[i]; i++) {
                    if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                        dec = decoders[i];
                        if (dec->write_metadata) {
                            dec->write_metadata (it);
                        }
                        break;
                    }
                }
            }
            count++;
            if (count >= num) {
                break;
            }
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    if (plt) {
        deadbeef->plt_modified (plt);
    }

out:
    deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    if (it) {
        deadbeef->pl_item_unref (it);
    }
    return 0;
}

static DB_plugin_action_t loveit_action = {
    .title = "Toggle Love",
    .name = "loveit",
    .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU,
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

static DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 5,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.name = "LoveIt",
    .plugin.descr = "Toggle Mark loved tracks.",
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
    .plugin.website = "https://github.com/rrevanth/deadbeef-loveit",
    .plugin.configdialog = settings_dlg,
    .plugin.get_actions = loveit_get_actions,
};

DB_plugin_t * loveit_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
