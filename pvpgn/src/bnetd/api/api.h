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

#ifndef PVPGN_API_H
#define PVPGN_API_H

#include <atomic>
#include <thread>

/*
 * REST API (Drogon) integration.
 *
 * Drogon runs its event loop(s) on its own thread(s), but bnetd's state is
 * single-threaded. Controllers therefore never touch bnetd state directly:
 * they hand their work to queue_on_bnetd() (see dispatch.h), which queues it,
 * and the bnetd main loop executes the queue via process_pending_api_jobs().
 *
 * This header is deliberately free of Drogon includes so that the core bnetd
 * sources only need it for the two hooks below.
 */
namespace pvpgn::bnetd::api {

/*
 * Starts the REST API thread if "api_enable" is set; does nothing otherwise.
 * The destructor answers requests that are still queued with 503, stops
 * Drogon and joins its thread. Create it after the server state is set up and
 * destroy it before that state is torn down.
 */
class RestApi {
public:
  RestApi();
  ~RestApi();

  RestApi(const RestApi&) = delete;
  RestApi& operator=(const RestApi&) = delete;

private:
  std::thread thread_;
  std::atomic<bool> finished_{false};
};

/* Runs the work queued by API handlers. Only call from the bnetd main loop. */
void process_pending_api_jobs();

}

#endif // PVPGN_API_H
