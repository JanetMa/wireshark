/* tap-iousers.c
 * iostat   2003 Ronnie Sahlberg
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <epan/packet.h>
#include <epan/timestamp.h>
#include <ui/cli/tshark-tap.h>

typedef struct _io_users_t {
	const char *type;
	const char *filter;
	conv_hash_t hash;
} io_users_t;

static void
iousers_draw(void *arg)
{
	conv_hash_t *hash = (conv_hash_t*)arg;
	io_users_t *iu = (io_users_t *)hash->user_data;
	conv_item_t *iui;
	guint64 last_frames, max_frames;
	struct tm * tm_time;
	guint i;

	printf("================================================================================\n");
	printf("%s Conversations\n", iu->type);
	printf("Filter:%s\n", iu->filter ? iu->filter : "<No Filter>");

	switch (timestamp_get_type()) {
	case TS_ABSOLUTE:
	case TS_UTC:
		printf("                                               |       <-      | |       ->      | |     Total     | Absolute Time  |   Duration   |\n");
		printf("                                               | Frames  Bytes | | Frames  Bytes | | Frames  Bytes |      Start     |              |\n");
		break;
	case TS_ABSOLUTE_WITH_YMD:
	case TS_ABSOLUTE_WITH_YDOY:
	case TS_UTC_WITH_YMD:
	case TS_UTC_WITH_YDOY:
		printf("                                               |       <-      | |       ->      | |     Total     | Absolute Date  |   Duration   |\n");
		printf("                                               | Frames  Bytes | | Frames  Bytes | | Frames  Bytes |     Start      |              |\n");
		break;
	case TS_RELATIVE:
	case TS_NOT_SET:
	default:
		printf("                                               |       <-      | |       ->      | |     Total     |    Relative    |   Duration   |\n");
		printf("                                               | Frames  Bytes | | Frames  Bytes | | Frames  Bytes |      Start     |              |\n");
		break;
	}

	max_frames = UINT_MAX;
	do {
		last_frames = 0;
		for (i=0; (iu->hash.conv_array && i < iu->hash.conv_array->len); i++) {
			guint64 tot_frames;

			iui = &g_array_index(iu->hash.conv_array, conv_item_t, i);
			tot_frames = iui->rx_frames + iui->tx_frames;

			if ((tot_frames > last_frames) && (tot_frames < max_frames)) {
				last_frames = tot_frames;
			}
		}

		for (i=0; (iu->hash.conv_array && i < iu->hash.conv_array->len); i++) {
			guint64 tot_frames;

			iui = &g_array_index(iu->hash.conv_array, conv_item_t, i);
			tot_frames = iui->rx_frames + iui->tx_frames;

			if (tot_frames == last_frames) {
				printf("%-20s <-> %-20s  %6" G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER
				       "u  %6" G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER "u  %6"
				       G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER "u  ",
					/* XXX - TODO: make name resolution configurable (through gbl_resolv_flags?) */
					get_conversation_address(&iui->src_address, TRUE), get_conversation_address(&iui->dst_address, TRUE),
					iui->tx_frames, iui->tx_bytes,
					iui->rx_frames, iui->rx_bytes,
					iui->tx_frames+iui->rx_frames,
					iui->tx_bytes+iui->rx_bytes
				);

				switch (timestamp_get_type()) {
				case TS_ABSOLUTE:
					tm_time = localtime(&iui->start_abs_time.secs);
					printf("%02d:%02d:%02d   %12.4f\n",
						 tm_time->tm_hour,
						 tm_time->tm_min,
						 tm_time->tm_sec,
						 nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time));
					break;
				case TS_ABSOLUTE_WITH_YMD:
					tm_time = localtime(&iui->start_abs_time.secs);
					printf("%04d-%02d-%02d %02d:%02d:%02d   %12.4f\n",
						 tm_time->tm_year + 1900,
						 tm_time->tm_mon + 1,
						 tm_time->tm_mday,
						 tm_time->tm_hour,
						 tm_time->tm_min,
						 tm_time->tm_sec,
						 nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time));
					break;
				case TS_ABSOLUTE_WITH_YDOY:
					tm_time = localtime(&iui->start_abs_time.secs);
					printf("%04d/%03d %02d:%02d:%02d   %12.4f\n",
						 tm_time->tm_year + 1900,
						 tm_time->tm_yday + 1,
						 tm_time->tm_hour,
						 tm_time->tm_min,
						 tm_time->tm_sec,
						 nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time));
					break;
				case TS_UTC:
					tm_time = gmtime(&iui->start_abs_time.secs);
					printf("%02d:%02d:%02d   %12.4f\n",
						 tm_time->tm_hour,
						 tm_time->tm_min,
						 tm_time->tm_sec,
						 nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time));
					break;
				case TS_UTC_WITH_YMD:
					tm_time = gmtime(&iui->start_abs_time.secs);
					printf("%04d-%02d-%02d %02d:%02d:%02d   %12.4f\n",
						 tm_time->tm_year + 1900,
						 tm_time->tm_mon + 1,
						 tm_time->tm_mday,
						 tm_time->tm_hour,
						 tm_time->tm_min,
						 tm_time->tm_sec,
						 nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time));
					break;
				case TS_UTC_WITH_YDOY:
					tm_time = gmtime(&iui->start_abs_time.secs);
					printf("%04d/%03d %02d:%02d:%02d   %12.4f\n",
						 tm_time->tm_year + 1900,
						 tm_time->tm_yday + 1,
						 tm_time->tm_hour,
						 tm_time->tm_min,
						 tm_time->tm_sec,
						 nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time));
					break;
				case TS_RELATIVE:
				case TS_NOT_SET:
				default:
					printf("%14.9f   %12.4f\n",
						nstime_to_sec(&iui->start_time),
						nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time)
					);
					break;
				}
			}
		}
		max_frames = last_frames;
	} while (last_frames);
	printf("================================================================================\n");
}

void init_iousers(struct register_ct *ct, const char *filter)
{
	io_users_t *iu;
	GString *error_string;

	iu = g_new0(io_users_t, 1);
	iu->type = proto_get_protocol_short_name(find_protocol_by_id(get_conversation_proto_id(ct)));
	iu->filter = g_strdup(filter);
	iu->hash.user_data = iu;

	error_string = register_tap_listener(proto_get_protocol_filter_name(get_conversation_proto_id(ct)), &iu->hash, filter, 0, NULL, get_conversation_packet_func(ct), iousers_draw);
	if (error_string) {
		g_free(iu);
		fprintf(stderr, "tshark: Couldn't register conversations tap: %s\n",
		    error_string->str);
		g_string_free(error_string, TRUE);
		exit(1);
	}

}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
