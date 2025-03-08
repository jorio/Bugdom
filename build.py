#!/usr/bin/env python3

import argparse
import contextlib
import hashlib
import glob
import multiprocessing
import os
import os.path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

def parse_metadata(version_dot_h):
    metadata = {}
    with open(version_dot_h) as version_header:
        for version_line in version_header.readlines():
            version_line = version_line.strip()
            match = re.match(r"#define\s+([A-Z_]+)\s+(.+)", version_line)
            if not match:
                continue
            key = match[1]
            value = match[2]
            value = value.removeprefix('"').removesuffix('"')
            metadata[key] = value
    return metadata

#----------------------------------------------------------------

root_dir            = os.path.dirname(os.path.abspath(__file__))
src_dir             = os.path.abspath(root_dir + "/src")
libs_dir            = os.path.abspath(root_dir + "/extern")
dist_dir            = os.path.abspath(root_dir + "/dist")
build_dir           = os.path.abspath(root_dir + "/build")
cache_dir           = os.path.abspath(tempfile.gettempdir() + "/pangea-games-build-cache")

game_metadata       = parse_metadata(src_dir + "/Headers/version.h")
game_name           = game_metadata["GAME_NAME"]  # no spaces
game_name_human     = game_metadata["GAME_FULL_NAME"]  # spaces and other special characters allowed
game_package        = game_metadata["GAME_IDENTIFIER"]  # unique package identifier
game_ver            = game_metadata["GAME_VERSION"]
game_docs           = ["CHANGELOG.md", "SECRETS.md", "docs/*.pdf"]

sdl_ver             = "3.2.8"
appimagetool_ver    = "1.9.0"

lib_hashes = {  # sha-256
    f"SDL3-{sdl_ver}.dmg":              "413f522a1519c8fcf28cb378a07d320164859b3c56ffe0160448502652809d2b",
    f"SDL3-{sdl_ver}.tar.gz":           "13388fabb361de768ecdf2b65e52bb27d1054cae6ccb6942ba926e378e00db03",
    f"SDL3-devel-{sdl_ver}-VC.zip":     "a674c6a6c7ece82227ceeb879c35fd4716e351752ff8b030f8629d832d028fa7",
    "appimagetool-aarch64.AppImage":    "04f45ea45b5aa07bb2b071aed9dbf7a5185d3953b11b47358c1311f11ea94a96",
    "appimagetool-x86_64.AppImage":     "46fdd785094c7f6e545b61afcfb0f3d98d8eab243f644b4b17698c01d06083d1",
}

NPROC = multiprocessing.cpu_count()
SYSTEM = platform.system()
MACHINE = platform.machine()

if SYSTEM == "Windows":
    os.system("")  # hack to get ANSI color escapes to work

#----------------------------------------------------------------

parser = argparse.ArgumentParser(description=f"Configure, build, and package {game_name_human} {game_ver}")

if SYSTEM == "Darwin":
    default_generator = "Xcode"
    default_architecture = None
    help_configure = "generate Xcode project"
    help_build = "build app from Xcode project"
    help_package = "package up the game into a DMG"
elif SYSTEM == "Windows":
    default_generator = "Visual Studio 17 2022"
    default_architecture = "x64"
    help_configure = f"generate {default_generator} solution"
    help_build = f"build exe from {default_generator} solution"
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
        help=f"cmake project generator for step 2 (default: {default_generator})")

parser.add_argument("-A", metavar="<arch>", default=default_architecture,
        help=f"cmake platform name for step 2 (default: {default_architecture})")

parser.add_argument("-B", metavar="<dir>", dest="build_dir", default=build_dir,
        help=f"where to create the build directory (default: {build_dir})")

parser.add_argument("--dist-dir", metavar="<dir>", default=dist_dir,
        help=f"where to store build artifacts in step 4 (default: {os.path.relpath(dist_dir)})")

parser.add_argument("--print-artifact-name", default=False, action="store_true",
        help="print artifact name and exit")

if SYSTEM == "Linux":
    parser.add_argument("--system-sdl", default=False, action="store_true",
        help="use system SDL instead of building SDL from scratch")

    parser.add_argument("--no-appimage", default=False, action="store_true",
        help="don't generate an AppImage in step 4")

args = parser.parse_args()

dist_dir = os.path.abspath(args.dist_dir)
build_dir = os.path.abspath(args.build_dir)

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
    print(f"\x1b[1;31m{message}\x1b[0m", file=sys.stderr)
    sys.exit(1)

def log(message):
    print(message, file=sys.stderr)

def fatlog(message):
    starbar = len(message) * '*'
    print(f"\n{starbar}\n{message}\n{starbar}", file=sys.stderr)

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
        die(f"Build script lacks reference checksum for {name}")

    path = os.path.normpath(f"{cache_dir}/{name}")
    if os.path.exists(path):
        log(f"Not redownloading: {path}")
    else:
        log(f"Downloading: {url}")
        os.makedirs(cache_dir, exist_ok=True)
        urllib.request.urlretrieve(url, path)

    actual_hash = hash_file(path)
    if reference_hash != actual_hash:
        die(f"Bad checksum for {name}: expected {reference_hash}, got {actual_hash}")

    return path

