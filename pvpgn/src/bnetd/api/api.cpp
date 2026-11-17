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
#include "api.h"

#include <chrono>
#include <cstddef>
#include <deque>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>

#ifndef WIN32
# include <csignal>  // pthread_sigmask
#endif

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include "dispatch.h"
#include "common/eventlog.h"
#include "../prefs.h"
#include "BasicAuthFilter.h"
#include "ChannelsCtrl.h"
#include "GamesCtrl.h"
#include "RawCtrl.h"
#include "common/setup_after.h"

namespace pvpgn::bnetd::api {

namespace {

struct Job {
  std::function<drogon::HttpResponsePtr()> work;
  ResponseCallback callback;
};

// Requests waiting beyond this many are refused with 503, so HTTP traffic
// cannot pile up unbounded work (and memory) for the game server.
constexpr std::size_t MAX_QUEUED_JOBS = 1000;
// Time the API jobs may take per server loop iteration (which polls the network
// every BNETD_POLL_INTERVAL = 20 ms). Once it is used up, the remaining jobs
// wait for the next iteration, so API traffic cannot make live connections lag.
// It is checked between jobs: one job always runs, and a slow one can overrun.
constexpr std::chrono::milliseconds JOB_TIME_BUDGET{5};

// Shared between the Drogon thread(s) (producers) and the bnetd thread
// (consumer). `accepting` is only true while the API thread is up.
std::mutex queue_mutex;
std::deque<Job> queue;
bool accepting = false;

drogon::HttpResponsePtr create_status_response(drogon::HttpStatusCode code) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(code);
  return resp;
}

}  // namespace

void queue_on_bnetd(std::function<drogon::HttpResponsePtr()> work,
                  ResponseCallback&& callback) {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (accepting && queue.size() < MAX_QUEUED_JOBS) {
      queue.push_back(Job{std::move(work), std::move(callback)});
      return;
    }
  }
  // shutting down or too busy
  callback(create_status_response(drogon::k503ServiceUnavailable));
}

// Invoked by bnetd main server loop (server.cpp)
void process_pending_api_jobs() {
  const auto deadline = std::chrono::steady_clock::now() + JOB_TIME_BUDGET;

  do {
    Job job;
    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      if (queue.empty())
        return;
      job = std::move(queue.front());
      queue.pop_front();
    }

    drogon::HttpResponsePtr resp;
    try {
      resp = job.work();
    } catch (const std::exception& ex) {
      eventlog(eventlog_level_error, __FUNCTION__, "API request failed: %s", ex.what());
    } catch (...) {
      eventlog(eventlog_level_error, __FUNCTION__, "API request failed with unknown exception");
    }
    if (!resp)
      resp = create_status_response(drogon::k500InternalServerError);
    // Drogon's response callback may be invoked from any thread.
    job.callback(resp);
  } while (std::chrono::steady_clock::now() < deadline);
}

RestApi::RestApi() {
  if (!prefs_get_api_enable())
    return;

  const char* addr = prefs_get_api_listen_addr();
  const unsigned int port = prefs_get_api_listen_port();
  if (!addr || !*addr || port == 0 || port > 65535) {
    eventlog(eventlog_level_error, __FUNCTION__, "invalid api_listen_addr/api_listen_port, REST API disabled");
    return;
  }

  // Drogon wants the log directory and the file base name separately (it
  // appends ".log" itself), while "apilogfile" is a path such as
  // "logs/api_bnetd.log". Drogon exit()s the whole process if the directory
  // is missing, so check it here and only disable the API instead.
  const std::filesystem::path logfile(prefs_get_api_logfile());
  std::filesystem::path logdir = logfile.parent_path();
  if (logdir.empty())
    logdir = ".";
  std::error_code ec;
  if (!std::filesystem::is_directory(logdir, ec)) {
    eventlog(eventlog_level_error, __FUNCTION__, "apilogfile directory \"%s\" does not exist, REST API disabled",
             logdir.string().c_str());
    return;
  }

  drogon::app()
      .disableSigtermHandling()  // signals belong to bnetd
      .setLogPath(logdir.string(), logfile.stem().string())
      .setLogLevel(trantor::Logger::kWarn)
      .addListener(addr, static_cast<uint16_t>(port))
      .setThreadNum(prefs_get_api_threads())
      // Drogon serves files from its document root (the working directory,
      // "/" when daemonized) for requests no route matches: turn that off.
      .setFileTypes({})
      .setImplicitPageEnable(false)
      // All filters and controllers are manually registered here
      .registerFilter(std::make_shared<BasicAuthFilter>())
      .registerController(std::make_shared<RawCtrl>())
      .registerController(std::make_shared<ChannelsCtrl>())
      .registerController(std::make_shared<GamesCtrl>());

  // Not technically needed, for consistency, bool should only change under lock
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    accepting = true;
  }

#ifndef WIN32
  // Threads inherit the creating thread's signal mask: block everything so
  // signals keep being delivered to the bnetd main thread only.
  sigset_t all, old;
  sigfillset(&all);
  pthread_sigmask(SIG_BLOCK, &all, &old);
#endif
  try {
    thread_ = std::thread([this] {
      drogon::app().run();
      finished_ = true;
    });
  } catch (const std::system_error& ex) {
    eventlog(eventlog_level_error, __FUNCTION__, "could not start REST API thread: %s", ex.what());
    std::lock_guard<std::mutex> lock(queue_mutex);
    accepting = false;
  }
#ifndef WIN32
  pthread_sigmask(SIG_SETMASK, &old, NULL);
#endif

  if (thread_.joinable())
    eventlog(eventlog_level_info, __FUNCTION__, "REST API listening on %s:%u", addr, port);
}

RestApi::~RestApi() {
  if (!thread_.joinable())
    return;

  // Stop accepting work and answer what is still queued; the bnetd loop no
  // longer runs, so those jobs would never be executed.
  std::deque<Job> leftover;
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    accepting = false;
    leftover.swap(queue);
  }
  for (auto& job : leftover)
    job.callback(create_status_response(drogon::k503ServiceUnavailable));

  // quit() is lost if Drogon's loop has not started yet (e.g. bnetd failed
  // right after startup), so wait until it actually runs.
  while (!finished_ && !drogon::app().getLoop()->isRunning())
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  drogon::app().quit();
  thread_.join();
  eventlog(eventlog_level_info, __FUNCTION__, "REST API stopped");
}

}
