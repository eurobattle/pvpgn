if(WITH_API)
  # DrogonConfig.cmake insists on finding the dev packages of every backend
  # Drogon was built with (libpq-dev, libhiredis-dev, ...) although the
  # Drogon::Drogon target does not link against them. Satisfy those checks with
  # stub modules so only the backends we actually use need to be installed.
  set(_PVPGN_SAVED_MODULE_PATH ${CMAKE_MODULE_PATH})
  list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/DrogonStubs)
  # Silence warnings coming from Drogon's own installed CMake files, which we
  # cannot fix:
  #  - FindJsoncpp uses exec_program() (CMP0153). Note this is also why our
  #    cmake_minimum_required must stay below 3.28: that makes it NEW (an error).
  #  - ParseAndAddDrogonTests calls cmake_minimum_required(VERSION 3.5); that
  #    deprecation warning only honors the *cache* entry CMAKE_WARN_DEPRECATED,
  #    so override it temporarily and restore the user's setting afterwards.
  cmake_policy(PUSH)
  if(POLICY CMP0153)
    cmake_policy(SET CMP0153 OLD)
  endif()
  if(DEFINED CACHE{CMAKE_WARN_DEPRECATED})
    set(_PVPGN_SAVED_WARN_DEPRECATED "$CACHE{CMAKE_WARN_DEPRECATED}")
  endif()
  set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
  find_package(Drogon REQUIRED)
  if(DEFINED _PVPGN_SAVED_WARN_DEPRECATED)
    set(CMAKE_WARN_DEPRECATED "${_PVPGN_SAVED_WARN_DEPRECATED}" CACHE BOOL "" FORCE)
    unset(_PVPGN_SAVED_WARN_DEPRECATED)
  else()
    unset(CMAKE_WARN_DEPRECATED CACHE)
  endif()
  cmake_policy(POP)
  set(CMAKE_MODULE_PATH ${_PVPGN_SAVED_MODULE_PATH})
  unset(_PVPGN_SAVED_MODULE_PATH)

  # reflect-cpp converts between JSON and the DTO structs in api/dto. Its
  # installed CMake config insists on the dev packages of every format it
  # supports (BSON, TOML, XML, ...); only JSON is used here, so look up the
  # library and what its JSON headers include (yyjson, ctre) directly.
  find_library(REFLECTCPP_LIBRARY reflectcpp)
  find_path(REFLECTCPP_INCLUDE_DIR rfl/json.hpp)
  find_library(YYJSON_LIBRARY yyjson)
  find_path(YYJSON_INCLUDE_DIR yyjson.h)
  find_path(CTRE_INCLUDE_DIR ctre.hpp)
  foreach(_var REFLECTCPP_LIBRARY REFLECTCPP_INCLUDE_DIR YYJSON_LIBRARY YYJSON_INCLUDE_DIR CTRE_INCLUDE_DIR)
    if(NOT ${_var})
      message(FATAL_ERROR "WITH_API needs reflect-cpp with yyjson and ctre "
        "(Debian: libreflectcpp-dev libyyjson-dev libctre-dev); ${_var} not found")
    endif()
  endforeach()

  # utfcpp (header only): checks that text put into JSON is valid UTF-8
  find_path(UTFCPP_INCLUDE_DIR utf8.h)
  if(NOT UTFCPP_INCLUDE_DIR)
    message(FATAL_ERROR "WITH_API needs utfcpp (Debian: libutfcpp-dev); utf8.h not found")
  endif()
endif(WITH_API)

# storage backends (found in ConfigureChecks.cmake); imported targets carry
# their own include dirs
set(BNETD_STORAGE_LIBRARIES)
if(WITH_MYSQL)
  list(APPEND BNETD_STORAGE_LIBRARIES PkgConfig::MYSQL)
endif(WITH_MYSQL)
if(WITH_SQLITE3)
  list(APPEND BNETD_STORAGE_LIBRARIES SQLite::SQLite3)
endif(WITH_SQLITE3)
if(WITH_PGSQL)
  list(APPEND BNETD_STORAGE_LIBRARIES PostgreSQL::PostgreSQL)
endif(WITH_PGSQL)

