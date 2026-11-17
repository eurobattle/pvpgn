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

#ifndef PVPGN_CHANNELSCTL_H
#define PVPGN_CHANNELSCTL_H

#include <functional>

#include <drogon/HttpController.h>

namespace pvpgn::bnetd::api {
/*
 * GET /channels lists the chat channels and who is in them (see
 * dto/ChannelDto.h), read straight from the server state.
 *
 * It needs no authentication and returns what any regular player can see with
 * /channels and by joining: no clan channels, no channels restricted to
 * operators and not The Void.
 *
 * Query parameters, all optional:
 *   clienttag=W3XP   only channels of that client (and those open to all)
 *   name=...         only the channel with that name
 *   active=true      only channels with somebody in them
 *   members=false    leave out the member lists
 *
 * AutoCreation=false: registered explicitly in RestApi() (see api.cpp)
 */
class ChannelsCtrl : public drogon::HttpController<ChannelsCtrl, false>
{
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ChannelsCtrl::list, "/channels", drogon::Get);
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& req,
            std::function<void (const drogon::HttpResponsePtr &)> &&callback);
};
}

#endif // PVPGN_CHANNELSCTL_H
