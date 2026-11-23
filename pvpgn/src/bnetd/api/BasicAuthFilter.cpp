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
#include "BasicAuthFilter.h"

#include <cctype>
#include <memory>
#include <utility>

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>

#include "dispatch.h"
#include "common/bnethash.h"
#include "common/eventlog.h"
#include "../account.h"
#include "../account_wrap.h"
#include "common/setup_after.h"

namespace pvpgn::bnetd::api {

namespace {

const char* const USER_ATTRIBUTE = "pvpgn.api.user";

// Far above what bnetd accepts for either; only bounds the work per request.
const std::size_t MAX_CREDENTIAL_LEN = 128;

drogon::HttpResponsePtr unauthorized() {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(drogon::k401Unauthorized);
  resp->addHeader("WWW-Authenticate", "Basic realm=\"bnetd\"");
  return resp;
}

// Extracts username and password from an "Authorization: Basic ..." header.
bool parse_basic_auth(const std::string& header, std::string& username, std::string& password) {
  static const char scheme[] = "basic ";
  const std::size_t scheme_len = sizeof(scheme) - 1;

  if (header.size() <= scheme_len)
    return false;
  for (std::size_t i = 0; i < scheme_len; i++)
    if (std::tolower(static_cast<unsigned char>(header[i])) != scheme[i])
      return false;

  const std::string decoded = drogon::utils::base64Decode(std::string_view(header).substr(scheme_len));
  const std::size_t colon = decoded.find(':');
  if (colon == std::string::npos || colon == 0)
    return false;

  username = decoded.substr(0, colon);
  password = decoded.substr(colon + 1);
  // an embedded NUL would cut the C strings bnetd works with short
  if (username.find('\0') != std::string::npos || password.find('\0') != std::string::npos)
    return false;
  return username.size() <= MAX_CREDENTIAL_LEN && password.size() <= MAX_CREDENTIAL_LEN;
}

// bnetd main thread only. Password and account lock, like a game client login.
// The REST API is its own access path: auth\botlogin (telnet/bot chat
// logins) deliberately does not apply.
bool check_credentials(const std::string& username, std::string password, const char*& reason) {
  t_account* account = accountlist_find_account(username.c_str());
  if (!account) {
    reason = "unknown account";
    return false;
  }

  const char* stored = account_get_pass(account);
  t_hash stored_hash;
  if (!stored || hash_set_str(&stored_hash, stored) < 0) {
    reason = "account has no usable password hash";
    return false;
  }

  // bnetd passwords are case insensitive: the hash is of the lower case form
  for (char& ch : password)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  t_hash try_hash;
  if (bnet_hash(&try_hash, static_cast<unsigned int>(password.size()), password.c_str()) < 0 ||
      hash_eq(try_hash, stored_hash) != 1) {
    reason = "wrong password";
    return false;
  }

  if (account_get_auth_lock(account) == 1) {
    reason = "account is locked";
    return false;
  }
  return true;
}

}  // namespace

void BasicAuthFilter::doFilter(const drogon::HttpRequestPtr& req,
                               drogon::FilterCallback&& fcb,
                               drogon::FilterChainCallback&& fccb) {
  // Drogon I/O thread: only look at the request here.
  std::string username, password;
  if (!parse_basic_auth(req->getHeader("authorization"), username, password)) {
    fcb(unauthorized());
    return;
  }
  std::string peer = req->peerAddr().toIp();

  // Set by the job below if the credentials are good; the response it returns
  // is only used otherwise (as is the 503/500 queue_on_bnetd() may produce).
  auto authenticated = std::make_shared<bool>(false);

  queue_on_bnetd(
      [req, authenticated, username = std::move(username), password = std::move(password),
       peer = std::move(peer)]() {
        const char* reason = "";
        if (!check_credentials(username, password, reason)) {
          eventlog(eventlog_level_info, "BasicAuthFilter", "API login for \"%s\" from %s refused (%s)",
                   username.c_str(), peer.c_str(), reason);
          return unauthorized();
        }
        req->attributes()->insert(USER_ATTRIBUTE, username);
        *authenticated = true;
        return drogon::HttpResponse::newHttpResponse();
      },
      [authenticated, fcb = std::move(fcb), fccb = std::move(fccb)](const drogon::HttpResponsePtr& resp) {
        if (*authenticated)
          fccb();
        else
          fcb(resp);
      });
}

std::string authenticated_user(const drogon::HttpRequestPtr& req) {
  return req->attributes()->get<std::string>(USER_ATTRIBUTE);
}

}
