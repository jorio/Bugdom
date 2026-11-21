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
    foreach(asset controls.js touch.js style.css pwa.js legacy_gl.js)
        configure_file("web/common/${asset}" "web/common/${asset}" COPYONLY)
    endforeach()
    configure_file(web/game.js web/game.js COPYONLY)

    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    # Run on incremental builds too: changing only CSS/config must refresh the
    # service worker version without forcing a costly Wasm relink.
    add_custom_target(web_pwa ALL
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/web/common/pwa_build.py"
            --build "${CMAKE_CURRENT_BINARY_DIR}" --web "${CMAKE_SOURCE_DIR}/web"
        DEPENDS ${GAME_TARGET}
        BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/sw.js" "${CMAKE_CURRENT_BINARY_DIR}/manifest.webmanifest"
        VERBATIM)
