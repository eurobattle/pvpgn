include(${PROJECT_SOURCE_DIR}/cmake/InstallIfMissing.cmake)

# generate the configs with proper line endings
set(OUTPUT_CONFS ad.conf anongame_infos.conf address_translation.conf 
  autoupdate.conf bnalias.conf bnban.conf bnetd_default_user.plain 
  bnhelp.conf bnissue.txt bnmaps.conf bnxpcalc.conf bnmotd-enUS.txt 
  bnmotd-csCZ.txt bnmotd-deDE.txt bnmotd-esES.txt bnmotd-frFR.txt 
  bnmotd-nlNL.txt bnmotd-plPL.txt bnmotd-ruRU.txt bnmotd-zhCN.txt 
  bnmotd-zhTW.txt bnmotd-ptBR.txt 
  bnxplevel.conf channel.conf command_groups.conf news.txt realm.conf 
  sql_DB_layout2.conf sql_DB_layout.conf supportfile.conf topics.conf 
  tournament.conf versioncheck.conf)
foreach(CONF ${OUTPUT_CONFS})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${CONF}.in ${CMAKE_CURRENT_BINARY_DIR}/${CONF} @ONLY)
endforeach(CONF)

# generate the config files replacing the path variables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bnetd.conf.in ${CMAKE_CURRENT_BINARY_DIR}/bnetd.conf)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/d2cs.conf.in ${CMAKE_CURRENT_BINARY_DIR}/d2cs.conf)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/d2dbs.conf.in ${CMAKE_CURRENT_BINARY_DIR}/d2dbs.conf)

if(WITH_BNETD)
  set(BNETD_CONFS bnetd.conf ad.conf anongame_infos.conf address_translation.conf 
  autoupdate.conf bnalias.conf bnban.conf 
  bnetd_default_user.plain bnhelp.conf bnissue.txt bnmaps.conf bnmotd-enUS.txt 
  bnmotd-csCZ.txt bnmotd-deDE.txt bnmotd-esES.txt bnmotd-frFR.txt bnmotd-nlNL.txt
  bnmotd-plPL.txt bnmotd-ruRU.txt bnmotd-zhCN.txt bnmotd-zhTW.txt bnmotd-ptBR.txt  
  bnxpcalc.conf bnxplevel.conf channel.conf command_groups.conf news.txt 
  realm.conf supportfile.conf topics.conf tournament.conf versioncheck.conf)

  # not configuration: bnetd uses these to add missing tables and columns at
  # startup, so they have to match the installed binary and are always replaced
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sql_DB_layout.conf
    ${CMAKE_CURRENT_BINARY_DIR}/sql_DB_layout2.conf DESTINATION ${SYSCONFDIR})

  # special treatement for non .in files
  install_if_missing(DIST DESTINATION ${SYSCONFDIR} FILES bnetd_default_user.cdb)
endif(WITH_BNETD)

if(WITH_D2CS)
  set(D2CS_CONFS d2cs.conf anongame_infos.conf)

  # special treatement for non .in files
  install_if_missing(DIST DESTINATION ${SYSCONFDIR} FILES d2server.ini)
endif(WITH_D2CS)

if(WITH_D2DBS)
  set(D2DBS_CONFS d2dbs.conf)
endif(WITH_D2DBS)

# existing files are kept, the stock version is installed as <name>.dist
set(INSTALL_CONFS ${BNETD_CONFS} ${D2CS_CONFS} ${D2DBS_CONFS})
list(REMOVE_DUPLICATES INSTALL_CONFS)
foreach(CONF_FILE ${INSTALL_CONFS})
  install_if_missing(DIST DESTINATION ${SYSCONFDIR} FILES ${CMAKE_CURRENT_BINARY_DIR}/${CONF_FILE})
endforeach(CONF_FILE)
