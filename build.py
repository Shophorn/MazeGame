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
	flags 		= "-static -std=c++17"
	definitions = "-DMAZEGAME_DEVELOPMENT=1"
	includePath	= "-Iinclude -IC:/VulkanSDK/1.1.108.0/Include"
	libPath 	= "-LC:/VulkanSDK/1.1.108.0/Lib"
	libLinks	= "-lvulkan-1 -lgdi32 -lws2_32 -lole32 -lwinmm"


	### COMPILE PLATFORM LAYER
	platform_call = "clang++ {} {} {} -o win_Mazegame.exe src/win_Mazegame.cpp {} {}".format(
				flags, definitions, includePath, libPath, libLinks)

	### COMPILE GAME CODE DLL
	game_call = "clang++ -shared {} {} {} -o Mazegame.dll src/Mazegame.cpp -DLL {} {}".format(
				flags, definitions, includePath, libPath, libLinks)

platform_result = compile(platform_call)
game_result = compile(game_call)

if (platform_result == 0) and (game_result == 0):
	exit (0)
else:
	exit(1)
