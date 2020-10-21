#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <type_traits>


#include "fs_essentials.hpp"
#include "platform_assets.cpp"
#include "game_assets.h"
#include "fs_asset_file_format.cpp"

// #include "asset_cooker_texture_loader.cpp"

#define EXPORT extern "C" __declspec(dllexport)

static int testThing;

static HANDLE 			global_outFile;
static AssetFileHeader 	global_header;
static u64 				global_dataPosition;

using cstring = char const *;


EXPORT int open_file(cstring filename);
EXPORT int close_file();

EXPORT int cook_mesh(MeshAssetId id, cstring filename, cstring gltfNodeName);
EXPORT int cook_texture(TextureAssetId id, cstring filename, TextureFormat format);
EXPORT int cook_skeleton(SkeletonAssetId id, cstring filename, cstring gltfNodeName);
EXPORT int cook_animation(SkeletonAssetId id, cstring filename, cstring animationName);
EXPORT int cook_audio(SoundAssetId id, cstring filename);
EXPORT int cook_font(FontAssetId id, cstring filename);

int open_file(cstring filename)
{
	std::cout << "OPEN FILE: " << filename << "\n";

	global_outFile = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, nullptr);
	WriteFile(global_outFile, "Hello from thing\n", 17, nullptr, nullptr);

	global_header 			= {};
	global_header.magic 	= AssetFileHeader::magicValue;
	global_header.version 	= AssetFileHeader::currentVersion;
	global_dataPosition 	= sizeof(AssetFileHeader);

	return 0;
}

int close_file()
{
	WriteFile(global_outFile, "Good bye!\n", 10, nullptr, nullptr);

	CloseHandle(global_outFile);

	std::cout << "File closed" << "\n";
	return 0;
}


int cook_mesh(MeshAssetId id, cstring filename, cstring gltfNodeName)
{
	std::cout << "COOK MESH: id = " << id << ", file = " << filename << ", node = " << gltfNodeName << "\n";
	return 0;
}

int cook_texture(TextureAssetId id, cstring filename, TextureFormat format)
{








	std::cout << "COOK TEXTURE: id = " << id << ", file = " << filename << ", format = " << format << "\n";
	return 0;
}

int cook_skeleton(SkeletonAssetId id, cstring filename, cstring gltfNodeName)
{
	std::cout << "COOK SKELETON: id = " << id << ", file = " << filename << ", node = " << gltfNodeName << "\n";
	return 0;
}

int cook_animation(SkeletonAssetId id, cstring filename, cstring animationName)
{
	std::cout << "COOK ANIMATION: id = " << id << ", file = " << filename << ", animation = " << animationName << "\n";
	return 0;
}

int cook_audio(SoundAssetId id, cstring filename)
{
	std::cout << "COOK AUDIO: id = " << id << ", file = " << filename << "\n";
	return 0;
}

int cook_font(FontAssetId id, cstring filename)
{
	std::cout << "COOK FONT: id = " << id << ", file = " << filename << "\n";
	return 0;
}

