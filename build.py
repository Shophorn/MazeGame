'''
Leo Tamminen

Build program for :MAZEGAME:. Compiles game and platform layer programs
using clang++.
'''
import subprocess
import os
import sys

silent = '--silent' in sys.argv
compiler = 'clang++'

compile_platform = True
compile_game = True

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

### CLANG++
if compiler == 'clang++':
	# Build settings
	flags 		= "-static -std=c++17 -g -gcodeview -O0"
	definitions = "-DMAZEGAME_DEVELOPMENT=1"
	includePath	= "-Iinclude -IC:/VulkanSDK/1.1.108.0/Include"
	libPath 	= "-LC:/VulkanSDK/1.1.108.0/Lib"
	libLinks	= "-lvulkan-1 -lgdi32 -lws2_32 -lole32 -lwinmm"

	# compiler_path = "C:/Programs/LLVM/bin/clang++"
	compiler_path = "clang++"

	### COMPILE PLATFORM LAYER
	platform_result = 0
	if compile_platform:
		# Specify '-mwindows' to get .exe to launch without console
		platform_call = "{} {} {} {} -o winapi_Mazegame.exe src/winapi_Mazegame.cpp {} {}".format(
					compiler_path, flags, definitions, includePath, libPath, libLinks)
		
		platform_result = compile(platform_call)

	### COMPILE GAME CODE DLL
	game_result = 0
	if compile_game:
		game_call = "{} -shared {} {} {} -o Mazegame.dll src/Mazegame.cpp -DLL".format(
					compiler_path, flags, definitions, includePath)

		game_result = compile(game_call)

if (platform_result == 0) and (game_result == 0):
	exit (0)
else:
	exit(1)
