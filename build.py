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
game_ver            = "1.3.3"

source_check        = "src/Enemies/Enemy_WorkerBee.c"  # some file that's likely to be from the game's source tree

sdl_ver             = "2.26.2"
appimagetool_ver    = "13"

lib_hashes = {  # sha-256
    "SDL2-2.26.2.dmg":               "f56a95714c54d89b5d67e08a0362a5b53d4a8d405f7d63ec1bee0f57dc13c69c",
    "SDL2-2.26.2.tar.gz":            "95d39bc3de037fbdfa722623737340648de4f180a601b0afad27645d150b99e0",
    "SDL2-devel-2.26.2-VC.zip":      "8fcc6c2fbfabeddbf7872f994022aa40826ed82324e5a8671dfd4166c62c7e27",
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

parser.add_argument("-1", "--dependencies", default=False, action="store_true", help="step 1: fetch and set up dependencies (SDL)")
parser.add_argument("-2", "--configure", default=False, action="store_true", help=f"step 2: {help_configure}")
parser.add_argument("-3", "--build", default=False, action="store_true", help=f"step 3: {help_build}")
parser.add_argument("-4", "--package", default=False, action="store_true", help=f"step 4: {help_package}")

parser.add_argument("-G", metavar="<generator>", default=default_generator,
        help=F"cmake project generator for step 2 (default: {default_generator})")

parser.add_argument("-A", metavar="<arch>", default=default_architecture,
        help=F"cmake platform name for step 2 (default: {default_architecture})")

parser.add_argument("--dist-dir", metavar="<dir>", default=dist_dir,
        help=F"where to store build artifacts in step 4 (default: {os.path.relpath(dist_dir)})")

parser.add_argument("--print-artifact-name", default=False, action="store_true",
        help="print artifact name and exit")

if SYSTEM == "Linux":
    parser.add_argument("--system-sdl", default=False, action="store_true",
        help="use system SDL instead of building SDL from scratch")
    
    parser.add_argument("--no-appimage", default=False, action="store_true",
        help="don't generate an AppImage in step 4")

args = parser.parse_args()

dist_dir = os.path.abspath(args.dist_dir)

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

def rm_if_exists(path):
    with contextlib.suppress(FileNotFoundError):
        os.remove(path)

def zipdir(zipname, topleveldir, arc_topleveldir):
    with zipfile.ZipFile(zipname, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as zipf:
        for root, dirs, files in os.walk(topleveldir):
            for file in files:
                filepath = os.path.join(root, file)
                arcpath = os.path.join(arc_topleveldir, filepath[len(topleveldir)+1:])
                log(F"Zipping: {arcpath}")
                zipf.write(filepath, arcpath)

#----------------------------------------------------------------

class Project:
    def __init__(self, dir_name):
        self.dir_name = dir_name
        self.gen_args = []
        self.gen_env = {}
        self.build_configs = []
        self.build_args = []

    def prepare_dependencies(self):
        raise NotImplementedError("prepare_dependencies not implemented for this platform")

    def configure(self):
        fatlog(F"Configuring {self.dir_name}")

        rmtree_if_exists(self.dir_name)

        env = None
        if self.gen_env:
            env = os.environ.copy()
            env.update(self.gen_env)

        call(["cmake", "-S", ".", "-B", self.dir_name] + self.gen_args, env=env)

    def build(self):
        build_command = ["cmake", "--build", self.dir_name]

        if self.build_configs:
            build_command += ["--config", self.build_configs[0]]

        if self.build_args:
            build_command += ["--"] + self.build_args

        call(build_command)

    def package(self):
        raise NotImplementedError("package not implemented for this platform")

    def get_artifact_path(self):
        return os.path.join(dist_dir, self.get_artifact_name())

    def get_artifact_name(self):
        raise NotImplementedError("get_artifact_name not implemented for this platform")

    def copy_documentation(self, appdir, full=True):
        shutil.copy(F"{self.dir_name}/ReadMe.txt", F"{appdir}")
        shutil.copy(F"LICENSE.md", F"{appdir}/License.txt")

        if full:
            shutil.copytree("docs", f"{appdir}/Documentation")
            os.remove(f"{appdir}/Documentation/screenshot.webp")
            for docfile in ["CHANGELOG.md", "CHEATS.md", "COMMANDLINE.md"]:
                shutil.copy(docfile, F"{appdir}/Documentation")


class WindowsProject(Project):
    def __init__(self, dir_name="build-msvc"):
        super().__init__(dir_name)
        # On Windows, ship a PDB file along with the Release build.
        # Avoid RelWithDebInfo because bottom-of-the-barrel AVs may raise a false positive with such builds.
        self.build_configs = ["Release", "Debug"]
        self.build_args = ["-m"]  # multiprocessor compilation

    def get_artifact_name(self):
        return F"{game_name}-{game_ver}-windows-x64.zip"

    def prepare_dependencies(self):
        rmtree_if_exists(F"{libs_dir}/SDL2-{sdl_ver}")

        sdl_zip_path = get_package(F"https://libsdl.org/release/SDL2-devel-{sdl_ver}-VC.zip")
        shutil.unpack_archive(sdl_zip_path, libs_dir)

    def package(self):
        release_config = self.build_configs[0]
        windows_dlls = ["SDL2.dll", "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"]

        # Prep DLLs with cmake (copied to {cache_dir}/install/bin)
        call(["cmake", "--install", self.dir_name, "--prefix", F"{cache_dir}/install"])

        appdir = F"{cache_dir}/{game_name}-{game_ver}"
        rmtree_if_exists(appdir)
        os.makedirs(F"{appdir}", exist_ok=True)

        # Copy executable, PDB, assets and libs
        shutil.copy(F"{self.dir_name}/{release_config}/{game_name}.exe", appdir)
        shutil.copy(F"{self.dir_name}/{release_config}/{game_name}.pdb", appdir)
        shutil.copytree("Data", F"{appdir}/Data")
        for dll in windows_dlls:
            shutil.copy(F"{cache_dir}/install/bin/{dll}", appdir)

        self.copy_documentation(appdir)

        rm_if_exists(self.get_artifact_path())
        zipdir(self.get_artifact_path(), appdir, F"{game_name}-{game_ver}")


class MacProject(Project):
    def __init__(self, dir_name="build-xcode"):
        super().__init__(dir_name)
        self.build_configs = ["RelWithDebInfo"]
        self.build_args += ["-j", str(NPROC), "-quiet"]

    def get_artifact_name(self):
        return F"{game_name}-{game_ver}-mac.dmg"

    def prepare_dependencies(self):
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

    def package(self):
        release_config = self.build_configs[0]
        appdir = F"{self.dir_name}/{release_config}"

        # Human-friendly name for .app
        os.rename(F"{appdir}/{game_name}.app", F"{appdir}/{game_name_human}.app")

        self.copy_documentation(appdir)

        rm_if_exists(self.get_artifact_path())
        call(["hdiutil", "create",
            "-fs", "HFS+",
            "-srcfolder", appdir,
            "-volname", F"{game_name_human} {game_ver}",
            self.get_artifact_path()])


class LinuxProject(Project):
    def __init__(self, dir_name, config_name, custom_sdl_path, as_appimage):
        super().__init__(dir_name)

        self.gen_args += ["-DCMAKE_BUILD_TYPE=" + config_name]
        self.build_args += ["-j", str(NPROC)]
        self.build_configs = [config_name]

        if custom_sdl_path:
            self.gen_env["SDL2DIR"] = custom_sdl_path
            self.use_system_sdl = False
        else:
            self.use_system_sdl = True

        self.as_appimage = as_appimage

    def get_artifact_name(self, extension=None):
        if extension is None:
            if self.as_appimage:
                extension = ".AppImage"
            else:
                extension = ".AppDir"
        
        return F"{game_name}-{game_ver}-linux-{MACHINE}{extension}"

    def prepare_dependencies(self):
        if self.use_system_sdl:
            return

        sdl_source_dir = F"{libs_dir}/SDL2-{sdl_ver}"
        sdl_build_dir = F"{sdl_source_dir}/build"
        sdl_prefix_dir = F"{sdl_source_dir}/install"
        rmtree_if_exists(sdl_source_dir)

        sdl_zip_path = get_package(F"https://libsdl.org/release/SDL2-{sdl_ver}.tar.gz")
        shutil.unpack_archive(sdl_zip_path, libs_dir)

        with chdir(sdl_source_dir):
            call(["cmake", "-S", ".", "-B", "build", F"-DCMAKE_INSTALL_PREFIX={sdl_prefix_dir}"])
            call(["cmake", "--build", sdl_build_dir])
            call(["cmake", "--install", sdl_build_dir])

    def package(self):
        if self.as_appimage:
            appdir = cache_dir + "/" + self.get_artifact_name(extension=".AppDir")
        else:
            appdir = self.get_artifact_path()

        assert appdir.endswith(".AppDir")
        rmtree_if_exists(appdir)

        os.makedirs(F"{appdir}", exist_ok=True)
        os.makedirs(F"{appdir}/usr/bin", exist_ok=True)
        os.makedirs(F"{appdir}/usr/lib", exist_ok=True)

        # Copy executable and assets
        shutil.copy(F"{self.dir_name}/{game_name}", F"{appdir}/usr/bin")  # executable
        shutil.copytree("Data", F"{appdir}/Data")
        self.copy_documentation(appdir, full=False)

        # Copy XDG stuff
        shutil.copy(F"packaging/{game_name.lower()}.desktop", appdir)
        shutil.copy(F"packaging/{game_name.lower()}-desktopicon.png", appdir)

        # Copy AppImage kicker script
        shutil.copy(F"packaging/AppRun", appdir)
        os.chmod(F"{appdir}/AppRun", 0o755)

        # Copy SDL (if not using system SDL)
        if not self.use_system_sdl:
            for file in glob.glob(F"{libs_dir}/SDL2-{sdl_ver}/install/lib/libSDL2*.so*"):
                shutil.copy(file, F"{appdir}/usr/lib", follow_symlinks=False)

        # Invoke appimagetool
        if self.as_appimage:
            appimagetool_path = get_package(F"https://github.com/AppImage/AppImageKit/releases/download/{appimagetool_ver}/appimagetool-{MACHINE}.AppImage")
            os.chmod(appimagetool_path, 0o755)

            rm_if_exists(self.get_artifact_path())
            call([appimagetool_path, "--no-appstream", appdir, self.get_artifact_path()])

#----------------------------------------------------------------

if __name__ == "__main__":
    # Make sure we're running from the correct directory...
    if not os.path.exists(source_check):  # some file that's likely to be from the game's source tree
        die(F"STOP - Please run this script from the root of the {game_name} source repo")

    #----------------------------------------------------------------
    # Set up project metadata

    if SYSTEM == "Windows":
        project = WindowsProject()

    elif SYSTEM == "Darwin":
        project = MacProject()

    elif SYSTEM == "Linux":
        custom_sdl_path = ""
        if not args.system_sdl:
            custom_sdl_path = F"{libs_dir}/SDL2-{sdl_ver}/install"
        project = LinuxProject("build", "RelWithDebInfo", custom_sdl_path, not args.no_appimage)
    else:
        die(F"Unsupported system for configure step: {SYSTEM}")

    common_gen_args = []
    if args.G:
        common_gen_args += ["-G", args.G]
    if args.A:
        common_gen_args += ["-A", args.A]

    project.gen_args += common_gen_args

    #----------------------------------------------------------------
    # Gather build steps

    if args.print_artifact_name:
        print(project.get_artifact_name())
        sys.exit(0)

    fatlog(F"{game_name} {game_ver} build script")

    if not (args.dependencies or args.configure or args.build or args.package):
        log("No build steps specified, running all of them.")
        args.dependencies = True
        args.configure = True
        args.build = True
        args.package = True

    #----------------------------------------------------------------
    # Prepare dependencies

    if args.dependencies:
        fatlog("Setting up dependencies")

        # Check that our submodules are here
        if not os.path.exists("extern/Pomme/CMakeLists.txt"):
            die("Submodules appear to be missing.\n"
                + "Did you clone the submodules recursively? Try this:    git submodule update --init --recursive")

        project.prepare_dependencies()

    #----------------------------------------------------------------
    # Configure projects

    if args.configure:
        project.configure()

    #----------------------------------------------------------------
    # Build the game

    if args.build:
        fatlog(F"Building the game: {project.dir_name}")
        project.build()

    #----------------------------------------------------------------
    # Package the game

    if args.package:
        fatlog(F"Packaging the game")
        os.makedirs(dist_dir, exist_ok=True)
        project.package()
