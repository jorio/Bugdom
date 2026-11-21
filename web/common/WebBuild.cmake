# Identical across the Pangea web ports; included only for Emscripten.
	set_target_properties(${GAME_TARGET} PROPERTIES SUFFIX ".html" OUTPUT_NAME "index")
	target_link_options(${GAME_TARGET} PRIVATE
		-fexceptions
		-sASYNCIFY=1
		-sASYNCIFY_STACK_SIZE=131072
		-sSTACK_SIZE=1048576
		-sALLOW_MEMORY_GROWTH=1
		-sINITIAL_MEMORY=134217728
		-sLEGACY_GL_EMULATION=1
		-sGL_UNSAFE_OPTS=0
		-sFORCE_FILESYSTEM=1
		-sEXIT_RUNTIME=1
		"-sEXPORTED_RUNTIME_METHODS=['callMain','FS']"
		-lidbfs.js
		"SHELL:--preload-file \"${GAME_DATADIR}@/Data\""
		"SHELL:--shell-file \"${CMAKE_SOURCE_DIR}/web/common/shell.html\""
	)
	set_property(TARGET ${GAME_TARGET} APPEND PROPERTY LINK_DEPENDS
		"${CMAKE_SOURCE_DIR}/web/common/shell.html")
	file(GLOB_RECURSE WEB_ASSETS CONFIGURE_DEPENDS "${GAME_DATADIR}/*")
	set_property(TARGET ${GAME_TARGET} APPEND PROPERTY LINK_DEPENDS ${WEB_ASSETS})

    target_sources(${GAME_TARGET} PRIVATE
        "${CMAKE_SOURCE_DIR}/web/common/platform.c"
        "${CMAKE_SOURCE_DIR}/web/common/WebTexture.c"
        "${CMAKE_SOURCE_DIR}/web/common/gl_compat.c")
    target_include_directories(${GAME_TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/web/common")
    foreach(asset controls.js touch.js style.css)
        configure_file("web/common/${asset}" "web/common/${asset}" COPYONLY)
    endforeach()
    configure_file(web/game.js web/game.js COPYONLY)
