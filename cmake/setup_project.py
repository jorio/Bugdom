import urllib.request
import os.path
import platform
import subprocess
import shutil
import tempfile
import sys
from zipfile import ZipFile
from pathlib import Path
from dataclasses import dataclass

#----------------------------------------------------------------

libs_dir = os.path.abspath('extern')
cache_dir = os.path.abspath('cache')

use_cocoa_mouse_fix = True

sdl_ver = "2.0.14"

#----------------------------------------------------------------

@dataclass
class CMakeConfig:
    gen_args: list
    build_configs: list

#----------------------------------------------------------------

def get_package(url):
    name = url[url.rfind('/')+1:]

    path = os.path.normpath(F"{cache_dir}/{name}")
    if os.path.exists(path):
        print("Not redownloading:", path)
    else:
        print(F"Downloading: {url}")
        os.makedirs(cache_dir, exist_ok=True)
        urllib.request.urlretrieve(url, path)
    return path

def call(cmd, **kwargs):
    print(">", ' '.join(cmd))
    #print(kwargs)
    try:
        result = subprocess.run(cmd, **kwargs)
        return result
    except subprocess.CalledProcessError as e:
        print("\x1b[1;31mSubprocess failed!\x1b[0m")
        raise e

def nuke_if_exists(path):
    if os.path.exists(path):
        print("==== Nuking", path)
        shutil.rmtree(path)

#----------------------------------------------------------------

def build_sdl_with_cocoa_mouse_patch():
    print("==== Building SDL with Cocoa mouse patch")
    sdl_src_path = get_package(F"http://libsdl.org/release/SDL2-{sdl_ver}.zip")

    with tempfile.TemporaryDirectory() as scratch_dir:
        # Extract SDL source
        sdl_srcdir = scratch_dir + F'/SDL2-{sdl_ver}'
        with ZipFile(sdl_src_path, 'r') as zip:
            zip.extractall(path=scratch_dir)

        # Patch it
        with open(F'extern/CocoaMouseFix-SDL2-{sdl_ver}.patch', 'r') as patchfile:
            call(['patch', '--strip=1'], stdin=patchfile, cwd=sdl_srcdir)

        # Build it
        call(['xcodebuild',
            '-quiet',
            '-project', 'Xcode/SDL/SDL.xcodeproj',
            '-configuration', 'Release',
            '-scheme', 'Framework',
            '-derivedDataPath', scratch_dir
        ], cwd=sdl_srcdir)

        # Copy SDL2.framework to extern
        shutil.copytree(F'{scratch_dir}/Build/Products/Release/{sdl2_framework}', sdl2_framework_target_path, symlinks=True)

#----------------------------------------------------------------

# Make sure we're running from the correct directory...
if not os.path.exists('src/Enemies/Enemy_WorkerBee.c'):  # some file that's likely to be from the Bugdom source tree
    print("\x1b[1;31mSTOP - Please run this script from the root of the Bugdom source repo\x1b[0m")
    sys.exit(1)

system = platform.system()

cmake_generated_dirs = {}
cmake_extra_build_args = []

if system == 'Windows':
    print("==== Fetching SDL")
    sdl_zip_path = get_package(F"http://libsdl.org/release/SDL2-devel-{sdl_ver}-VC.zip")
    with ZipFile(sdl_zip_path, 'r') as zip:
        zip.extractall(path=libs_dir)

    cmake_generated_dirs['build-msvc'] = CMakeConfig(['-G', 'Visual Studio 16 2019', '-A', 'x64'], ['Release', 'Debug'])

elif system == 'Darwin':
    sdl2_framework = "SDL2.framework"
    sdl2_framework_target_path = F'{libs_dir}/{sdl2_framework}'

    nuke_if_exists(sdl2_framework_target_path)

    if use_cocoa_mouse_fix:
        build_sdl_with_cocoa_mouse_patch()
    else:
        printf("==== Fetching SDL")
        sdl_dmg_path = get_package(F"http://libsdl.org/release/SDL2-{sdl_ver}.dmg")
        with tempfile.TemporaryDirectory() as mount_point:
            call(['hdiutil', 'attach', sdl_dmg_path, '-mountpoint', mount_point, '-quiet'])
            shutil.copytree(F'{mount_point}/{sdl2_framework}', sdl2_framework_target_path, symlinks=True)
            call(['hdiutil', 'detach', mount_point, '-quiet'])

    cmake_generated_dirs['build-xcode'] = CMakeConfig(['-G', 'Xcode'], ['Release', 'Debug'])
    cmake_extra_build_args = ['--', '-quiet']

else:
    cmake_generated_dirs['build-release'] = CMakeConfig(['-DCMAKE_BUILD_TYPE=Release'], [])
    cmake_generated_dirs['build-debug'] = CMakeConfig(['-DCMAKE_BUILD_TYPE=Debug'], [])

#----------------------------------------------------------------
# Prepare config dirs

for config_dir in cmake_generated_dirs:
    nuke_if_exists(config_dir)

    print(F"==== Preparing {config_dir}")
    args = cmake_generated_dirs[config_dir].gen_args
    call(['cmake','-S','.','-B',config_dir] + args)

#----------------------------------------------------------------
# Build the game

for config_dir in cmake_generated_dirs:
    build_configs = cmake_generated_dirs[config_dir].build_configs
    if build_configs:
        for build_config in build_configs:
            print(F"==== Building the game:", config_dir, build_config)
            call(['cmake', '--build', config_dir, '--config', build_config] + cmake_extra_build_args)
    else:
        print(F"==== Building the game:", config_dir)
        call(['cmake', '--build', config_dir] + cmake_extra_build_args)
