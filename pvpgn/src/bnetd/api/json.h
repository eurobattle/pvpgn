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

#ifndef PVPGN_API_JSON_H
#define PVPGN_API_JSON_H

#include <string>

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <rfl.hpp>
#include <rfl/json.hpp>

#include "dto/ErrorDto.h"

/*
 * JSON <-> DTO conversion for the controllers. A DTO is a plain struct (see
 * api/dto); reflect-cpp derives the JSON object from its field names, so no
 * per-field code is needed.
 */
namespace pvpgn::bnetd::api {

/* Response with `dto` as its JSON body. */
template <class T>
drogon::HttpResponsePtr json_response(const T& dto, drogon::HttpStatusCode code = drogon::k200OK) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(code);
  resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  resp->setBody(rfl::json::write(dto));
  return resp;
}

inline drogon::HttpResponsePtr error_response(drogon::HttpStatusCode code, std::string message) {
  return json_response(ErrorDto{std::move(message)}, code);
}

/*
 * Parses the body of `req` into a T. Fields the struct does not have are
 * ignored; a missing field or a wrong type is an error. On failure the result
 * is false and its error().what() says what is wrong with the body.
 */
template <class T>
rfl::Result<T> read_json(const drogon::HttpRequestPtr& req) {
  return rfl::json::read<T>(req->body());
}

}

#endif // PVPGN_API_JSON_H
