#------------------------------------------------------------------------------
# Usage: find_package(SDL2 [REQUIRED] [COMPONENTS main])
#
# Sets variables:
#     SDL2_INCLUDE_DIRS
#     SDL2_LIBRARIES
#     SDL2_DLLS (Windows only)
#------------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)

set(SDL2_VERSION 2.26.2)

# Check if "main" was specified as a component
set(_SDL2_use_main FALSE)
foreach(_SDL2_component ${SDL2_FIND_COMPONENTS})
    if(_SDL2_component STREQUAL "main")
        set(_SDL2_use_main TRUE)
    else()
        message(WARNING "Unrecognized component \"${_SDL2_component}\"")
    endif()
endforeach()

if(WIN32)
    find_path(SDL2_ROOT "include/SDL.h"
        PATHS "${CMAKE_SOURCE_DIR}/extern/SDL2-${SDL2_VERSION}"
        NO_DEFAULT_PATH
    )
    
    if(SDL2_ROOT)
        set(SDL2_INCLUDE_DIRS "${SDL2_ROOT}/include")
        set(_SDL2_ARCH "x64")
        set(SDL2_LIBRARIES "${SDL2_ROOT}/lib/${_SDL2_ARCH}/SDL2.lib")
        set(SDL2_DLLS "${SDL2_ROOT}/lib/${_SDL2_ARCH}/SDL2.dll")
        if(_SDL2_use_main)
            list(APPEND SDL2_LIBRARIES "${SDL2_ROOT}/lib/${_SDL2_ARCH}/SDL2main.lib")
        endif()

        # When installing, copy DLLs to install location
        install(FILES ${SDL2_DLLS} DESTINATION bin)
    endif()

    mark_as_advanced(SDL2_ROOT)
    find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_INCLUDE_DIRS SDL2_LIBRARIES SDL2_DLLS)

elseif(APPLE)
    find_path(SDL2_INCLUDE_DIRS "SDL.h"
        PATHS "${CMAKE_SOURCE_DIR}/extern/SDL2.framework/Versions/Current"
        PATH_SUFFIXES "Headers"
        REQUIRED
        NO_DEFAULT_PATH
    )

    set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/extern/SDL2.framework")

    find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_INCLUDE_DIRS SDL2_LIBRARIES)

else()
    find_path(SDL2_INCLUDE_DIRS "SDL.h"
        HINTS $ENV{SDL2DIR}
        PATH_SUFFIXES "include/SDL2" "include"
        REQUIRED
    )

    find_library(SDL2_LIBRARIES
        NAMES "SDL2"
        HINTS $ENV{SDL2DIR}
        PATH_SUFFIXES lib64 lib
        REQUIRED
    )

    find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_INCLUDE_DIRS SDL2_LIBRARIES)

endif()
