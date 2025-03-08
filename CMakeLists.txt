cmake_minimum_required(VERSION 3.21)

set(GAME_TARGET				"Bugdom")
set(GAME_FULL_NAME			"Bugdom")
set(GAME_VERSION			"1.3.5")
set(GAME_IDENTIFIER			"io.jor.bugdom")
set(GAME_COPYRIGHT			"© 1999 Pangea Software, Inc., © 2025 Iliyas Jorio.")

set(GAME_DATADIR			"${CMAKE_SOURCE_DIR}/Data")
set(GAME_SRCDIR				"${CMAKE_SOURCE_DIR}/src")

set(CMAKE_CXX_STANDARD		20)
set(CMAKE_C_STANDARD		17)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")  # Sync with LSMinimumSystemVersion in SDL3.framework's Info.plist
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target macOS architectures")

project(${GAME_TARGET}
	VERSION ${GAME_VERSION}
	LANGUAGES C CXX)

option(BUILD_SDL_FROM_SOURCE "Build SDL from source" OFF)
option(SDL_STATIC "Static link SDL" OFF)

option(SANITIZE "Build with asan/ubsan" OFF)

if(WIN32 OR APPLE)
	# Don't warn
elseif(SANITIZE)
	message("Sanitizers enabled")
else()
	message("Sanitizers disabled (pass -DSANITIZE=1 to enable)")
endif()

#------------------------------------------------------------------------------
# GLOBAL OPTIONS (BEFORE ADDING SUBDIRECTORIES)
#------------------------------------------------------------------------------

if(MSVC)
	# This lets us detect that MSVC supports C++20 features via the __cplusplus macro
	# (See: https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
	add_compile_options(/Zc:__cplusplus)
	add_compile_definitions(UNICODE _UNICODE)

	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${GAME_TARGET})
elseif(APPLE)
	# Let the game (and Pomme) find header files within extern/SDL3.framework
	set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS extern)
	add_compile_options("-F${CMAKE_SOURCE_DIR}/extern")  # for non-xcode IDEs
endif()

set(POMME_NO_GRAPHICS	true)
set(POMME_NO_INPUT		true)
set(POMME_NO_MP3		true)
set(POMME_NO_QD3D		false)		# Bugdom needs this
set(POMME_NO_VIDEO		true)
add_compile_definitions(POMME_CASE_SENSITIVE_FSSPEC=1)

#------------------------------------------------------------------------------
# FIND SDL
#------------------------------------------------------------------------------

# If SDL3_DIR wasn't specified, discover if the user put some prebuilt SDL3
# package in the 'extern' directory
if(NOT DEFINED SDL3_DIR)
	if(APPLE)
		set(_sdl3_maybe "${CMAKE_SOURCE_DIR}/extern/SDL3.framework/Resources/CMake")
	elseif(WIN32)
		set(_sdl3_maybe "${CMAKE_SOURCE_DIR}/extern/SDL3/cmake")
	endif()

	if(DEFINED _sdl3_maybe AND EXISTS "${_sdl3_maybe}")
		set(SDL3_DIR "${_sdl3_maybe}")
		message("Looking for SDL3 in: ${_sdl3_maybe}")
	else()
		message("Couldn't find pre-built SDL3 package in: ${_sdl3_maybe}")
	endif()
	unset(_sdl3_maybe)
endif()

if(NOT BUILD_SDL_FROM_SOURCE)
	find_package(SDL3 CONFIG REQUIRED)
	message("Found pre-built SDL3")
else()
	if(NOT DEFINED SDL3_DIR)
		set(SDL3_DIR "${CMAKE_SOURCE_DIR}/extern/SDL")
	endif()
	message("Building SDL3 from source: " ${SDL3_DIR})
	add_subdirectory("${SDL3_DIR}" EXCLUDE_FROM_ALL)
endif()

#------------------------------------------------------------------------------
# DEPENDENCIES
#------------------------------------------------------------------------------

add_subdirectory(extern/Pomme)

find_package(OpenGL REQUIRED)

#------------------------------------------------------------------------------
# EXECUTABLE TARGET
#------------------------------------------------------------------------------

# Write header file containing version info
configure_file(${GAME_SRCDIR}/Headers/version.h.in ${GAME_SRCDIR}/Headers/version.h)

