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

#ifndef PVPGN_API_DISPATCH_H
#define PVPGN_API_DISPATCH_H

#include <functional>

#include <drogon/HttpResponse.h>

namespace pvpgn::bnetd::api {

using ResponseCallback = std::function<void(const drogon::HttpResponsePtr&)>;

/*
 * Call this from a controller (on a Drogon I/O thread) to do the actual work.
 *
 * `work` runs later on the bnetd main thread, where it may use any bnetd state
 * (connections, accounts, games, eventlog, ...); its response is passed to
 * `callback`. Copy whatever `work` needs out of the request beforehand, and
 * keep it short: it runs between network polls of the game server.
 *
 * If bnetd is shutting down, `callback` gets 503 right away and `work` never
 * runs. If `work` throws or returns null, the client gets 500.
 */
void queue_on_bnetd(std::function<drogon::HttpResponsePtr()> work,
                  ResponseCallback&& callback);

}

#endif // PVPGN_API_DISPATCH_H
