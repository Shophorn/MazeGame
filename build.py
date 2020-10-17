'''
Leo Tamminen

Build program for :MAZEGAME:. Compiles game and platform layer programs
using clang++.
'''
import subprocess
import os
import sys
import psutil
from datetime import datetime

# from termcolor import cprint

buildTime = datetime.now().strftime("%B %d, %Y, %H:%M:%S")
buildTime = '\\""{}\\""'.format(buildTime)


silent = '--silent' in sys.argv
# compiler = 'clang++'

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
	if not silent:
		print ("Call: " + call)

	result = subprocess.run(call)

	if not silent:
		if result.returncode == 0:
			print ("\tGreat SUCCESS")
			# print ('\033[92m' + "Great SUCCESS" + '\033[0m')
		else:
			print ("\tDid NOT WORK ({})".format(result.returncode))	

	return result.returncode

vulkan_sdk = os.environ['VULKAN_SDK']
print (vulkan_sdk)

devbuild = False
devbuild = True

### BUILD DEVELOPMENT GAME WITH SEPARATE GAME CODE DLL
if devbuild:
	# Build settings
	flags 		= "-static -std=c++17 -g -gcodeview -O0 -Werror"
	# flags 		= "-static -std=c++17 -O3"

	definitions = "-DFS_DEVELOPMENT -DFS_VULKAN_USE_VALIDATION=1 -DBUILD_DATE_TIME=" + buildTime
	includePath	= "-Iinclude -I{}/Include".format(vulkan_sdk)
	libPath 	= "-L{}/Lib".format(vulkan_sdk)
	libLinks	= "-lvulkan-1 -luser32 -lgdi32 -lws2_32 -lole32 -lwinmm"

	compiler_path = "clang++"

	### COMPILE PLATFORM LAYER
	platform_result = 0
	if compile_platform:
		# Specify '-mwindows' to get .exe to launch without console
		platform_call = "{} {} {} {} -o win32_friendsimulator.exe src/fswin32_friendsimulator.cpp -ferror-limit=100 {} {}".format(
					compiler_path, flags, definitions, includePath, libPath, libLinks)
		
		platform_result = compile(platform_call)

	### COMPILE GAME CODE DLL
	game_result = 0
	if compile_game:
		game_call = "{} -shared {} {} {} -o friendsimulator.dll src/friendsimulator.cpp -DLL".format(
					compiler_path, flags, definitions, includePath)

		game_result = compile(game_call)

		if game_result == 0:
			checkfile = open('friendsimulator_game_dll_built_checkfile', 'w')
			checkfile.close()

else:
	# Build settings
	# flags 		= "-static -std=c++17 -g -gcodeview -O0 -Werror"
	flags 		= "-static -std=c++17 -O0 -Werror"

	definitions = "-DFS_FULL_GAME -DBUILD_DATE_TIME=" + buildTime
	includePath	= "-Iinclude -I{}/Include".format(vulkan_sdk)
	libPath 	= "-L{}/Lib".format(vulkan_sdk)
	libLinks	= "-lvulkan-1 -luser32 -lgdi32 -lws2_32 -lole32 -lwinmm"

	compiler_path = "clang++"

	### COMPILE PLATFORM LAYER
	platform_call = "{} {} {} {} -o win32_friendsimulator_distributable.exe src/fswin32_friendsimulator.cpp {} {}".format(
				compiler_path, flags, definitions, includePath, libPath, libLinks)
	
	platform_result = compile(platform_call)
	game_result 	= 0

if (platform_result == 0) and (game_result == 0):
	exit (0)
else:
	exit(1)
