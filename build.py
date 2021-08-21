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

game_name = "Bugdom"  # no spaces
game_name_human = "Bugdom"  # spaces and other special characters allowed
game_ver = "1.3.1"
sdl_ver = "2.0.14"
appimagetool_ver = "13"

lib_hashes = {
    "SDL2-2.0.14.tar.gz":           "d8215b571a581be1332d2106f8036fcb03d12a70bae01e20f424976d275432bc",
    "SDL2-2.0.14.dmg":              "05ee7538e4617e561333e7a85e5d9ef9813d3e5352e91c10e7f8912f86378793",
    "SDL2-devel-2.0.14-VC.zip":     "232071cf7d40546cde9daeddd0ec30e8a13254c3431be1f60e1cdab35a968824",
    "appimagetool-x86_64.AppImage": "df3baf5ca5facbecfc2f3fa6713c29ab9cefa8fd8c1eac5d283b79cab33e4acb", # appimagetool v13
}

NPROC = multiprocessing.cpu_count()
SYSTEM = platform.system()

#----------------------------------------------------------------

parser = argparse.ArgumentParser(description=F"Configure, build, and package {game_name}")

parser.add_argument("--dependencies", default=False, action="store_true")
parser.add_argument("--configure", default=False, action="store_true")
parser.add_argument("--build", default=False, action="store_true")
parser.add_argument("--package", default=False, action="store_true")
parser.add_argument("--system-sdl", default=False, action="store_true")

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
    print(F"\x1b[1;31m{message}\x1b[0m")
    sys.exit(1)

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
        print("Not redownloading:", path)
    else:
        print(F"Downloading: {url}")
        os.makedirs(cache_dir, exist_ok=True)
        urllib.request.urlretrieve(url, path)

    actual_hash = hash_file(path)
    if reference_hash != actual_hash:
        die(F"Bad checksum for {name}: expected {reference_hash}, got {actual_hash}")

    return path

