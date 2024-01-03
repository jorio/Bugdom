cmake_minimum_required(VERSION 3.16)

set(GAME_TARGET				"Bugdom")
set(GAME_VERSION			"1.3.4")

set(GAME_MAC_ICNS			"packaging/${GAME_TARGET}.icns")
set(GAME_MAC_COPYRIGHT		"© 1999 Pangea Software, Inc.\n© 2024 Iliyas Jorio.")
set(GAME_MAC_BUNDLE_ID		"io.jor.bugdom")
set(GAME_MAC_BUNDLE_NAME	"Bugdom")

set(CMAKE_MODULE_PATH		"${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD		20)

if (${CMAKE_VERSION} VERSION_LESS "3.21")
	set(CMAKE_C_STANDARD	11)
else()
	set(CMAKE_C_STANDARD	17)
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target macOS architectures")

project(${GAME_TARGET}
	VERSION ${GAME_VERSION}
	LANGUAGES C CXX
)

option(SANITIZE "Build with asan/ubsan" OFF)

if(SANITIZE)
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

	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${GAME_TARGET})
elseif(APPLE)
	# Let the game (and Pomme) find header files within SDL2.framework
	set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS extern)
	add_compile_options("-F${CMAKE_SOURCE_DIR}/extern")  # for non-xcode IDEs
endif()

set(POMME_NO_VIDEO true)
set(POMME_NO_INPUT true)
set(POMME_NO_GRAPHICS true)
set(POMME_NO_MP3 true)

add_compile_definitions(POMME_CASE_SENSITIVE_FSSPEC=1)

#------------------------------------------------------------------------------
# DEPENDENCIES
#------------------------------------------------------------------------------

add_subdirectory(extern/Pomme)

find_package(SDL2 REQUIRED COMPONENTS main)
find_package(OpenGL REQUIRED)

#------------------------------------------------------------------------------
# GAME SOURCES
#------------------------------------------------------------------------------