def call(cmd, **kwargs):
    cmdstr = ""
    for token in cmd:
        cmdstr += " "
        if " " in token:
            cmdstr += f"\"{token}\""
        else:
            cmdstr += token

    log(f">{cmdstr}")
    try:
        return subprocess.run(cmd, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        die(f"Aborting setup because: {e}")

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
                log(f"Zipping: {arcpath}")
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
        fatlog(f"Configuring {self.dir_name}")

        if os.path.exists(self.dir_name):
            if not os.path.exists(self.dir_name + "/CMakeCache.txt"):
                die(f"Path exists and isn't an old build directory: {self.dir_name}")
            shutil.rmtree(self.dir_name)

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

    def copy_documentation(self, appdir, everything=True):
        shutil.copy(f"{self.dir_name}/ReadMe.txt", f"{appdir}")
        shutil.copy(f"LICENSE.md", f"{appdir}/License.txt")
        if not everything:
            return
        os.makedirs(f"{appdir}/Documentation")
        for pattern in game_docs:
            for docfile in glob.glob(pattern):
                if os.path.isdir(docfile):
                    shutil.copytree(docfile, f"{appdir}/Documentation/{os.path.basename(docfile)}")
                else:
                    shutil.copy(docfile, f"{appdir}/Documentation")


class WindowsProject(Project):
    def __init__(self, dir_name="build-msvc"):
        super().__init__(dir_name)
        # On Windows, ship a PDB file along with the Release build.
        # Avoid RelWithDebInfo because bottom-of-the-barrel AVs may raise a false positive with such builds.
        self.build_configs = ["Release", "Debug"]
        self.build_args = ["-m"]  # multiprocessor compilation

    def get_artifact_name(self):
        return f"{game_name}-{game_ver}-windows-x64.zip"

    def prepare_dependencies(self):
        rmtree_if_exists(f"{libs_dir}/SDL3")

        sdl_zip_path = get_package(f"https://libsdl.org/release/SDL3-devel-{sdl_ver}-VC.zip")
        shutil.unpack_archive(sdl_zip_path, libs_dir)
        shutil.move(f"{libs_dir}/SDL3-{sdl_ver}", f"{libs_dir}/SDL3")

    def package(self):
        release_config = self.build_configs[0]
        windows_dlls = ["msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"]

        # Prep Visual Studio redistributable DLLs with cmake (copied to {cache_dir}/install/bin)
        call(["cmake", "--install", self.dir_name, "--prefix", f"{cache_dir}/install"])

        appdir = f"{cache_dir}/{game_name}-{game_ver}"
        rmtree_if_exists(appdir)
        os.makedirs(f"{appdir}", exist_ok=True)

        # Copy executable, PDB, assets and libs
        shutil.copy(f"{self.dir_name}/{release_config}/{game_name}.exe", appdir)
        shutil.copy(f"{self.dir_name}/{release_config}/{game_name}.pdb", appdir)
        shutil.copy(f"{self.dir_name}/{release_config}/SDL3.dll", appdir)
        shutil.copytree("Data", f"{appdir}/Data")
        for dll in windows_dlls:
            shutil.copy(f"{cache_dir}/install/bin/{dll}", appdir)

        self.copy_documentation(appdir)

        rm_if_exists(self.get_artifact_path())
        zipdir(self.get_artifact_path(), appdir, f"{game_name}-{game_ver}")


class MacProject(Project):
    def __init__(self, dir_name="build-xcode"):
        super().__init__(dir_name)
        self.build_configs = ["RelWithDebInfo"]
        self.build_args += ["-j", str(NPROC), "-quiet"]

    def get_artifact_name(self):
        return f"{game_name}-{game_ver}-mac.dmg"

    def prepare_dependencies(self):
        sdl3_framework = "SDL3.xcframework/macos-arm64_x86_64/SDL3.framework"
        sdl3_framework_target_path = f"{libs_dir}/SDL3.framework"

        rmtree_if_exists(sdl3_framework_target_path)

        sdl_dmg_path = get_package(f"https://libsdl.org/release/SDL3-{sdl_ver}.dmg")

        # Mount the DMG and copy SDL3.framework to extern/
        with tempfile.TemporaryDirectory() as mount_point:
            call(["hdiutil", "attach", sdl_dmg_path, "-mountpoint", mount_point, "-quiet"])
            shutil.copytree(f"{mount_point}/{sdl3_framework}", sdl3_framework_target_path, symlinks=True)
            call(["hdiutil", "detach", mount_point, "-quiet"])

        if "CODE_SIGN_IDENTITY" in os.environ:
            call(["codesign", "--force", "--timestamp", "--sign", os.environ["CODE_SIGN_IDENTITY"], sdl3_framework_target_path])
        else:
            print("SDL will not be codesigned. Set the CODE_SIGN_IDENTITY environment variable if you want to sign it.")

    def package(self):
        release_config = self.build_configs[0]
        appdir = f"{self.dir_name}/{release_config}"

        # Human-friendly name for .app
        os.rename(f"{appdir}/{game_name}.app", f"{appdir}/{game_name_human}.app")

        self.copy_documentation(appdir)

        rm_if_exists(self.get_artifact_path())
        call(["hdiutil", "create",
            "-fs", "HFS+",
            "-srcfolder", appdir,
            "-volname", f"{game_name_human} {game_ver}",
            self.get_artifact_path()])


class LinuxProject(Project):
    def __init__(self, dir_name, config_name, custom_sdl_path, as_appimage):
        super().__init__(dir_name)

        self.gen_args += ["-DCMAKE_BUILD_TYPE=" + config_name]
        self.build_args += ["-j", str(NPROC)]
        self.build_configs = [config_name]

        if custom_sdl_path:
            self.gen_env["SDL3_DIR"] = custom_sdl_path
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

        return f"{game_name}-{game_ver}-linux-{MACHINE}{extension}"

    def prepare_dependencies(self):
        if self.use_system_sdl:
            return

        sdl_source_dir = f"{libs_dir}/SDL3-{sdl_ver}"
        sdl_build_dir = f"{sdl_source_dir}/build"
        sdl_prefix_dir = f"{sdl_source_dir}/install"
        rmtree_if_exists(sdl_source_dir)

        sdl_zip_path = get_package(f"https://libsdl.org/release/SDL3-{sdl_ver}.tar.gz")
        shutil.unpack_archive(sdl_zip_path, libs_dir)

        with chdir(sdl_source_dir):
            call(["cmake", "-S", ".", "-B", "build", f"-DCMAKE_INSTALL_PREFIX={sdl_prefix_dir}"])
            call(["cmake", "--build", sdl_build_dir, "-j", str(NPROC)])
            call(["cmake", "--install", sdl_build_dir])

    def package(self):
        if self.as_appimage:
            appdir = cache_dir + "/" + self.get_artifact_name(extension=".AppDir")
        else:
            appdir = self.get_artifact_path()

        assert appdir.endswith(".AppDir")
        rmtree_if_exists(appdir)

        # Prepare directory tree before copying files
        for d in ["", "usr/bin", "usr/lib", "usr/share/metainfo", "usr/share/applications"]:
            os.makedirs(f"{appdir}/{d}", exist_ok=True)

        # Copy executable and assets
        shutil.copy(f"{self.dir_name}/{game_name}", f"{appdir}/usr/bin")  # executable
        shutil.copytree("Data", f"{appdir}/Data")
        self.copy_documentation(appdir, everything=False)

        # Copy XDG stuff
        shutil.copy(f"packaging/{game_package}.desktop", appdir)
        shutil.copy(f"packaging/{game_package}.png", appdir)
        shutil.copy(f"packaging/{game_package}.appdata.xml", f"{appdir}/usr/share/metainfo")
        shutil.copy(f"packaging/{game_package}.desktop", f"{appdir}/usr/share/applications")  # must copy desktop file there as well, for validation

        # Copy AppImage kicker script
        shutil.copy(f"packaging/AppRun", appdir)
        os.chmod(f"{appdir}/AppRun", 0o755)

        # Copy SDL (if not using system SDL)
        if not self.use_system_sdl:
            for file in glob.glob(f"{libs_dir}/SDL3-{sdl_ver}/install/lib/libSDL3*.so*"):
                shutil.copy(file, f"{appdir}/usr/lib", follow_symlinks=False)

        # Invoke appimagetool
        if self.as_appimage:
            appimagetool_path = get_package(f"https://github.com/AppImage/appimagetool/releases/download/{appimagetool_ver}/appimagetool-{MACHINE}.AppImage")
            os.chmod(appimagetool_path, 0o755)

            rm_if_exists(self.get_artifact_path())
            call([appimagetool_path, appdir, self.get_artifact_path()])

#----------------------------------------------------------------

if __name__ == "__main__":
    # Make sure we're running from the correct directory...
    os.chdir(root_dir)

    #----------------------------------------------------------------
    # Set up project metadata

    if SYSTEM == "Windows":
        project = WindowsProject(build_dir)

    elif SYSTEM == "Darwin":
        project = MacProject(build_dir)

    elif SYSTEM == "Linux":
        custom_sdl_path = ""
        if not args.system_sdl:
            custom_sdl_path = f"{libs_dir}/SDL3-{sdl_ver}/install"
        project = LinuxProject(build_dir, "RelWithDebInfo", custom_sdl_path, not args.no_appimage)
    else:
        die(f"Unsupported system for configure step: {SYSTEM}")

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

    fatlog(f"{game_name} {game_ver} build script")

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
        fatlog(f"Building the game: {project.dir_name}")
        project.build()

    #----------------------------------------------------------------
    # Package the game

    if args.package:
        fatlog(f"Packaging the game")
        os.makedirs(dist_dir, exist_ok=True)
        project.package()
