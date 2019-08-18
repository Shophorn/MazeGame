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
outputName 	= "mazegame.exe"
flags 		= "-static -std=c++17"
definitions = "-DMAZEGAME_DEVELOPMENT=1"
sources 	= "src/win_Mazegame.cpp src/Mazegame.cpp"
includePath	= "-Iinclude -IC:/VulkanSDK/1.1.108.0/Include"
libPath 	= "-Llib -LC:/VulkanSDK/1.1.108.0/Lib"
libLinks	= "-llibglfw3 -lvulkan-1 -lgdi32 -lws2_32 -lole32 -lwinmm"

call = "clang++ {} {} {} -o {} {} {} {}".format(
			flags, definitions, includePath, outputName,
			sources, libPath, libLinks)

if not silent:
	print (call)

result = subprocess.run(call)

if not silent:
	if result.returncode == 0:
		print ("Great SUCCESS")
		# print ('\033[92m' + "Great SUCCESS" + '\033[0m')
	else:
		print ("Did NOT WORK")

exit(result.returncode)

