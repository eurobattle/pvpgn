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

#ifndef PVPGN_API_DTO_GAMEDTO_H
#define PVPGN_API_DTO_GAMEDTO_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <rfl.hpp>

namespace pvpgn::bnetd::api {

/* ---- GET /games/custom ---- */

/* A game created by a player or a hostbot, as listed in the game list. */
struct CustomGameDto {
  unsigned int id;
  std::string name;
  /* the account that hosts the game; absent if the host is gone */
  std::optional<std::string> owner;
  std::string clienttag;
  /* "open", "full", "started", "loaded" or "done" */
  std::string status;
  /* has a password; only listed when the request filters by owner */
  rfl::Rename<"private", bool> is_private;
  std::optional<std::string> map;
  std::optional<unsigned int> map_width;
  std::optional<unsigned int> map_height;
  /* unix time */
  std::int64_t created;
  std::optional<std::int64_t> started;
  /* the players bnetd knows to be in the game */
  std::vector<std::string> players;
  /* "ip:port" of the host; only when the server does not hide addresses */
  std::optional<std::string> address;
};

struct CustomGameListDto {
  std::vector<CustomGameDto> games;
};

/* ---- GET /games/ladder/queue ---- */

/* How many players are searching a matchmaking queue. No names: that would help fixing matches. */
struct LadderQueueDto {
  std::string queue;
  unsigned int searching;
  /* seconds the longest searching player has been waiting; 0 if nobody is searching */
  std::int64_t longest_wait;
};

struct LadderQueueListDto {
  std::vector<LadderQueueDto> queues;
};

/* ---- GET /games/ladder/in_progress ---- */

struct LadderMatchPlayerDto {
  std::string name;
  std::string clienttag;
};

/* A match made by matchmaking, from the moment it is made until the players leave the game. */
struct LadderMatchDto {
  std::string queue;
  /* "joining": not all players are in the game yet, "loading", or "playing" */
  std::string state;
  std::optional<std::string> map;
  /* unix time the match was made */
  std::int64_t created;
  /* the players still in the match */
  std::vector<LadderMatchPlayerDto> players;
};

struct LadderMatchListDto {
  std::vector<LadderMatchDto> matches;
};

}

#endif // PVPGN_API_DTO_GAMEDTO_H
