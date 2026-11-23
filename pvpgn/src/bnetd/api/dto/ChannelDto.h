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

#ifndef PVPGN_API_DTO_CHANNELDTO_H
#define PVPGN_API_DTO_CHANNELDTO_H

#include <optional>
#include <string>
#include <vector>

namespace pvpgn::bnetd::api {

/* A user in a channel, as the other users in that channel see it. */
struct ChannelMemberDto {
  std::string name;
  /* the client the user shows up as, e.g. "W3XP" or "CHAT" */
  std::string clienttag;
  /* the MF_* bits of bnet_protocol.h: 0x01 Blizzard rep/admin, 0x02 operator, 0x04 voice, 0x20 squelched, ... */
  unsigned int flags;
};

struct ChannelDto {
  std::string name;
  /* the client the channel belongs to; absent for channels open to every client */
  std::optional<std::string> clienttag;
  /* false for a channel that only exists while somebody is in it */
  bool permanent;
  bool moderated;
  std::optional<std::string> topic;
  int member_count;
  /* -1: no limit */
  int max_members;
  /* absent if the request asked for members=false */
  std::optional<std::vector<ChannelMemberDto>> members;
};

/* Response of GET /channels */
struct ChannelListDto {
  std::vector<ChannelDto> channels;
};

}

#endif // PVPGN_API_DTO_CHANNELDTO_H
