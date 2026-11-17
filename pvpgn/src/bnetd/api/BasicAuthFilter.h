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

#ifndef PVPGN_API_BASICAUTHFILTER_H
#define PVPGN_API_BASICAUTHFILTER_H

#include <string>

#include <drogon/HttpFilter.h>

namespace pvpgn::bnetd::api {

/*
 * HTTP Basic authentication against the bnetd accounts. Put it on a route to
 * require a valid bnetd username and password:
 *
 *   PATH_ADD("/something", Post, BasicAuthFilter::NAME);
 *
 * Requests without valid credentials (or for a locked account) are answered
 * with 401 and never reach the controller. The password check needs bnetd
 * state, so it runs on the bnetd main thread.
 *
 * NOTE: once the check passed, Drogon continues with the controller on the
 * request's I/O thread, not on the bnetd thread the check ran on. Controllers
 * must hand their work to queue_on_bnetd() again.
 *
 * AutoCreation=false: registered explicitly in RestApi() (see api.cpp)
 */
class BasicAuthFilter : public drogon::HttpFilter<BasicAuthFilter, false>
{
public:
  static constexpr const char* NAME = "pvpgn::bnetd::api::BasicAuthFilter";

  void doFilter(const drogon::HttpRequestPtr& req,
                drogon::FilterCallback&& fcb,
                drogon::FilterChainCallback&& fccb) override;
};

/*
 * Name of the account `req` was authenticated as by BasicAuthFilter, or an
 * empty string if the route is not behind the filter. Look the account up
 * again (on the bnetd thread) before using it: it may be gone by then.
 */
std::string authenticated_user(const drogon::HttpRequestPtr& req);

}

#endif // PVPGN_API_BASICAUTHFILTER_H
