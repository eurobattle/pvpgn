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

#include "common/setup_before.h"
#include "GamesCtrl.h"

#include <algorithm>
#include <charconv>
#include <ctime>
#include <map>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include "dispatch.h"
#include "json.h"
#include "ladder.h"
#include "text.h"
#include "dto/GameDto.h"
#include "compat/strcasecmp.h"
#include "compat/strncasecmp.h"
#include "common/addr.h"
#include "common/list.h"
#include "common/tag.h"
#include "../anongame.h"
#include "../anongame_maplists.h"
#include "../connection.h"
#include "../game.h"
#include "../prefs.h"
#include "common/setup_after.h"

namespace pvpgn::bnetd::api {

namespace {

using Callback = std::function<void (const drogon::HttpResponsePtr &)>;

std::string tag_string(t_clienttag tag) {
  char text[5];
  return tag_uint_to_str(text, tag);
}

// The account name of `conn`, as other players see it; empty if it has none.
std::string user_name(t_connection* conn) {
  if (!conn)
    return {};
  const char* name = conn_get_chatname(conn);
  if (!name)
    return {};
  std::string result = to_utf8(name);
  conn_unget_chatname(conn, name);
  return result;
}

// Reads the "clienttag" query parameter; false (and a 400 sent) if it is not a known tag.
bool read_clienttag(const drogon::HttpRequestPtr& req, t_clienttag& tag, Callback& callback) {
  tag = 0;
  const std::string& text = req->getParameter("clienttag");
  if (text.empty())
    return true;
  const std::optional<t_clienttag> parsed = parse_clienttag(text);
  if (!parsed) {
    callback(error_response(drogon::k400BadRequest, "\"clienttag\" is not a known client tag"));
    return false;
  }
  tag = *parsed;
  return true;
}

// Reads the "queue" query parameter; false (and a 400 sent) if it is not a queue name.
bool read_queue(const drogon::HttpRequestPtr& req, std::optional<int>& queue, Callback& callback) {
  queue.reset();
  const std::string& text = req->getParameter("queue");
  if (text.empty())
    return true;
  queue = parse_queue(text);
  if (!queue) {
    callback(error_response(drogon::k400BadRequest, "\"queue\" is not a known matchmaking queue"));
    return false;
  }
  return true;
}

/* ---- /games/custom ---- */

struct CustomFilter {
  t_clienttag clienttag = 0;
  std::optional<t_game_status> status;
  std::string owner;  // name prefix; empty: any
  std::string name;   // empty: any
};

struct CustomListing {
  const CustomFilter* filter;
  CustomGameListDto* response;
};

std::optional<t_game_status> parse_status(const std::string& text) {
  static const std::pair<const char*, t_game_status> statuses[] = {
      {"open", game_status_open},       {"full", game_status_full},
      {"started", game_status_started}, {"loaded", game_status_loaded},
      {"done", game_status_done},
  };
  for (const auto& [name, status] : statuses) {
    if (text == name)
      return status;
  }
  return std::nullopt;
}

// Whether a game shows up in /games/custom at all, whatever the filter.
// Private games only for somebody who knows the host's full name: one who
// knows it can find the game with /whereis anyway. (A prefix is not enough,
// that would let anyone enumerate them.)
bool is_listable(t_game* game, const std::string& owner_name, const std::string& owner_filter) {
  if (!game_get_name(game))
    return false;
  if (game_get_type(game) == game_type_anongame)  // matchmaking: see /games/ladder
    return false;
  if (game_get_flag(game) == game_flag_private &&
      (owner_filter.empty() || owner_name.empty() || strcasecmp(owner_name.c_str(), owner_filter.c_str()) != 0))
    return false;
  return true;
}

CustomGameDto make_custom_game_dto(t_game* game, const std::string& owner_name) {
  CustomGameDto dto;
  dto.id = game_get_id(game);
  dto.name = to_utf8(game_get_name(game));
  if (!owner_name.empty())
    dto.owner = owner_name;
  dto.clienttag = tag_string(game_get_clienttag(game));
  dto.status = game_status_get_str(game_get_status(game));
  dto.is_private = game_get_flag(game) == game_flag_private;
  if (const char* map = game_get_mapname(game))
    dto.map = to_utf8(map);
  if (game_get_mapsize_x(game) && game_get_mapsize_y(game)) {
    dto.map_width = game_get_mapsize_x(game);
    dto.map_height = game_get_mapsize_y(game);
  }
  dto.created = static_cast<std::int64_t>(game_get_create_time(game));
  if (game_get_start_time(game))
    dto.started = static_cast<std::int64_t>(game_get_start_time(game));
  for (unsigned int i = 0; i < game_get_count(game); i++) {
    std::string player = user_name(game_get_connection(game, i));
    if (!player.empty())
      dto.players.push_back(std::move(player));
  }
  if (!prefs_get_hide_addr())
    dto.address = addr_num_to_addr_str(game_get_addr(game), game_get_port(game));
  return dto;
}

int add_custom_game(t_game* game, void* data) {
  const CustomListing& listing = *static_cast<const CustomListing*>(data);
  const CustomFilter& filter = *listing.filter;
  const std::string owner_name = user_name(game_get_owner(game));

  if (!is_listable(game, owner_name, filter.owner))
    return 0;
  if (filter.clienttag && game_get_clienttag(game) != filter.clienttag)
    return 0;
  if (filter.status && game_get_status(game) != *filter.status)
    return 0;
  if (!filter.owner.empty() &&
      (owner_name.empty() || strncasecmp(owner_name.c_str(), filter.owner.c_str(), filter.owner.size()) != 0))
    return 0;
  // the name as stored, or as this API reports it (see to_utf8())
  if (!filter.name.empty() && strcasecmp(game_get_name(game), filter.name.c_str()) != 0 &&
      strcasecmp(to_utf8(game_get_name(game)).c_str(), filter.name.c_str()) != 0)
    return 0;

  listing.response->games.push_back(make_custom_game_dto(game, owner_name));
  return 0;
}

/* ---- /games/ladder/queue ---- */

struct QueueCount {
  unsigned int searching = 0;
  std::time_t oldest = 0;  // search start of the longest waiting entry; 0: none
};

struct QueueListing {
  t_clienttag clienttag;
  std::map<int, QueueCount>* counts;
};

void count_queue_entry(int queue, t_connection* conn, int players, void* data) {
  const QueueListing& listing = *static_cast<const QueueListing*>(data);
  if (listing.clienttag && conn_get_clienttag(conn) != listing.clienttag)
    return;
  QueueCount& count = (*listing.counts)[queue];
  count.searching += static_cast<unsigned int>(players > 0 ? players : 1);
  const std::time_t since = conn_get_anongame_search_starttime(conn);
  if (since && (!count.oldest || since < count.oldest))
    count.oldest = since;
}

// Whether players of `clienttag` (any Warcraft III client if 0) can search `queue`.
bool queue_has_maps(int queue, t_clienttag clienttag) {
  // matchmaking is Warcraft III only; the map lists know no other client
  if (clienttag && clienttag != CLIENTTAG_WAR3XP_UINT && clienttag != CLIENTTAG_WARCRAFT3_UINT)
    return false;
  if (clienttag)
    return maplists_get_totalmaps_by_queue(clienttag, queue) > 0;
  return maplists_get_totalmaps_by_queue(CLIENTTAG_WAR3XP_UINT, queue) > 0 ||
         maplists_get_totalmaps_by_queue(CLIENTTAG_WARCRAFT3_UINT, queue) > 0;
}

/* ---- /games/ladder/in_progress ---- */

const char* match_state(const t_anongameinfo* info) {
  bool joined = true, loaded = true;
  for (int i = 0; i < info->totalplayers && i < ANONGAME_MAX_GAMECOUNT; i++) {
    t_connection* player = info->player[i];
    t_anongame* a = player ? conn_get_anongame(player) : nullptr;
    if (!a)
      continue;
    joined = joined && anongame_get_joined(a);
    loaded = loaded && anongame_get_loaded(a);
  }
  return !joined ? "joining" : !loaded ? "loading" : "playing";
}

}  // namespace

void GamesCtrl::custom(const drogon::HttpRequestPtr& req, Callback&& callback)
{
  // Do not touch bnetd state here, only copy what the job needs out of `req`.
  CustomFilter filter;
  if (!read_clienttag(req, filter.clienttag, callback))
    return;
  if (const std::string& text = req->getParameter("status"); !text.empty()) {
    filter.status = parse_status(text);
    if (!filter.status) {
      callback(error_response(drogon::k400BadRequest, "\"status\" must be open, full, started, loaded or done"));
      return;
    }
  }
  filter.owner = req->getParameter("owner");
  filter.name = req->getParameter("name");

  queue_on_bnetd([filter = std::move(filter)]() {
    // bnetd main thread: bnetd state may be used freely.
    CustomGameListDto response;
    CustomListing listing{&filter, &response};
    gamelist_traverse(add_custom_game, &listing);
    return json_response(response);
  }, std::move(callback));
}

void GamesCtrl::customById(const drogon::HttpRequestPtr& req, Callback&& callback, const std::string& id_text)
{
  unsigned int id = 0;
  const char* const end = id_text.data() + id_text.size();
  if (const auto [ptr, ec] = std::from_chars(id_text.data(), end, id); ec != std::errc() || ptr != end || !id) {
    callback(error_response(drogon::k400BadRequest, "the game id must be a positive number"));
    return;
  }
  std::string owner = req->getParameter("owner");

  queue_on_bnetd([id, owner = std::move(owner)]() {
    // bnetd main thread: bnetd state may be used freely.
    t_game* game = gamelist_find_game_byid(id);
    if (!game)
      return error_response(drogon::k404NotFound, "no such game");
    const std::string owner_name = user_name(game_get_owner(game));
    // a private game is "not found" rather than "forbidden": that would tell it exists
    if (!is_listable(game, owner_name, owner))
      return error_response(drogon::k404NotFound, "no such game");
    return json_response(make_custom_game_dto(game, owner_name));
  }, std::move(callback));
}

void GamesCtrl::ladderQueue(const drogon::HttpRequestPtr& req, Callback&& callback)
{
  t_clienttag clienttag;
  std::optional<int> queue;
  if (!read_clienttag(req, clienttag, callback) || !read_queue(req, queue, callback))
    return;

  queue_on_bnetd([clienttag, queue]() {
    std::map<int, QueueCount> counts;
    QueueListing listing{clienttag, &counts};
    anongame_queue_traverse(count_queue_entry, &listing);

    const std::time_t now = std::time(nullptr);
    LadderQueueListDto response;
    for (int q = 0; q < ANONGAME_TYPES; q++) {
      if (queue && q != *queue)
        continue;
      const QueueCount& count = counts[q];
      // queues nobody can search are left out, unless somebody is in one anyway
      if (!count.searching && !queue_has_maps(q, clienttag) && !queue)
        continue;
      response.queues.push_back(LadderQueueDto{
          std::string(*queue_name(q)), count.searching,
          count.oldest ? static_cast<std::int64_t>(std::max<std::time_t>(0, now - count.oldest)) : 0});
    }
    return json_response(response);
  }, std::move(callback));
}

void GamesCtrl::ladderInProgress(const drogon::HttpRequestPtr& req, Callback&& callback)
{
  t_clienttag clienttag;
  std::optional<int> queue;
  if (!read_clienttag(req, clienttag, callback) || !read_queue(req, queue, callback))
    return;

  queue_on_bnetd([clienttag, queue]() {
    // every player of a match points to the same match info
    std::map<const t_anongameinfo*, int> seen;  // match -> its queue
    t_elem const* curr;
    LIST_TRAVERSE_CONST(connlist(), curr) {
      t_connection* conn = static_cast<t_connection*>(elem_get_data(curr));
      t_anongame* a = conn ? conn_get_anongame(conn) : nullptr;
      if (a && a->info)
        seen.emplace(a->info, anongame_get_queue(a));
    }

    LadderMatchListDto response;
    for (const auto& [info, q] : seen) {
      if (queue && q != *queue)
        continue;
      LadderMatchDto dto;
      dto.queue = std::string(queue_name(q).value_or("unknown"));
      dto.state = match_state(info);
      if (info->mapname)
        dto.map = to_utf8(info->mapname);
      dto.created = static_cast<std::int64_t>(info->created);
      bool tag_matches = !clienttag;
      for (int i = 0; i < info->totalplayers && i < ANONGAME_MAX_GAMECOUNT; i++) {
        t_connection* player = info->player[i];
        if (!player)
          continue;
        tag_matches = tag_matches || conn_get_clienttag(player) == clienttag;
        dto.players.push_back(LadderMatchPlayerDto{user_name(player), tag_string(conn_get_clienttag(player))});
      }
      if (!tag_matches)
        continue;
      response.matches.push_back(std::move(dto));
    }
    return json_response(response);
  }, std::move(callback));
}

}