set(BNETD_SOURCES
	account.cpp account.h account_wrap.cpp account_wrap.h adbanner.cpp
	adbanner.h alias_command.cpp alias_command.h anongame.cpp
	anongame_gameresult.cpp anongame_gameresult.h anongame.h
	anongame_infos.cpp anongame_infos.h anongame_maplists.cpp
	anongame_maplists.h attrgroup.cpp attrgroup.h attr.h attrlayer.cpp
	attrlayer.h autoupdate.cpp autoupdate.h channel_conv.cpp channel_conv.h
	channel.cpp channel.h character.cpp character.h clan.cpp clan.h
	cmdline.cpp cmdline.h command.cpp command_groups.cpp command_groups.h
	command.h connection.cpp connection.h file_cdb.cpp file_cdb.h file.cpp
	file.h file_plain.cpp file_plain.h friends.cpp friends.h game_conv.cpp
	game_conv.h game.cpp game.h handle_anongame.cpp handle_anongame.h
	handle_apireg.cpp handle_apireg.h handle_bnet.cpp handle_bnet.h
	handle_bot.cpp handle_bot.h handle_d2cs.cpp handle_d2cs.h
	handle_file.cpp handle_file.h handle_init.cpp handle_init.h
	handle_irc_common.cpp handle_irc_common.h handle_irc.cpp handle_irc.h
	handlers.h handle_telnet.cpp handle_telnet.h handle_udp.cpp
	handle_udp.h handle_wol.cpp handle_wol.h handle_wol_gameres.cpp
    handle_wol_gameres.h helpfile.cpp helpfile.h
	ipban.cpp ipban.h irc.cpp irc.h ladder_calc.cpp ladder_calc.h ladder.cpp
	ladder.h mail.cpp mail.h main.cpp message.cpp message.h news.cpp news.h
	output.cpp output.h prefs.cpp prefs.h quota.h realm.cpp realm.h
	runprog.cpp runprog.h server.cpp server.h sql_common.cpp sql_common.h
	sql_dbcreator.cpp sql_dbcreator.h sql_mysql.cpp sql_mysql.h sql_odbc.cpp
	sql_odbc.h sql_pgsql.cpp sql_pgsql.h sql_sqlite3.cpp sql_sqlite3.h
	storage.cpp storage_file.cpp storage_file.h storage.h storage_sql2.cpp
	storage_sql2.h storage_sql.cpp storage_sql.h support.cpp support.h
	team.cpp team.h tick.cpp tick.h timer.cpp timer.h topic.cpp topic.h
	tournament.cpp tournament.h tracker.cpp tracker.h udptest_send.cpp
	udptest_send.h versioncheck.cpp versioncheck.h watch.cpp watch.h
	anongame_wol.cpp anongame_wol.h handle_wserv.cpp handle_wserv.h
	../win32/winmain.cpp ../win32/winmain.h
)

if(WITH_API)
  list(APPEND BNETD_SOURCES
	api/api.cpp api/api.h api/dispatch.h api/GamesCtrl.cpp api/GamesCtrl.h
	api/RawCtrl.cpp api/RawCtrl.h
	api/BasicAuthFilter.cpp api/BasicAuthFilter.h api/json.h api/text.h
	api/ChannelsCtrl.cpp api/ChannelsCtrl.h
	api/ladder.h api/dto/ChannelDto.h api/dto/ErrorDto.h api/dto/GameDto.h api/dto/RawCommandDto.h
  )
endif(WITH_API)

set(BNETD_CONSOLE_RESOURCES
  ../win32/console_resource.h ../win32/console_resource.rc)

set(BNETD_GUI_RESOURCES
  ../win32/resource.h ../win32/resource.rc)

if(WITH_WIN32_GUI)
  add_executable(bnetd WIN32 ${BNETD_SOURCES} ${BNETD_GUI_RESOURCES})
else(WITH_WIN32_GUI)  
  add_executable(bnetd ${BNETD_SOURCES} ${BNETD_CONSOLE_RESOURCES})
endif(WITH_WIN32_GUI)

target_link_libraries(bnetd common compat win32 tinycdb ${NETWORK_LIBRARIES}
    ZLIB::ZLIB ${BNETD_STORAGE_LIBRARIES})

if(WITH_API)
  target_compile_definitions(bnetd PRIVATE WITH_API)
  target_link_libraries(bnetd Drogon::Drogon ${REFLECTCPP_LIBRARY} ${YYJSON_LIBRARY})
  target_include_directories(bnetd PRIVATE
    ${REFLECTCPP_INCLUDE_DIR} ${YYJSON_INCLUDE_DIR} ${CTRE_INCLUDE_DIR} ${UTFCPP_INCLUDE_DIR})
endif(WITH_API)

install(TARGETS bnetd DESTINATION ${SBINDIR})
