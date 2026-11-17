/*
 * Copyright (C) 2026  cen (cen.is.imba@gmail.com)
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef PVPGN_API_LADDER_H
#define PVPGN_API_LADDER_H

#include <array>
#include <optional>
#include <string_view>

// the matchmaking queue numbers (ANONGAME_TYPE_*); comes with the packet types
#include "common/packet.h"

/* Names the REST API uses for the matchmaking (ladder) queues. */
namespace pvpgn::bnetd::api {

namespace detail {
// indexed by the ANONGAME_TYPE_* queue number
inline constexpr std::array<std::string_view, ANONGAME_TYPES> queue_names = {
    "1v1",       // ANONGAME_TYPE_1V1
    "2v2",       // ANONGAME_TYPE_2V2
    "3v3",       // ANONGAME_TYPE_3V3
    "4v4",       // ANONGAME_TYPE_4V4
    "ffa",       // ANONGAME_TYPE_SMALL_FFA
    "at2v2",     // ANONGAME_TYPE_AT_2V2
    "atffa",     // ANONGAME_TYPE_TEAM_FFA
    "at3v3",     // ANONGAME_TYPE_AT_3V3
    "at4v4",     // ANONGAME_TYPE_AT_4V4
    "tournament",// ANONGAME_TYPE_TY
    "5v5",       // ANONGAME_TYPE_5V5
    "6v6",       // ANONGAME_TYPE_6V6
    "2v2v2",     // ANONGAME_TYPE_2V2V2
    "3v3v3",     // ANONGAME_TYPE_3V3V3
    "4v4v4",     // ANONGAME_TYPE_4V4V4
    "2v2v2v2",   // ANONGAME_TYPE_2V2V2V2
    "3v3v3v3",   // ANONGAME_TYPE_3V3V3V3
    "at2v2v2",   // ANONGAME_TYPE_AT_2V2V2
};
}  // namespace detail

/* The name of queue number `queue`, or nothing for a number that is not a queue. */
inline std::optional<std::string_view> queue_name(int queue) {
  if (queue < 0 || queue >= ANONGAME_TYPES)
    return std::nullopt;
  return detail::queue_names[queue];
}

/* The queue number named `name` (exact, lower case), or nothing for an unknown name. */
inline std::optional<int> parse_queue(std::string_view name) {
  for (int queue = 0; queue < ANONGAME_TYPES; queue++) {
    if (detail::queue_names[queue] == name)
      return queue;
  }
  return std::nullopt;
}

}

#endif // PVPGN_API_LADDER_H
