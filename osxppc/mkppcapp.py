import os
import shutil
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--exe", required=True)
args = parser.parse_args()

GAME = "Bugdom"
EXE = args.exe
VERSION = "1.3.3"
DOMAIN = "io.jor.bugdom"
COPYRIGHT = f"Bugdom version {VERSION}, © 1999 Pangea Software, Inc., © 2022 Iliyas Jorio"
APPDIR = f"{EXE}.app"
ICNS = "../packaging/Bugdom.icns"

plist_xml = f"""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>{EXE}</string>
	<key>CFBundleGetInfoString</key>
	<string>{COPYRIGHT}</string>
	<key>CFBundleShortVersionString</key>
	<string>{VERSION}</string>
	<key>CFBundleVersion</key>
	<string>{VERSION}</string>
	<key>CFBundleIconFile</key>
	<string>Bugdom.icns</string>
	<key>CFBundleIdentifier</key>
	<string>{DOMAIN}</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>{GAME}</string>
</dict>
</plist>
"""

os.makedirs(f"{APPDIR}/Contents/MacOS")
#os.makedirs(f"{APPDIR}/Contents/Resources")

with open(f"{APPDIR}/Contents/Info.plist", "wt", encoding="utf-8") as plist:
    plist.write(plist_xml)

shutil.copytree("../Data", f"{APPDIR}/Contents/Resources")
shutil.copy(ICNS, f"{APPDIR}/Contents/Resources")
shutil.copy(EXE, f"{APPDIR}/Contents/MacOS")