# Glob all source files
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS ${GAME_SRCDIR}/*.c ${GAME_SRCDIR}/*.cpp ${GAME_SRCDIR}/*.h)

# Make source groups match file tree
source_group(TREE ${GAME_SRCDIR} PREFIX "" FILES ${GAME_SOURCES})

if(WIN32)
	# Windows resource file
	configure_file(${CMAKE_SOURCE_DIR}/packaging/win32.rc.in ${CMAKE_SOURCE_DIR}/packaging/${GAME_TARGET}.exe.rc)
	list(APPEND GAME_SOURCES ${CMAKE_SOURCE_DIR}/packaging/${GAME_TARGET}.exe.rc)
elseif(APPLE)
	list(APPEND GAME_SOURCES ${CMAKE_SOURCE_DIR}/packaging/${GAME_TARGET}.icns)
	set_source_files_properties(${CMAKE_SOURCE_DIR}/packaging/${GAME_TARGET}.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_executable(${GAME_TARGET} ${GAME_SOURCES})

target_include_directories(${GAME_TARGET} PRIVATE ${GAME_SRCDIR}/Headers)

#------------------------------------------------------------------------------
# COMPILER/LINKER OPTIONS
#------------------------------------------------------------------------------

add_compile_definitions(
	"$<$<CONFIG:DEBUG>:_DEBUG>"
)

target_compile_definitions(${GAME_TARGET} PRIVATE
	GL_SILENCE_DEPRECATION)

if(NOT MSVC)
	target_compile_options(${GAME_TARGET} PRIVATE
		-fexceptions
		-Wall
		-Wextra
		-Wshadow
		-Werror=return-type
		$<$<COMPILE_LANGUAGE:C>:-Werror=incompatible-pointer-types>
		-Wno-multichar
		-Wno-unknown-pragmas
		-Wstrict-aliasing=2
	)

	# Sanitizers in debug mode (Linux only)
	# When using a debugger, you should export LSAN_OPTIONS=detect_leaks=0
	if(SANITIZE)
		target_compile_options(${GAME_TARGET} PRIVATE
			-fsanitize=alignment
			-fsanitize=address
			-fsanitize=leak
			-fsanitize=undefined
			-fno-omit-frame-pointer
		)
	endif()
else()
	target_compile_definitions(${GAME_TARGET} PRIVATE
		WIN32_LEAN_AND_MEAN
		NOGDI				# don't clash with Mac function names
		NOUSER				# don't clash with Mac function names
	)

	target_compile_options(${GAME_TARGET} PRIVATE
		/EHs	# synchronous exceptions; also, extern "C" functions may throw exceptions
		/W4
		/wd4068 # ignore unrecognized pragmas
		/wd4100 # unreferenced formal parameters
		/wd4200 # nonstandard extension (zero-sized array in struct/union)
		/wd4201 # nonstandard extension (nameless struct)
		/wd4244 # conversion from double to float
		/wd4305 # truncation from double to float
		/wd5105 # see https://developercommunity.visualstudio.com/t/1249671
		/MP		# multiprocessor build
		/Zi		# output info to PDB
	)

	# Let executable be debugged with PDB, even in Release builds
	target_link_options(${GAME_TARGET} PRIVATE /DEBUG)

	# Enable console for debug builds
	set_target_properties(${GAME_TARGET} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
	set_target_properties(${GAME_TARGET} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
endif()

#------------------------------------------------------------------------------
# LINK LIBRARIES
#------------------------------------------------------------------------------

# Link sanitizers first
if(SANITIZE)
	target_link_libraries(${GAME_TARGET} PRIVATE asan ubsan)
endif()

# Explicitly link math lib on Linux
if(NOT APPLE AND NOT WIN32)
	target_link_libraries(${GAME_TARGET} PRIVATE m)
endif()

# Link SDL
if(NOT SDL_STATIC)
	target_link_libraries(${GAME_TARGET} PRIVATE SDL3::SDL3)
else()
	target_link_libraries(${GAME_TARGET} PRIVATE SDL3::SDL3-static)
endif()

# Link core dependencies
target_link_libraries(${GAME_TARGET} PRIVATE Pomme OpenGL::GL)

# Add required frameworks for KillMacMouseAcceleration
if(APPLE)
	target_link_libraries(${GAME_TARGET} PRIVATE "-framework Foundation" "-framework IOKit")
endif()

#------------------------------------------------------------------------------
# PLATFORM-SPECIFIC PROPERTIES
#------------------------------------------------------------------------------

set_target_properties(${GAME_TARGET} PROPERTIES
	#--------------------------------------------------------------------------
	# MSVC/WIN32
	#--------------------------------------------------------------------------

	WIN32_EXECUTABLE					TRUE					# GUI application instead of console application
	VS_DEBUGGER_WORKING_DIRECTORY		"${CMAKE_SOURCE_DIR}"
	VS_DPI_AWARE						"PerMonitor"

	#--------------------------------------------------------------------------
	# APPLE
	#--------------------------------------------------------------------------

	# Build it as an .app bundle
	MACOSX_BUNDLE						TRUE

	# Set framework search path to (App bundle)/Contents/Frameworks so the game can use its embedded SDL3.framework
	XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"

	# Explicitly turn off code signing, otherwise downloaded app will be quarantined forever
	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""

	# Set up Info.plist values
	MACOSX_BUNDLE_ICON_FILE				"${GAME_TARGET}.icns"	# CFBundleIconFile
	MACOSX_BUNDLE_EXECUTABLE_NAME		${GAME_TARGET}			# CFBundleExecutable - executable name inside the bundle
	MACOSX_BUNDLE_SHORT_VERSION_STRING	${GAME_VERSION}			# CFBundleShortVersionString
	MACOSX_BUNDLE_COPYRIGHT				${GAME_COPYRIGHT}		# NSHumanReadableCopyright (supersedes CFBundleGetInfoString (MACOSX_BUNDLE_INFO_STRING))
	MACOSX_BUNDLE_BUNDLE_NAME			${GAME_FULL_NAME}		# CFBundleName - user-visible short name for the bundle, up to 15 characters
	MACOSX_BUNDLE_GUI_IDENTIFIER		${GAME_IDENTIFIER}		# CFBundleIdentifier - unique bundle ID in reverse-DNS format

	# If CODE_SIGN_IDENTITY is NOT empty: tell Xcode to codesign the app properly
	# Otherwise, if it's empty: explicitly turn off code signing, otherwise downloaded app will be quarantined forever
	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${CODE_SIGN_IDENTITY}"
	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] ""		# don't bother signing debug build

	# Bundle ID required for code signing - must match CFBundleIdentifier otherwise xcode will complain
	XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${GAME_IDENTIFIER}

	# Don't bother with universal builds when we're working on the debug version
	XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES"

	XCODE_EMBED_FRAMEWORKS							"extern/SDL3.framework"
	XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY		"YES"		# frameworks must be signed by the same developer as the binary
	XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY	"YES"		# not strictly necessary, but that's cleaner
	XCODE_ATTRIBUTE_COPY_PHASE_STRIP[variant=Debug]	"NO"		# avoid "skipping copy phase strip" warning while working on Debug config

	# The following is to pass notarization requirements
	XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME			"YES"
	XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS "NO"
	XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS			"--options=runtime --timestamp"
)

#------------------------------------------------------------------------------
# POST-BUILD (COPY ASSETS, ETC.)
#------------------------------------------------------------------------------

if(APPLE)
	set(GAME_DATA_TARGET_LOCATION "$<TARGET_FILE_DIR:${GAME_TARGET}>/../Resources")
else()
	set(GAME_DATA_TARGET_LOCATION "$<TARGET_FILE_DIR:${GAME_TARGET}>/Data")
endif()

add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${GAME_DATADIR}" "${GAME_DATA_TARGET_LOCATION}")

if(APPLE)
	set(INFO_PLIST "${GAME_DATA_TARGET_LOCATION}/../Info.plist")

	add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
		# Copy extra resources to app bundle
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/packaging/mainmenu.nib ${GAME_DATA_TARGET_LOCATION}/mainmenu.nib

		# Info.plist: high-DPI support, Game Mode, honor mainmenu.nib
		COMMAND plutil -replace NSHighResolutionCapable -bool true ${INFO_PLIST}
		COMMAND plutil -replace LSApplicationCategoryType -string "public.app-category.games" ${INFO_PLIST}
		COMMAND plutil -replace NSMainNibFile -string "mainmenu" ${INFO_PLIST}
	)
endif()

# Windows-specific libraries
if(WIN32)
	# Copy SDL3 DLLs to output folder on Windows for convenience
	add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${GAME_TARGET}> $<TARGET_FILE_DIR:${GAME_TARGET}>)

	# When installing (cmake --install), copy Visual Studio redistributable DLLs to install location
	include(InstallRequiredSystemLibraries)
endif()

# Copy documentation to output folder
configure_file(${CMAKE_SOURCE_DIR}/packaging/ReadMe.txt.in ${CMAKE_CURRENT_BINARY_DIR}/ReadMe.txt)
