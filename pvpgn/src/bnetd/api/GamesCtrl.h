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

#ifndef PVPGN_GAMESCTL_H
#define PVPGN_GAMESCTL_H

#include <functional>
#include <string>

#include <drogon/HttpController.h>

namespace pvpgn::bnetd::api {
/*
 * Games, read straight from the server state (see dto/GameDto.h). No
 * authentication: only what a regular player could find out is returned.
 *
 * GET /games/custom              games created by players and hostbots
 *   clienttag=W3XP                 only games of that client
 *   status=open|full|started|loaded|done
 *   owner=lagabuse.com.            only games whose host's name starts with this
 *                                  (case insensitive); private games are only
 *                                  listed if this is the host's full name
 *   name=...                       only games with that name (case insensitive
 *                                  for ASCII); the name as the API returns it
 *                                  or as bnetd stores it both work
 *
 * GET /games/custom/{id}         one game, by the "id" a listing returned;
 *                                404 if there is no such game (or it is a
 *                                matchmaking game)
 *   owner=...                      the host's full name, needed for a private
 *                                  game (ids are sequential, so they could be
 *                                  enumerated otherwise)
 *
 * GET /games/ladder/queue        how many players search each matchmaking queue
 *   clienttag=W3XP                 only players of that client, and only the
 *                                  queues with maps for it
 *   queue=1v1                      only that queue
 *
 * GET /games/ladder/in_progress  matches made by matchmaking
 *   clienttag=W3XP, queue=1v1      as above
 *
 * AutoCreation=false: registered explicitly in RestApi() (see api.cpp)
 */
class GamesCtrl : public drogon::HttpController<GamesCtrl, false>
{
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(GamesCtrl::custom, "/games/custom", drogon::Get);
  ADD_METHOD_TO(GamesCtrl::customById, "/games/custom/{1}", drogon::Get);
  ADD_METHOD_TO(GamesCtrl::ladderQueue, "/games/ladder/queue", drogon::Get);
  ADD_METHOD_TO(GamesCtrl::ladderInProgress, "/games/ladder/in_progress", drogon::Get);
  METHOD_LIST_END

  void custom(const drogon::HttpRequestPtr& req,
              std::function<void (const drogon::HttpResponsePtr &)> &&callback);
  void customById(const drogon::HttpRequestPtr& req,
                  std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                  const std::string& id);
  void ladderQueue(const drogon::HttpRequestPtr& req,
                   std::function<void (const drogon::HttpResponsePtr &)> &&callback);
  void ladderInProgress(const drogon::HttpRequestPtr& req,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback);
};
}

#endif // PVPGN_GAMESCTL_H
