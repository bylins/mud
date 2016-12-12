if (GIT)
    execute_process(COMMAND ${GIT} show -s --format=%h OUTPUT_VARIABLE REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
else ()
    set(REVISION "<undefined>")
endif ()
configure_file(${VERSION_INPUT} ${VERSION_OUTPUT})
execute_process(COMMAND ${CMAKE_COMMAND} -E touch ${VERSION_OUTPUT})	# invalidate output to force rebuild this source file even if it was not changed.

# vim: set ts=4 sw=4 ai tw=0 et syntax=cmake :
