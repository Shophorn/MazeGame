'''
Leo Tamminen

Build program for :MAZEGAME:. Compiles game and platform layer programs
using clang++.
'''
import subprocess
import os
import sys

silent = '--silent' in sys.argv

# Build settings
flags 		= "-static -std=c++17"
definitions = "-DMAZEGAME_DEVELOPMENT=1"
includePath	= "-Iinclude -IC:/VulkanSDK/1.1.108.0/Include"
libPath 	= "-Llib -LC:/VulkanSDK/1.1.108.0/Lib"
libLinks	= "-llibglfw3 -lvulkan-1 -lgdi32 -lws2_32 -lole32 -lwinmm"



### COMPILE PLATFORM LAYER
platform_call = "clang++ {} {} {} -o win_Mazegame.exe src/win_Mazegame.cpp {} {}".format(
			flags, definitions, includePath, libPath, libLinks)

if not silent:
	print ("Compiling platform code")
	print ("\t" + platform_call)

platform_result = subprocess.run(platform_call)

if not silent:
	if result.returncode == 0:
		print ("\tGreat SUCCESS")
		# print ('\033[92m' + "Great SUCCESS" + '\033[0m')
	else:
		print ("\tDid NOT WORK ({})".format(platform_result.returncode))


### COMPILE GAME CODE DLL
game_call = "clang++ -shared {} {} {} -o Mazegame.dll src/Mazegame.cpp -DLL {} {}".format(
			flags, definitions, includePath, libPath, libLinks)

if not silent:
	print ("Compiling Game Code")
	print ("\t" + game_call)

game_result = subprocess.run(game_call)

if not silent:
	if result.returncode == 0:
		print ("\tGreat SUCCESS")
		# print ('\033[92m' + "Great SUCCESS" + '\033[0m')
	else:
		print ("\tDid NOT WORK ({})".format(game_result.returncode))


if (platform_result.returncode == 0) and (game_result.returncode == 0):
	exit(0)
else:
	exit(1)

# Bed image error 0xc0 00 00 20

# '''
# Leo Tamminen

# Build program for :MAZEGAME:. Compiles game and platform layer programs
# using clang++.
# '''
# import subprocess
# import os
# import sys

# silent = '--silent' in sys.argv

# # Build settings
# outputName 	= "mazegame.exe"
# flags 		= "-static -std=c++17"
# definitions = "-DMAZEGAME_DEVELOPMENT=1"
# sources 	= "src/win_Mazegame.cpp src/Mazegame.cpp"
# includePath	= "-Iinclude -IC:/VulkanSDK/1.1.108.0/Include"
# libPath 	= "-Llib -LC:/VulkanSDK/1.1.108.0/Lib"
# libLinks	= "-llibglfw3 -lvulkan-1 -lgdi32 -lws2_32 -lole32 -lwinmm"

# call = "clang++ {} {} {} -o {} {} {} {}".format(
# 			flags, definitions, includePath, outputName,
# 			sources, libPath, libLinks)

# if not silent:
# 	print (call)

# result = subprocess.run(call)

# if not silent:
# 	if result.returncode == 0:
# 		print ("Great SUCCESS")
# 		# print ('\033[92m' + "Great SUCCESS" + '\033[0m')
# 	else:
# 		print ("Did NOT WORK")

# exit(result.returncode)

