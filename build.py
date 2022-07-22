#!/usr/bin/env python3

import argparse
import contextlib
import hashlib
import glob
import multiprocessing
import os
import os.path
import platform
import shutil
import stat
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

#----------------------------------------------------------------

libs_dir = os.path.abspath("extern")
cache_dir = os.path.abspath("cache")
dist_dir = os.path.abspath("dist")

game_name           = "Bugdom"  # no spaces
game_name_human     = "Bugdom"  # spaces and other special characters allowed
game_ver            = "1.3.2"

source_check        = "src/Enemies/Enemy_WorkerBee.c"  # some file that's likely to be from the game's source tree

sdl_ver             = "2.0.22"
appimagetool_ver    = "13"

lib_hashes = {  # sha-256
    "SDL2-2.0.22.tar.gz":            "fe7cbf3127882e3fc7259a75a0cb585620272c51745d3852ab9dd87960697f2e",
    "SDL2-2.0.22.dmg":               "72974672b8359057aa2f6d467c8adae8182a6caedd660e3936e23c3c683c3801",
    "SDL2-devel-2.0.22-VC.zip":      "32adc96d8b25e5671189f1f38a4fc7deb105fbb1b3ed78ffcb23f5b8f36b3922",
    "appimagetool-x86_64.AppImage":  "df3baf5ca5facbecfc2f3fa6713c29ab9cefa8fd8c1eac5d283b79cab33e4acb", # appimagetool v13
    "appimagetool-aarch64.AppImage": "334e77beb67fc1e71856c29d5f3f324ca77b0fde7a840fdd14bd3b88c25c341f",
}

NPROC = multiprocessing.cpu_count()
SYSTEM = platform.system()
MACHINE = platform.machine()

if SYSTEM == "Windows":
    os.system("")  # hack to get ANSI color escapes to work

#----------------------------------------------------------------

parser = argparse.ArgumentParser(description=F"Configure, build, and package {game_name_human}")

if SYSTEM == "Darwin":
    default_generator = "Xcode"
    default_architecture = None
    help_configure = "generate Xcode project"
    help_build = "build app from Xcode project"
    help_package = "package up the game into a DMG"
elif SYSTEM == "Windows":
    default_generator = "Visual Studio 17 2022"
    default_architecture = "x64"
    help_configure = F"generate {default_generator} solution"
    help_build = F"build exe from {default_generator} solution"
    help_package = "package up the game into a ZIP"
else:
    default_generator = None
    default_architecture = None
    help_configure = "generate project"
    help_build = "build binary"
    help_package = "package up the game into an AppImage"

parser.add_argument("--dependencies", default=False, action="store_true", help="fetch and set up dependencies (SDL)")
parser.add_argument("--configure", default=False, action="store_true", help=help_configure)
parser.add_argument("--build", default=False, action="store_true", help=help_build)
parser.add_argument("--package", default=False, action="store_true", help=help_package)

parser.add_argument("-G", metavar="<generator>", default=default_generator,
        help=F"custom project generator for the CMake configure step (default: {default_generator})")

parser.add_argument("-A", metavar="<arch>", default=default_architecture,
        help=F"custom platform name for the CMake configure step (default: {default_architecture})")

parser.add_argument("--print-artifact-name", default=False, action="store_true",
        help="print artifact name and exit")

if SYSTEM == "Linux":
    parser.add_argument("--system-sdl", default=False, action="store_true",
        help="use system SDL instead of building SDL from scratch")

args = parser.parse_args()

#----------------------------------------------------------------

class Project:
    def __init__(self, dir_name, gen_args=[], gen_env={}, build_configs=[], build_args=[]):
        self.dir_name = dir_name
        self.gen_args = gen_args
        self.gen_env = gen_env
        self.build_configs = build_configs
        self.build_args = build_args

#----------------------------------------------------------------

@contextlib.contextmanager
def chdir(path):
    origin = os.getcwd()
    try:
        os.chdir(path)
        yield
    finally:
        os.chdir(origin)

def die(message):
    print(F"\x1b[1;31m{message}\x1b[0m", file=sys.stderr)
    sys.exit(1)

def log(message):
    print(message, file=sys.stderr)