set(GAME_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Write header file containing version info
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in ${GAME_SRCDIR}/Headers/version.h)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS ${GAME_SRCDIR}/*.c ${GAME_SRCDIR}/*.cpp)

file(GLOB_RECURSE GAME_HEADERS CONFIGURE_DEPENDS ${GAME_SRCDIR}/*.h)

set(GAME_ALL_SOURCES
	${GAME_SOURCES}
	${GAME_HEADERS}
)

# Make source groups match file tree
source_group(TREE ${GAME_SRCDIR} PREFIX "" FILES ${GAME_ALL_SOURCES})

#------------------------------------------------------------------------------
# EXECUTABLE TARGET
#------------------------------------------------------------------------------

set(GAME_LIBRARIES
	Pomme
	${SDL2_LIBRARIES}
	OpenGL::GL
)

if(WIN32)
	# "WIN32" here is equivalent to /SUBSYSTEM:WINDOWS for MSVC
	add_executable(${GAME_TARGET} WIN32
		${GAME_ALL_SOURCES}
		"${CMAKE_CURRENT_SOURCE_DIR}/packaging/${GAME_TARGET}.exe.rc"
	)
elseif(APPLE)
	# Add required frameworks for KillMacMouseAcceleration
	list(APPEND GAME_LIBRARIES "-framework Foundation" "-framework IOKit")

	add_executable(${GAME_TARGET} MACOSX_BUNDLE
		${GAME_ALL_SOURCES}
		${GAME_MAC_ICNS}
		${SDL2_LIBRARIES}
	)
else()
	# Math lib, explicitly required on some Linux systems
	list(APPEND GAME_LIBRARIES m)

	add_executable(${GAME_TARGET} ${GAME_ALL_SOURCES})
endif()

target_include_directories(${GAME_TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	extern/Pomme/src
	extern/Pomme/src/QD3D
	${GAME_SRCDIR}/Headers
)


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
		-Wno-multichar
		-Wno-unknown-pragmas
		-Wstrict-aliasing=2
	)

	# Sanitizers in debug mode (Linux only)
	# When using a debugger, you should export LSAN_OPTIONS=detect_leaks=0
	if(SANITIZE)
		list(INSERT GAME_LIBRARIES 0 asan ubsan)
		target_compile_options(${GAME_TARGET} PRIVATE
			-fsanitize=alignment
			-fsanitize=address
			-fsanitize=leak
			-fsanitize=undefined
			-fno-omit-frame-pointer
		)
	endif()
else()
	# On Win32, we need NOGDI and NOUSER to be able to define some Mac functions
	# whose names are otherwise taken by Windows APIs.
	target_compile_definitions(${GAME_TARGET} PRIVATE
		WIN32_LEAN_AND_MEAN
		NOGDI						# avoid name clashes with Mac functions
		NOUSER						# avoid name clashes with Mac functions
		_CRT_SECURE_NO_WARNINGS		# quit whining about snprintf_s
	)

	target_compile_options(${GAME_TARGET} PRIVATE
		/EHs	# synchronous exceptions; also, extern "C" functions may throw exceptions
		/W4
		/wd4068 # ignore unrecognized pragmas
		/wd4100 # unreferenced formal parameters
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

target_link_libraries(${GAME_TARGET} ${GAME_LIBRARIES})

#------------------------------------------------------------------------------
# PLATFORM-SPECIFIC PACKAGING
#------------------------------------------------------------------------------

set_target_properties(${GAME_TARGET} PROPERTIES
	#--------------------------------------------------------------------------
	# MSVC/WIN32
	#--------------------------------------------------------------------------

	VS_DEBUGGER_WORKING_DIRECTORY		"${CMAKE_SOURCE_DIR}"
	VS_DPI_AWARE						"PerMonitor"

	#--------------------------------------------------------------------------
	# APPLE
	#--------------------------------------------------------------------------

	# Set framework search path to (App bundle)/Contents/Frameworks so the game can use its embedded SDL2.framework
	XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"

	# Explicitly turn off code signing, otherwise downloaded app will be quarantined forever
	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""

	# Set up Info.plist values
	MACOSX_BUNDLE_ICON_FILE				"${GAME_TARGET}.icns"	# CFBundleIconFile
	MACOSX_BUNDLE_EXECUTABLE_NAME		${GAME_TARGET}			# CFBundleExecutable - executable name inside the bundle
	MACOSX_BUNDLE_SHORT_VERSION_STRING	${PROJECT_VERSION}		# CFBundleShortVersionString
	MACOSX_BUNDLE_COPYRIGHT				${GAME_MAC_COPYRIGHT}	# NSHumanReadableCopyright (supersedes CFBundleGetInfoString (MACOSX_BUNDLE_INFO_STRING))
	MACOSX_BUNDLE_BUNDLE_NAME			${GAME_MAC_BUNDLE_NAME}	# CFBundleName - user-visible short name for the bundle, up to 15 characters
	MACOSX_BUNDLE_GUI_IDENTIFIER		${GAME_MAC_BUNDLE_ID}	# CFBundleIdentifier - unique bundle ID in reverse-DNS format

	# Bundle ID required for code signing - must match CFBundleIdentifier otherwise xcode will complain
	XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${GAME_MAC_BUNDLE_ID}

	# Don't bother with universal builds when we're working on the debug version
	XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES"

	XCODE_EMBED_FRAMEWORKS							"${SDL2_LIBRARIES}"
	XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY		"YES"		# frameworks must be signed by the same developer as the binary
	XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY	"YES"		# not strictly necessary, but that's cleaner
	XCODE_ATTRIBUTE_COPY_PHASE_STRIP[variant=Debug]	"NO"		# avoid "skipping copy phase strip" warning while working on Debug config
	XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME			"YES"		# required for notarization to pass
)

if(APPLE)
	# If we have a code signing identity (CODE_SIGN_IDENTITY environment variable),
	# set up the release build for proper code signing
	if(NOT "$ENV{CODE_SIGN_IDENTITY}" STREQUAL "")
		set_target_properties(${GAME_TARGET} PROPERTIES
			XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Release] "$ENV{CODE_SIGN_IDENTITY}"
			XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=RelWithDebInfo] "$ENV{CODE_SIGN_IDENTITY}"
			XCODE_ATTRIBUTE_DEVELOPMENT_TEAM[variant=Release] "$ENV{CODE_SIGN_IDENTITY}"
			XCODE_ATTRIBUTE_DEVELOPMENT_TEAM[variant=RelWithDebInfo] "$ENV{CODE_SIGN_IDENTITY}"

			# The following is to pass notarization requirements
			XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS[variant=Release] "NO"
			XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS[variant=RelWithDebInfo] "NO"
			XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS[variant=Release] "--options=runtime --timestamp"
			XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS[variant=RelWithDebInfo] "--options=runtime --timestamp"
		)
		message("Release build will be code signed!")
	endif()

	# Copy stuff to app bundle contents
	set_source_files_properties(${GAME_MAC_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

	set(BUNDLE_CONTENTS_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/..")
	set(APP_PARENT_DIR "${BUNDLE_CONTENTS_DIR}/../..")

	add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
		# Copy assets to app bundle
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data ${BUNDLE_CONTENTS_DIR}/Resources
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/packaging/mainmenu.nib ${BUNDLE_CONTENTS_DIR}/Resources/mainmenu.nib

		# High-DPI support in Info.plist
		COMMAND plutil -replace NSHighResolutionCapable -bool true ${BUNDLE_CONTENTS_DIR}/Info.plist

		# Sonoma Game Mode support
		COMMAND plutil -replace LSApplicationCategoryType -string "public.app-category.games" ${BUNDLE_CONTENTS_DIR}/Info.plist

		# Make it honor mainmenu.nib (use -replace instead of -insert so it doesn't fail on subsequent builds)
		COMMAND plutil -replace NSMainNibFile -string "mainmenu" ${BUNDLE_CONTENTS_DIR}/Info.plist
	)
else()
	set(APP_PARENT_DIR "$<TARGET_FILE_DIR:${GAME_TARGET}>")

	# Copy assets besides executable
	add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data ${APP_PARENT_DIR}/Data)
endif()

# Windows-specific libraries
if(WIN32)
	# Copy SDL2 DLLs to output folder on Windows for convenience
	foreach(DLL ${SDL2_DLLS})
		add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${APP_PARENT_DIR})
	endforeach()

	# When installing (cmake --install), copy Visual Studio redistributable DLLs to install location
	include(InstallRequiredSystemLibraries)
endif()

# Copy documentation to output folder
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packaging/ReadMe.txt.in ${CMAKE_CURRENT_BINARY_DIR}/ReadMe.txt)
