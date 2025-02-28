import argparse, os, textwrap

parser = argparse.ArgumentParser()
parser.add_argument("--cc", default="/gcc/7.3.0/bin/gcc-7")
parser.add_argument("--cxx", default="/gcc/7.3.0/bin/g++-7")
parser.add_argument("--cpu", default="G3")
parser.add_argument("--sdk", default="/Developer/SDKs/MacOSX10.4u.sdk")
parser.add_argument("--debug", default=False, action='store_true')
parser.add_argument("-o", default="Makefile")
args = parser.parse_args()

SOURCE_DIR = "../src"

INCLUDE_DIRS = [
	"../extern/Pomme/src",
	"../extern/Pomme/src/QD3D",
	"sdl-2.0.3/include",
]

EXCLUDE_ROOTS = [
	"../extern/Pomme/src/Platform/Windows",
	"../extern/Pomme/src/Graphics",
	"../extern/Pomme/src/Input",
	"../extern/Pomme/src/Video",
]

EXCLUDE_FILES = [
	"../extern/Pomme/src/SoundFormats/mp3.cpp",
	"../src/System/KillMacMouseAcceleration.c",
]

CONFIG = 'debug' if args.debug else 'release'
CPU = args.cpu
CC = args.cc
CXX = args.cxx
CCSYSROOT = "/gcc/7.3.0"
OBJECT_DIR = f"obj/{CPU.lower()}_{CONFIG}"
POMME_SOURCE_DIR = "../extern/Pomme/src"
PROGRAM_NAME = f"Bugdom_{CPU}_{CONFIG}"
SDK = args.sdk
CFLAGS = (
	f"-mcpu={CPU} "
	f"--sysroot {CCSYSROOT} "
	f"-isysroot {SDK} "
	"-fexceptions "
	"-Wall -Wextra -Wno-multichar -Werror=implicit-function-declaration -Werror=return-type -Wstrict-aliasing=2 "
)
for id in INCLUDE_DIRS:
	CFLAGS += f"-I{id} "

if args.debug:
	CFLAGS += "-O0 -g -D_DEBUG "
else:
	CFLAGS += "-O3 "

CXXFLAGS = CFLAGS + " -Wno-unused-parameter"
LDFLAGS = f"-mcpu={CPU}"

CFLAGS += " -std=gnu11 "
CXXFLAGS += " -std=gnu++17 "


class Lib:
	def __init__(self, name):
		self.name = name
		self.rules = []
		self.objs = []
		self.cflags = CFLAGS
		self.cxxflags = CXXFLAGS
	
	@property
	def lib_path(self):
		return f"{OBJECT_DIR}/{self.name}.a"

	def add_flags(self, newflags):
		self.cflags += " " + newflags
		self.cxxflags += " " + newflags

	def make_rules(self):
		obj_string = ' \\\n\t\t\t'.join(self.objs)

		out = (
			f"{self.lib_path}: {obj_string}\n"
			"\t@echo $@\n"
			"\t@ar -rcs $@ $^\n"
			"\n"
		)
		out += "\n".join(self.rules)
		out += "\n\n"
		return out

	def add_source_tree(self, src_dir):
		wildcard_c = f"{src_dir}/%.c"
		wildcard_cpp = f"{src_dir}/%.cpp"
		self.rules.append(
			f"{src_to_obj(wildcard_c, src_dir, self.name)}: {wildcard_c}\n"
			f"\t@echo $@\n"
			f"\t@mkdir -p $(@D)\n"
			f"\t@{CC} {self.cflags} -c $< -o $@\n"
			f"\n"
			f"{src_to_obj(wildcard_cpp, src_dir, self.name)}: {wildcard_cpp}\n"
			f"\t@echo $@\n"
			f"\t@mkdir -p $(@D)\n"
			f"\t@{CXX} {self.cxxflags} -c $< -o $@\n"
			f"\n"
		)

		for root, dirs, files in os.walk(src_dir):
			if root in EXCLUDE_ROOTS:
				continue

			for file in files:
				src_path = root + "/" + file

				if (file.endswith(".c") or file.endswith(".cpp")) and (src_path not in EXCLUDE_FILES):
					self.objs.append(src_to_obj(src_path, src_dir, self.name))


def src_to_obj(src, crufty_prefix_path, forced_obj_prefix):
	obj = src + ".o"
	crufty_prefix = crufty_prefix_path + "/"
	if obj.startswith(crufty_prefix):
		obj = obj[len(crufty_prefix):]
	obj = forced_obj_prefix + "/" + obj
	return os.path.join(OBJECT_DIR, obj)

pomme_lib = Lib("pomme")
pomme_lib.add_flags("-DPOMME_NO_GRAPHICS -DPOMME_NO_INPUT -DPOMME_NO_MP3")
pomme_lib.add_source_tree("../extern/Pomme/src")

game_lib = Lib("game")
game_lib.add_flags(f"-I{SOURCE_DIR}/Headers")
game_lib.add_flags("-DNOJOYSTICK -DOSXPPC")
game_lib.add_source_tree("../src")

makefile = f"""# {PROGRAM_NAME} for Mac OS X PowerPC

{PROGRAM_NAME}: {game_lib.lib_path} {pomme_lib.lib_path} libSDL2.a
	{CXX} {LDFLAGS} $^ -o $@ \\
		-static-libstdc++ -static-libgcc \\
		-lm -liconv -Wl,-framework,OpenGL \\
		-lobjc -Wl,-framework,Carbon -Wl,-framework,Cocoa -Wl,-framework,IOKit -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,AudioUnit \\
		--sysroot {CCSYSROOT} \\
		-isysroot {SDK}
	{'strip $@' if not args.debug else ''}

clean:
	rm -rf {OBJECT_DIR}
	rm -f {PROGRAM_NAME}
	rm -rf {PROGRAM_NAME}.app

app: {PROGRAM_NAME}.app

{PROGRAM_NAME}.app: {PROGRAM_NAME}
	rm -rf {PROGRAM_NAME}.app
	python3 mkppcapp.py --exe {PROGRAM_NAME}

run: {PROGRAM_NAME}
	./{PROGRAM_NAME}

#------------------------------------------------------------------------------
# Game lib

{game_lib.make_rules()}

#------------------------------------------------------------------------------
# Pomme lib

{pomme_lib.make_rules()}
"""

with open(args.o, "wt") as f:
	f.write(makefile)
	print("Wrote " + f.name)
