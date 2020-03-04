/*
* Copyright (C) 2020 cen1
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
#define GAME_INTERNAL_ACCESS
#include "common/setup_before.h"
#include "pf.h"
#include "common/addr.h"
#include "nlohmann/json.hpp"
#include <curl/curl.h>
#include "common/eventlog.h"
#include "common/setup_after.h"

using json = nlohmann::json;

typedef size_t(*CURL_WRITEFUNCTION_PTR)(char*, size_t, size_t, void*);

namespace pvpgn
{

namespace bnetd
{

extern uint16_t pf_get_port(std::string ip) {
	return 0;
}

extern void pf_replace_host(t_game *game, std::string realIp) {

	json j = {
		{ "ipof", realIp }
	};

	std::string apiUrl = "http://pf2.xpam.pl";

	CURL *curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl) {
		std::string readBuffer;

		auto curl_callback = [](char *contents, size_t size, size_t nmemb, void *userp) -> size_t {
			((std::string*)userp)->append((char*)contents, size * nmemb);
			return size * nmemb;
		};

		std::string payload = j.dump();
		char *cstrPayload = &payload[0];
		eventlog(eventlog_level_debug, __FUNCTION__, "Pf: request payload %s", cstrPayload);

		struct curl_slist *chunk = NULL;
		chunk = curl_slist_append(chunk, "Content-Type: application/json");

		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, apiUrl + "/user.php");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cstrPayload);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, static_cast<CURL_WRITEFUNCTION_PTR>(curl_callback));
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		long httpCode(0);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			eventlog(eventlog_level_debug, __FUNCTION__, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return;
		}
		else {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
			if (httpCode == 200) {
				eventlog(eventlog_level_debug, __FUNCTION__, readBuffer.c_str());
				json response = json::parse(readBuffer);

				char newIp[] = "82.77.63.49";
				int newPort = response["port"].get<int>();

				eventlog(eventlog_level_debug, __FUNCTION__, "Pf: new host IP is %s", newIp);
				eventlog(eventlog_level_debug, __FUNCTION__, "Pf: new host port is %i", newPort);

				game->addr = pvpgn::ip_str_to_addr_num(newIp);
				game->port = newPort;
			}
			else {
				eventlog(eventlog_level_error, __FUNCTION__, "Pf: backend returned code != 200: %s", httpCode);
			}
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
}
}
}