# install_if_missing(DESTINATION <dir> [DIST] FILES <file>...)
#
# Like install(FILES), but never overwrites a file which already exists at the
# destination (checked at install time, honours DESTDIR). With DIST the pristine
# version is also installed, always, as <file>.dist.
function(install_if_missing)
  cmake_parse_arguments(ARG "DIST" "DESTINATION" "FILES" ${ARGN})

  if(IS_ABSOLUTE "${ARG_DESTINATION}")
    set(dest "${ARG_DESTINATION}")
  else()
    set(dest "\${CMAKE_INSTALL_PREFIX}/${ARG_DESTINATION}")
  endif()

  foreach(file ${ARG_FILES})
    get_filename_component(src "${file}" ABSOLUTE)
    get_filename_component(name "${file}" NAME)
    install(CODE "
      set(target \"\$ENV{DESTDIR}${dest}/${name}\")
      if(EXISTS \"\${target}\" OR IS_SYMLINK \"\${target}\")
        message(STATUS \"Keeping existing: \${target}\")
      else()
        file(INSTALL DESTINATION \"${dest}\" TYPE FILE FILES \"${src}\")
      endif()")
    if(ARG_DIST)
      install(FILES "${src}" DESTINATION "${ARG_DESTINATION}" RENAME "${name}.dist")
    endif()
  endforeach()
endfunction()
