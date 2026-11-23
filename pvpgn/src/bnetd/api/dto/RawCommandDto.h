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

#ifndef PVPGN_API_DTO_RAWCOMMANDDTO_H
#define PVPGN_API_DTO_RAWCOMMANDDTO_H

#include <optional>
#include <string>
#include <vector>

#include "common/field_sizes.h"
#include "../text.h"

namespace pvpgn::bnetd::api {

/*
 * Body of POST /rawcmd: a bnetd command exactly as it would be typed in chat.
 *
 *   {"command": "/lockacct playername"}
 *
 * "clienttag" is optional: the client (W3XP, WAR3, STAR, ...) the command is
 * run as, like a bot using /tag. Commands that act on the caller's client use
 * it, e.g. /games, /channels, /status and /gameinfo <name>.
 *
 *   {"command": "/gameinfo somegame", "clienttag": "W3XP"}
 */
struct RawCommandRequest {
  std::string command;
  std::optional<std::string> clienttag;
};

/*
 * What the command replied, one entry per line.
 *
 *   {"output": ["That user account is now locked."]}
 */
struct RawCommandResponse {
  std::vector<std::string> output;
};

/* Returns what is wrong with `request`, or nothing if it can be run. */
inline std::optional<std::string> validate(const RawCommandRequest& request) {
  const std::string& command = request.command;

  if (command.size() < 2 || command[0] != '/')
    return "\"command\" must start with '/'";
  // Some command handlers (/set, /cg, /seticon) strncpy() the command into a
  // MAX_MESSAGE_LEN buffer, copying at most MAX_MESSAGE_LEN - 1 bytes, which
  // only includes the terminating NUL if the command is shorter than that.
  if (command.size() > MAX_MESSAGE_LEN - 2)
    return "\"command\" is too long";
  for (const char ch : command) {
    // one chat line only; this also rejects an embedded NUL
    if (static_cast<unsigned char>(ch) < 0x20)
      return "\"command\" must not contain control characters";
  }
  if (request.clienttag && !parse_clienttag(*request.clienttag))
    return "\"clienttag\" is not a known client tag";
  return std::nullopt;
}

}

#endif // PVPGN_API_DTO_RAWCOMMANDDTO_H
