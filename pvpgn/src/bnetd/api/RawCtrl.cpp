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
#include "RawCtrl.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "dispatch.h"
#include "json.h"
#include "text.h"
#include "dto/RawCommandDto.h"
#include "common/packet.h"
#include "common/tag.h"
#include "../account.h"
#include "../command.h"
#include "../connection.h"
#include "common/setup_after.h"

namespace pvpgn::bnetd::api {

namespace {

// Owning handles, so the connection and packets are released even if
// handle_command() or collecting its output throws. A virtual connection must
// be gone before control returns to the server loop.
struct VirtualConnDeleter {
  void operator()(t_connection* c) const { conn_destroy_virtual(c); }
};
using VirtualConnPtr = std::unique_ptr<t_connection, VirtualConnDeleter>;

struct PacketDeleter {
  void operator()(t_packet* p) const { packet_del_ref(p); }
};
using PacketPtr = std::unique_ptr<t_packet, PacketDeleter>;

// Moves the text lines a command sent to the virtual connection into `lines`.
void collect_output(t_connection* c, std::vector<std::string>& lines) {
  while (PacketPtr packet{conn_pull_outqueue(c)}) {
    const char* data = static_cast<const char*>(packet_get_raw_data_const(packet.get(), 0));
    const std::size_t size = data ? packet_get_size(packet.get()) : 0;

    // each packet holds NUL terminated text made of "\r\n" terminated lines
    std::string line;
    for (std::size_t i = 0; i < size; i++) {
      const char ch = data[i];
      if (ch == '\n') {
        lines.push_back(to_utf8(line));
        line.clear();
      } else if (ch != '\r' && ch != '\0') {
        line += ch;
      }
    }
    if (!line.empty())
      lines.push_back(to_utf8(line));
  }
}

}  // namespace

void RawCtrl::asyncHandleHttpRequest(const HttpRequestPtr& req,
                                     std::function<void (const HttpResponsePtr &)> &&callback)
{
  // Do not touch bnetd state here, only copy what the job needs out of `req`.
  auto request = read_json<RawCommandRequest>(req);
  if (!request) {
    callback(error_response(k400BadRequest, "invalid request body: " + request.error().what()));
    return;
  }
  if (const auto problem = validate(request.value())) {
    callback(error_response(k400BadRequest, *problem));
    return;
  }

  // validate() made sure the tag is a known one
  const t_clienttag clienttag =
      request.value().clienttag ? parse_clienttag(*request.value().clienttag).value_or(0) : 0;

  queue_on_bnetd([username = authenticated_user(req), command = std::move(request.value().command),
                  clienttag]() mutable {
    // bnetd main thread: bnetd state may be used freely.
    t_account* account = accountlist_find_account(username.c_str());
    if (!account)
      return error_response(k401Unauthorized, "account no longer exists");

    const VirtualConnPtr c{conn_create_virtual(account, clienttag)};
    if (!c)
      return error_response(k500InternalServerError, "could not create a connection for the command");

    // handle_command() takes a const string but the handlers write to it
    handle_command(c.get(), command.data());

    RawCommandResponse response;
    collect_output(c.get(), response.output);

    return json_response(response);
  }, std::move(callback));
}

}
