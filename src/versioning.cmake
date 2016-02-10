if (HG)
    execute_process(COMMAND ${HG} id OUTPUT_VARIABLE REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
else ()
    set(REVISION "<undefined>")
endif ()
configure_file(${INPUT_FILE} ${OUTPUT_FILE})

# vim: set ts=4 sw=4 ai tw=0 et syntax=cmake :
