'''
Leo Tamminen

Build program for :MAZEGAME:. Compiles game and platform layer programs
using clang++.
'''
import subprocess
import os
import sys
import psutil
import shutil
from datetime import datetime

buildTime = datetime.now().strftime("%B %d, %Y, %H:%M:%S")
buildTime = '\\""{}\\""'.format(buildTime)

silent = '--silent' in sys.argv

compile_platform = 1
compile_game = 1

for proc in psutil.process_iter():
	try:
		if 'friendsimulator' in proc.name().lower():
			print("Friendsimulator running, not building platform layer.")
			compile_platform = 0;
	except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
		pass

def compile(call):
	result = subprocess.run(call)

	if not silent:
		if result.returncode == 0:
			print ("[92mGreat SUCCESS[0m")
		else:
			print ("[91mDid NOT WORK ({})[0m".format(result.returncode))

	return result.returncode

vulkan_sdk = os.environ['VULKAN_SDK']
imgui_include = os.environ['LEO_EXTERNAL_LIBRARIES_INCLUDE']

platform_dependencies = (	"-I" + vulkan_sdk + "\Include -I" + imgui_include + " "
							"-L" + vulkan_sdk + "\Lib "
							"-lvulkan-1 -luser32 -lgdi32 -lws2_32 -lole32 -lwinmm"
						)


devbuild = not 'release' in sys.argv

### BUILD DEVELOPMENT GAME WITH SEPARATE GAME CODE DLL
if devbuild:
	print ("FRIENDSIMULATOR [95mDEVELOPMENT[0m BUILD")

	flags 		= "-static -std=c++17 -g -gcodeview -O0 -Werror "
	definitions = "-DFS_DEVELOPMENT -DFS_VULKAN_USE_VALIDATION=1 -DBUILD_DATE_TIME=" + buildTime + " "	

	os.chdir("development")

	### COMPILE PLATFORM LAYER
	platform_result = 0
	if compile_platform:
		print("Compile platform")
		call = (
			"clang++ "
			"../src/fswin32_friendsimulator.cpp "
			"-o win32_friendsimulator.exe "
			+ flags + definitions + platform_dependencies
		)

		platform_result = compile(call)

	### COMPILE GAME CODE DLL
	game_result = 0
	if compile_game:
		print("Compile game")
		call = (
			"clang++ "
			"-shared "
			"../src/friendsimulator.cpp "
			"-o friendsimulator.dll "
			"-I" + imgui_include + " "
			+ flags + definitions
		)
		
		game_result = compile(call)

		if game_result == 0:
			checkfile = open('friendsimulator_game_dll_built_checkfile', 'w')
			checkfile.close()

	os.chdir("..")

else:
	print ("FRIENDSIMULATOR [94mRELEASE[0m BUILD")

	shutil.rmtree("release", True)
	os.mkdir("release")

	call = (
		"clang++ "
		"src/fswin32_friendsimulator.cpp "
		"-o release/win32_friendsimulator_distributable.exe "
		"-static -std=c++17 -O3 -Werror "
		# "-ffast-math -fopenmp"
		"-DFS_RELEASE -DBUILD_DATE_TIME=" + buildTime + " "
		+ platform_dependencies
	)

	platform_result = compile(call)
	game_result 	= 0

	if platform_result == 0:
		shutil.copy("development/assets.fsa", "release/assets.fsa")
		shutil.copy("development/settings", "release/settings")
		shutil.copytree("development/shaders", "release/shaders")

		print ("Assets copied")


if (platform_result == 0) and (game_result == 0):
	exit (0)
else:
	exit(1)