def fatlog(message):
    starbar = len(message) * '*'
    print(F"\n{starbar}\n{message}\n{starbar}", file=sys.stderr)

def hash_file(path):
    hasher = hashlib.sha256()
    with open(path, 'rb') as f:
        while True:
            chunk = f.read(64*1024)
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest()

def get_package(url):
    name = url[url.rfind('/')+1:]

    if name in lib_hashes:
        reference_hash = lib_hashes[name]
    else:
        die(F"Build script lacks reference checksum for {name}")

    path = os.path.normpath(F"{cache_dir}/{name}")
    if os.path.exists(path):
        log(F"Not redownloading: {path}")
    else:
        log(F"Downloading: {url}")
        os.makedirs(cache_dir, exist_ok=True)
        urllib.request.urlretrieve(url, path)

    actual_hash = hash_file(path)
    if reference_hash != actual_hash:
        die(F"Bad checksum for {name}: expected {reference_hash}, got {actual_hash}")

    return path

def call(cmd, **kwargs):
    cmdstr = ""
    for token in cmd:
        cmdstr += " "
        if " " in token:
            cmdstr += F"\"{token}\""
        else:
            cmdstr += token

    log(F">{cmdstr}")
    try:
        return subprocess.run(cmd, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        die(F"Aborting setup because: {e}")

def rmtree_if_exists(path):
    if os.path.exists(path):
        shutil.rmtree(path)

def zipdir(zipname, topleveldir, arc_topleveldir):
    with zipfile.ZipFile(zipname, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as zipf:
        for root, dirs, files in os.walk(topleveldir):
            for file in files:
                filepath = os.path.join(root, file)
                arcpath = os.path.join(arc_topleveldir, filepath[len(topleveldir)+1:])
                log(F"Zipping: {arcpath}")
                zipf.write(filepath, arcpath)

#----------------------------------------------------------------

def prepare_dependencies_windows():
    rmtree_if_exists(F"{libs_dir}/SDL2-{sdl_ver}")

    sdl_zip_path = get_package(F"https://libsdl.org/release/SDL2-devel-{sdl_ver}-VC.zip")
    shutil.unpack_archive(sdl_zip_path, libs_dir)

def prepare_dependencies_macos():
    sdl2_framework = "SDL2.framework"
    sdl2_framework_target_path = F"{libs_dir}/{sdl2_framework}"

    rmtree_if_exists(sdl2_framework_target_path)

    sdl_dmg_path = get_package(F"https://libsdl.org/release/SDL2-{sdl_ver}.dmg")

    # Mount the DMG and copy SDL2.framework to extern/
    with tempfile.TemporaryDirectory() as mount_point:
        call(["hdiutil", "attach", sdl_dmg_path, "-mountpoint", mount_point, "-quiet"])
        shutil.copytree(F"{mount_point}/{sdl2_framework}", sdl2_framework_target_path, symlinks=True)
        call(["hdiutil", "detach", mount_point, "-quiet"])

    if "CODE_SIGN_IDENTITY" in os.environ:
        call(["codesign", "--force", "--timestamp", "--sign", os.environ["CODE_SIGN_IDENTITY"], sdl2_framework_target_path])
    else:
        print("SDL will not be codesigned. Set the CODE_SIGN_IDENTITY environment variable if you want to sign it.")

def prepare_dependencies_linux():
    if not args.system_sdl:
        sdl_source_dir = F"{libs_dir}/SDL2-{sdl_ver}"
        sdl_build_dir = F"{sdl_source_dir}/build"
        rmtree_if_exists(sdl_source_dir)

        sdl_zip_path = get_package(F"https://libsdl.org/release/SDL2-{sdl_ver}.tar.gz")
        shutil.unpack_archive(sdl_zip_path, libs_dir)

        with chdir(sdl_source_dir):
            call([F"{sdl_source_dir}/configure", F"--prefix={sdl_build_dir}", "--quiet"])
            call(["make", "-j", str(NPROC)], stdout=subprocess.DEVNULL)
            call(["make", "install", "--silent"])  # install to configured prefix (sdl_build_dir)

#----------------------------------------------------------------

def get_artifact_name():
    if SYSTEM == "Windows":
        return F"{game_name}-{game_ver}-windows-x64.zip"
    elif SYSTEM == "Darwin":
        return F"{game_name}-{game_ver}-mac.dmg"
    elif SYSTEM == "Linux":
        return F"{game_name}-{game_ver}-linux-{MACHINE}.AppImage"
    else:
        die("Unknown system for print_artifact_name")

def copy_documentation(proj, appdir, full=True):
    shutil.copy(F"{proj.dir_name}/ReadMe.txt", F"{appdir}")
    shutil.copy(F"LICENSE.md", F"{appdir}/License.txt")

    if full:
        shutil.copytree("docs", F"{appdir}/Documentation")
        os.remove(F"{appdir}/Documentation/screenshot.png")
        for docfile in ["CHANGELOG.md", "CHEATS.md", "COMMANDLINE.md"]:
            shutil.copy(docfile, F"{appdir}/Documentation")

def package_windows(proj: Project):
    release_config = proj.build_configs[0]
    windows_dlls = ["SDL2.dll", "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"]

    # Prep DLLs with cmake (copied to {cache_dir}/install/bin)
    call(["cmake", "--install", proj.dir_name, "--prefix", F"{cache_dir}/install"])

    appdir = F"{cache_dir}/{game_name}-{game_ver}"
    rmtree_if_exists(appdir)
    os.makedirs(F"{appdir}", exist_ok=True)

    # Copy executable, PDB, assets and libs
    shutil.copy(F"{proj.dir_name}/{release_config}/{game_name}.exe", appdir)
    shutil.copy(F"{proj.dir_name}/{release_config}/{game_name}.pdb", appdir)
    shutil.copytree("Data", F"{appdir}/Data")
    for dll in windows_dlls:
        shutil.copy(F"{cache_dir}/install/bin/{dll}", appdir)

    copy_documentation(proj, appdir)

    zipdir(F"{dist_dir}/{get_artifact_name()}", appdir, F"{game_name}-{game_ver}")

def package_macos(proj: Project):
    release_config = proj.build_configs[0]
    appdir = F"{proj.dir_name}/{release_config}"

    # Human-friendly name for .app
    os.rename(F"{appdir}/{game_name}.app", F"{appdir}/{game_name_human}.app")

    copy_documentation(proj, appdir)

    #shutil.copy("packaging/dmg_DS_Store", F"{appdir}/.DS_Store")

    call(["hdiutil", "create",
        "-fs", "HFS+",
        "-srcfolder", appdir,
        "-volname", F"{game_name_human} {game_ver}",
        F"{dist_dir}/{get_artifact_name()}"])

def package_linux(proj):
    appimagetool_path = get_package(F"https://github.com/AppImage/AppImageKit/releases/download/{appimagetool_ver}/appimagetool-{MACHINE}.AppImage")
    os.chmod(appimagetool_path, 0o755)

    appdir = F"{cache_dir}/{game_name}-{game_ver}.AppDir"
    rmtree_if_exists(appdir)

    os.makedirs(F"{appdir}", exist_ok=True)
    os.makedirs(F"{appdir}/usr/bin", exist_ok=True)
    os.makedirs(F"{appdir}/usr/lib", exist_ok=True)

    # Copy executable and assets
    shutil.copy(F"{proj.dir_name}/{game_name}", F"{appdir}/usr/bin")  # executable
    shutil.copytree("Data", F"{appdir}/Data")
    copy_documentation(proj, appdir, full=False)

    # Copy XDG stuff
    shutil.copy(F"packaging/{game_name.lower()}.desktop", appdir)
    shutil.copy(F"packaging/{game_name.lower()}-desktopicon.png", appdir)

    # Copy AppImage kicker script
    shutil.copy(F"packaging/AppRun", appdir)
    os.chmod(F"{appdir}/AppRun", 0o755)

    # Copy SDL (if not using system SDL)
    if not args.system_sdl:
        for file in glob.glob(F"{libs_dir}/SDL2-{sdl_ver}/build/lib/libSDL2*.so*"):
            shutil.copy(file, F"{appdir}/usr/lib", follow_symlinks=False)

    # Invoke appimagetool
    call([appimagetool_path, "--no-appstream", appdir, F"{dist_dir}/{get_artifact_name()}"])

#----------------------------------------------------------------

if args.print_artifact_name:
    print(get_artifact_name())
    sys.exit(0)

fatlog(F"{game_name} {game_ver} build script")

if not (args.dependencies or args.configure or args.build or args.package):
    log("No build steps specified, running all of them.")
    args.dependencies = True
    args.configure = True
    args.build = True
    args.package = True

# Make sure we're running from the correct directory...
if not os.path.exists(source_check):  # some file that's likely to be from the game's source tree
    die(F"STOP - Please run this script from the root of the {game_name} source repo")

#----------------------------------------------------------------
# Set up project metadata

projects = []

common_gen_args = []
if args.G:
    common_gen_args += ["-G", args.G]
if args.A:
    common_gen_args += ["-A", args.A]

if SYSTEM == "Windows":
    # On Windows, ship a PDB file along with the Release build.
    # Avoid RelWithDebInfo because bottom-of-the-barrel AVs may raise a false positive with such builds.
    projects = [Project(
        dir_name="build-msvc",
        gen_args=common_gen_args,
        build_configs=["Release", "Debug"],
        build_args=["-m"]  # multiprocessor compilation
    )]

elif SYSTEM == "Darwin":
    projects = [Project(
        dir_name="build-xcode",
        gen_args=common_gen_args,
        build_configs=["RelWithDebInfo"],
        build_args=["-j", str(NPROC), "-quiet"]
    )]

elif SYSTEM == "Linux":
    release_config = "RelWithDebInfo"

    gen_env = {}
    if not args.system_sdl:
        gen_env["SDL2DIR"] = F"{libs_dir}/SDL2-{sdl_ver}/build"

    projects.append(Project(
        dir_name=F"build-{release_config.lower()}",
        gen_args=common_gen_args + [F"-DCMAKE_BUILD_TYPE={release_config}"],
        gen_env=gen_env,
        build_args=["-j", str(NPROC)]
    ))

    projects.append(Project(
        dir_name="build-debug",
        gen_args=common_gen_args + ["-DCMAKE_BUILD_TYPE=Debug"],
        gen_env=gen_env,
        build_args=["-j", str(NPROC)]
    ))
else:
    die(F"Unsupported system for configure step: {SYSTEM}")


#----------------------------------------------------------------
# Prepare dependencies

if args.dependencies:
    fatlog("Setting up dependencies")

    # Check that our submodules are here
    if not os.path.exists("extern/Pomme/CMakeLists.txt"):
        die("Submodules appear to be missing.\n"
            + "Did you clone the submodules recursively? Try this:    git submodule update --init --recursive")

    if SYSTEM == "Windows":
        prepare_dependencies_windows()
    elif SYSTEM == "Darwin":
        prepare_dependencies_macos()
    elif SYSTEM == "Linux":
        prepare_dependencies_linux()
    else:
        die(F"Unsupported system for dependencies step: {SYSTEM}")

#----------------------------------------------------------------
# Configure projects

if args.configure:
    for proj in projects:
        fatlog(F"Configuring {proj.dir_name}")

        rmtree_if_exists(proj.dir_name)

        env = None
        if proj.gen_env:
            env = os.environ.copy()
            env.update(proj.gen_env)

        call(["cmake", "-S", ".", "-B", proj.dir_name] + proj.gen_args, env=env)

#----------------------------------------------------------------
# Build the game

proj = projects[0]

if args.build:
    fatlog(F"Building the game: {proj.dir_name}")

    build_command = ["cmake", "--build", proj.dir_name]

    if proj.build_configs:
        build_command += ["--config", proj.build_configs[0]]

    if proj.build_args:
        build_command += ["--"] + proj.build_args

    call(build_command)

#----------------------------------------------------------------
# Package the game

if args.package:
    fatlog(F"Packaging the game")

    rmtree_if_exists(dist_dir)
    os.makedirs(dist_dir, exist_ok=True)

    if SYSTEM == "Darwin":
        package_macos(proj)
    elif SYSTEM == "Windows":
        package_windows(proj)
    elif SYSTEM == "Linux":
        package_linux(proj)
    else:
        die(F"Unsupported system for package step: {SYSTEM}")