def call(cmd, **kwargs):
    print(">", " ".join(cmd))
    try:
        return subprocess.run(cmd, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        die(F"Aborting setup because: {e}")

def rmtree_if_exists(path):
    if os.path.exists(path):
        print("rmtree:", path)
        shutil.rmtree(path)

def zipdir(zipname, topleveldir, arc_topleveldir):
    with zipfile.ZipFile(zipname, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as zipf:
        for root, dirs, files in os.walk(topleveldir):
            for file in files:
                filepath = os.path.join(root, file)
                arcpath = os.path.join(arc_topleveldir, filepath[len(topleveldir)+1:])
                print(filepath, "--zip-->", arcpath)
                zipf.write(filepath, arcpath)

#----------------------------------------------------------------

def prepare_dependencies_windows():
    rmtree_if_exists(F"{libs_dir}/SDL2-{sdl_ver}")

    sdl_zip_path = get_package(F"http://libsdl.org/release/SDL2-devel-{sdl_ver}-VC.zip")
    shutil.unpack_archive(sdl_zip_path, libs_dir)

def prepare_dependencies_macos():
    sdl2_framework = "SDL2.framework"
    sdl2_framework_target_path = F"{libs_dir}/{sdl2_framework}"

    rmtree_if_exists(sdl2_framework_target_path)

    print("==== Fetching SDL")
    sdl_dmg_path = get_package(F"http://libsdl.org/release/SDL2-{sdl_ver}.dmg")

    # Mount the DMG and copy SDL2.framework to extern/
    with tempfile.TemporaryDirectory() as mount_point:
        call(["hdiutil", "attach", sdl_dmg_path, "-mountpoint", mount_point, "-quiet"])
        shutil.copytree(F"{mount_point}/{sdl2_framework}", sdl2_framework_target_path, symlinks=True)
        call(["hdiutil", "detach", mount_point, "-quiet"])

def prepare_dependencies_linux():
    if not args.system_sdl:
        sdl_source_dir = F"{libs_dir}/SDL2-{sdl_ver}"
        sdl_build_dir = F"{sdl_source_dir}/build"
        rmtree_if_exists(sdl_source_dir)

        sdl_zip_path = get_package(F"http://libsdl.org/release/SDL2-{sdl_ver}.tar.gz")
        shutil.unpack_archive(sdl_zip_path, libs_dir)

        with chdir(sdl_source_dir):
            call([F"{sdl_source_dir}/configure", F"--prefix={sdl_build_dir}", "--quiet"])
            call(["make", "-j", str(NPROC)], stdout=subprocess.DEVNULL)
            call(["make", "install", "--silent"])  # install to configured prefix (sdl_build_dir)

#----------------------------------------------------------------

def package_windows(proj):
    zipdir(F"{dist_dir}/{game_name}-{game_ver}-windows-x64.zip", F"{proj.dir_name}/Release", F"{game_name}-{game_ver}")

def package_macos(proj):
    call(["hdiutil", "create",
        "-fs", "HFS+",
        "-srcfolder", F"{proj.dir_name}/Release",
        "-volname", F"{game_name_human} {game_ver}",
        F"{dist_dir}/{game_name}-{game_ver}-mac.dmg"])

def package_linux(proj):
    appimagetool_path = get_package(F"https://github.com/AppImage/AppImageKit/releases/download/{appimagetool_ver}/appimagetool-x86_64.AppImage")
    os.chmod(appimagetool_path, 0o755)

    appdir = F"{cache_dir}/{game_name}-{game_ver}.AppDir"
    rmtree_if_exists(appdir)

    os.makedirs(F"{appdir}", exist_ok=True)
    os.makedirs(F"{appdir}/usr/bin", exist_ok=True)
    os.makedirs(F"{appdir}/usr/lib", exist_ok=True)

    # Copy executable and assets
    shutil.copy(F"{proj.dir_name}/{game_name}", F"{appdir}/usr/bin")  # executable
    shutil.copytree("Data", F"{appdir}/Data")

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
    call([appimagetool_path, "--no-appstream", appdir, F"{dist_dir}/{game_name}-{game_ver}-linux-x86_64.AppImage"])

#----------------------------------------------------------------

print(F"""
************************************
{game_name} {game_ver} build script
************************************
""")

# Make sure we're running from the correct directory...
if not os.path.exists("src/Enemies/Enemy_WorkerBee.c"):  # some file that's likely to be from the game's source tree
    die(F"STOP - Please run this script from the root of the {game_name} source repo")

#----------------------------------------------------------------
# Set up project metadata

projects = []

if SYSTEM == "Windows":
    projects = [Project(
        dir_name="build-msvc",
        gen_args=["-G", "Visual Studio 16 2019", "-A", "x64"],
        build_configs=["Release", "Debug"],
    )]

elif SYSTEM == "Darwin":
    projects = [Project(
        dir_name="build-xcode",
        gen_args=["-G", "Xcode"],
        build_configs=["Release"],
        build_args=["-j", str(NPROC), "-quiet"]
    )]

elif SYSTEM == "Linux":
    gen_env = {}
    if not args.system_sdl:
        gen_env["SDL2DIR"] = F"{libs_dir}/SDL2-{sdl_ver}/build"

    projects.append(Project(
        dir_name="build-release",
        gen_args=["-DCMAKE_BUILD_TYPE=Release"],
        gen_env=gen_env,
        build_args=["-j", str(NPROC)]
    ))

    projects.append(Project(
        dir_name="build-debug",
        gen_args=["-DCMAKE_BUILD_TYPE=Debug"],
        gen_env=gen_env,
        build_args=["-j", str(NPROC)]
    ))
else:
    die(F"Unsupported system for configure step: {SYSTEM}")


#----------------------------------------------------------------
# Prepare dependencies

if args.dependencies:
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
        print(F"==== Configuring {proj.dir_name}")

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
    print(F"==== Building the game:", proj.dir_name)

    build_command = ["cmake", "--build", proj.dir_name]

    if proj.build_configs:
        build_command += ["--config", proj.build_configs[0]]

    if proj.build_args:
        build_command += ["--"] + proj.build_args

    call(build_command)

#----------------------------------------------------------------
# Package the game

if args.package:
    print(F"==== Packaging the game")

    os.makedirs(dist_dir, exist_ok=True)

    if SYSTEM == "Darwin":
        package_macos(proj)
    elif SYSTEM == "Windows":
        package_windows(proj)
    elif SYSTEM == "Linux":
        package_linux(proj)
    else:
        die(F"Unsupported system for package step: {SYSTEM}")
