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
#include "ChannelsCtrl.h"

#include <optional>
#include <string>
#include <utility>

#include "dispatch.h"
#include "json.h"
#include "text.h"
#include "dto/ChannelDto.h"
#include "compat/strcasecmp.h"
#include "common/list.h"
#include "common/tag.h"
#include "../channel.h"
#include "../connection.h"
#include "../prefs.h"
#include "../topic.h"
#include "common/setup_after.h"

namespace pvpgn::bnetd::api {

namespace {

struct ChannelFilter {
  t_clienttag clienttag = 0;  // 0: any
  std::string name;           // empty: any
  bool active = false;
  bool members = true;
};

// Reads a true/false query parameter into `value`; false if it is neither.
bool parse_flag(const std::string& text, bool& value) {
  if (text == "true" || text == "1")
    value = true;
  else if (text == "false" || text == "0")
    value = false;
  else
    return false;
  return true;
}

std::string tag_string(t_clienttag tag) {
  char text[5];
  return tag_uint_to_str(text, tag);
}

// The rules of the /channels command for a user who is neither operator nor admin.
bool visible_to_players(const t_channel* channel, const ChannelFilter& filter) {
  const unsigned flags = channel_get_flags(channel);

  if (flags & (channel_flags_clan | channel_flags_thevoid))
    return false;
  if (channel_get_max(channel) == 0)  // restricted to operators
    return false;
  if (filter.clienttag) {
    if (prefs_get_hide_temp_channels() && !channel_get_permanent(channel))
      return false;
    if (channel_get_clienttag(channel) && channel_get_clienttag(channel) != filter.clienttag)
      return false;
  }
  return true;
}

ChannelDto to_dto(const t_channel* channel, bool with_members) {
  ChannelDto dto;
  const char* name = channel_get_name(channel);

  dto.name = to_utf8(name);
  if (channel_get_clienttag(channel))
    dto.clienttag = tag_string(channel_get_clienttag(channel));
  dto.permanent = channel_get_permanent(channel) != 0;
  dto.moderated = (channel_get_flags(channel) & channel_flags_moderated) != 0;
  if (const char* topic = name ? channel_get_topic(name) : nullptr)
    dto.topic = to_utf8(topic);
  dto.member_count = channel_get_length(channel);
  dto.max_members = channel_get_max(channel);

  if (with_members) {
    dto.members.emplace();
    for (t_connection* conn = channel_get_first(channel); conn; conn = channel_get_next()) {
      const char* chatname = conn_get_chatname(conn);
      if (!chatname)
        continue;
      dto.members->push_back(
          ChannelMemberDto{to_utf8(chatname), tag_string(conn_get_fake_clienttag(conn)), conn_get_flags(conn)});
      conn_unget_chatname(conn, chatname);
    }
  }
  return dto;
}

}  // namespace

void ChannelsCtrl::list(const drogon::HttpRequestPtr& req,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback)
{
  // Do not touch bnetd state here, only copy what the job needs out of `req`.
  ChannelFilter filter;

  if (const std::string& text = req->getParameter("clienttag"); !text.empty()) {
    const std::optional<t_clienttag> tag = parse_clienttag(text);
    if (!tag) {
      callback(error_response(drogon::k400BadRequest, "\"clienttag\" is not a known client tag"));
      return;
    }
    filter.clienttag = *tag;
  }
  filter.name = req->getParameter("name");
  if (const std::string& text = req->getParameter("active"); !text.empty() && !parse_flag(text, filter.active)) {
    callback(error_response(drogon::k400BadRequest, "\"active\" must be true or false"));
    return;
  }
  if (const std::string& text = req->getParameter("members"); !text.empty() && !parse_flag(text, filter.members)) {
    callback(error_response(drogon::k400BadRequest, "\"members\" must be true or false"));
    return;
  }

  queue_on_bnetd([filter = std::move(filter)]() {
    // bnetd main thread: bnetd state may be used freely.
    ChannelListDto response;
    t_elem const* curr;

    LIST_TRAVERSE_CONST(channellist(), curr) {
      const t_channel* channel = static_cast<const t_channel*>(elem_get_data(curr));
      if (!channel || !channel_get_name(channel))
        continue;
      if (!visible_to_players(channel, filter))
        continue;
      // the name as stored, or as this API reports it (see to_utf8())
      if (!filter.name.empty() && strcasecmp(channel_get_name(channel), filter.name.c_str()) != 0 &&
          strcasecmp(to_utf8(channel_get_name(channel)).c_str(), filter.name.c_str()) != 0)
        continue;
      if (filter.active && channel_get_length(channel) < 1)
        continue;
      response.channels.push_back(to_dto(channel, filter.members));
    }
    return json_response(response);
  }, std::move(callback));
}

}
