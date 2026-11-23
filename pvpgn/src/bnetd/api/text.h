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

#ifndef PVPGN_API_TEXT_H
#define PVPGN_API_TEXT_H

#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include <utf8.h>

#include "common/tag.h"

/* Helpers for the text that goes in and out of the REST API. */
namespace pvpgn::bnetd::api {

/*
 * bnetd text (names, topics, command output) is whatever bytes the clients
 * sent, but JSON has to be UTF-8 and the JSON writer refuses anything else.
 * Valid UTF-8 (what Warcraft III sends) is kept as it is. A byte that is not
 * part of a valid UTF-8 character is taken to be Latin-1, the usual encoding
 * of the older clients, and becomes the character with that number. So a
 * Latin-1 name is converted, and a UTF-8 text that bnetd cut off in the middle
 * of a character only gets its last character wrong.
 */
inline std::string to_utf8(std::string_view text) {
  std::string out;
  out.reserve(text.size() * 2);

  auto it = text.begin();
  while (it != text.end()) {
    const auto invalid = utf8::find_invalid(it, text.end());
    out.append(it, invalid);
    if (invalid == text.end())
      break;

    utf8::append(static_cast<char32_t>(static_cast<unsigned char>(*invalid)), std::back_inserter(out));
    it = invalid + 1;
  }
  return out;
}

inline std::string to_utf8(const char* text) {
  return text ? to_utf8(std::string_view(text)) : std::string();
}

/* The client tag named by `text` ("W3XP", any case), or nothing if it is not a known one. */
inline std::optional<t_clienttag> parse_clienttag(std::string_view text) {
  if (text.size() != 4)
    return std::nullopt;
  for (const char ch : text) {
    if (static_cast<unsigned char>(ch) <= 0x20 || static_cast<unsigned char>(ch) >= 0x7f)
      return std::nullopt;
  }
  const t_clienttag tag = tag_case_str_to_uint(std::string(text).c_str());
  if (!tag || !tag_check_client(tag))
    return std::nullopt;
  return tag;
}

}

#endif // PVPGN_API_TEXT_H
