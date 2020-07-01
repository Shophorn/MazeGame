/*=============================================================================
Leo Tamminen

Interface definition between Platform and Game.
===============================================================================*/

#if !defined MAZEGAME_PLATFORM_HPP

#define MAZEGAME_INCLUDE_STD_IOSTREAM 1
#include <iostream>

#include "MazegameEssentials.hpp"
#include "Logging.cpp"

#if MAZEGAME_DEVELOPMENT
	void log_assert(LogInput::FileAddress fileAddress, char const * message, char const * expression)
	{
		auto log = logDebug(0);
		log << fileAddress << "Assertion failed: ";

		if (message)
			log << message;

		if (expression)
			log << " (" << expression << ")";
	}

	#define Assert(expr) if(!(expr)) { log_assert(FILE_ADDRESS, nullptr, #expr); 			abort(); }
	#define AssertMsg(expr, msg) if(!(expr)) { log_assert(FILE_ADDRESS, msg, #expr); 	abort(); }

	// Note(Leo): Some things need to asserted in production too, this is a reminder for those only.
	#define AssertRelease AssertMsg
#else
	#define Assert(expr)
	#define AssertMsg(expr, msg)
	#define AssertRelease AssertMsg

#endif

#include "CStringUtility.cpp"
#include "SmallString.cpp"

// Note(Leo): Before assets...	
enum GraphicsPipeline : s64
{
	GRAPHICS_PIPELINE_NORMAL,
	GRAPHICS_PIPELINE_ANIMATED,
	GRAPHICS_PIPELINE_SKYBOX,
	GRAPHICS_PIPELINE_WATER,
	GRAPHICS_PIPELINE_SCREEN_GUI,
	GRAPHICS_PIPELINE_LEAVES,

	GRAPHICS_PIPELINE_COUNT
};


/* Note(Leo): This is called 'Unity-build'. Basically import all things in single
translation unit(??) to reduce pressure on linker and enable more optimizations.
Also makes actual build commands a lot cleaner and easier, since only this file
needs to be specified for compiler. */
#include "Math.cpp"
#include "Vectors.cpp"
#include "Quaternion.cpp"
#include "Matrices.cpp"

#include "Memory.cpp"
#include "Assets.cpp"

#include "Camera.cpp"
#include "Light.cpp"

struct ScreenRect
{
	v2 position;
	v2 size;
	v2 uvPosition;
	v2 uvSize;	
};

constexpr GuiTextureHandle GRAPHICS_RESOURCE_SHADOWMAP_GUI_TEXTURE = {-1};


/// ***********************************************************************
/// PLATFORM OPAQUE HANDLES
/* Note(Leo): these are defined in platform layer, and
can (and are supposed to) be used as opaque handles in
game layer. */

struct PlatformGraphics;
struct PlatformWindow;
// struct PlatformNetwork;
// struct PlatformAudio;

/// ***********************************************************************
/// PLATFORM VALUE TYPES

struct PlatformNetworkPackage
{
	v3 			characterPosition;
	quaternion 	characterRotation;
};

constexpr s32 NETWORK_PACKAGE_SIZE = sizeof(PlatformNetworkPackage);
static_assert(NETWORK_PACKAGE_SIZE <= 512, "Time to deal with bigger network packages");


enum PlatformGraphicsFrameResult
{
	PGFR_FRAME_OK,
	PGFR_FRAME_RECREATE,
	PGFR_FRAME_BAD_PROBLEM,
};

struct PlatformStereoSoundSample
{
	float left;
	float right;
};

// Todo(Leo): These buttonstate things are implementation details and may even vary per platform,
// so consider moving them elsewhere. Maybe only expose is_clicked etc. as
enum struct ButtonState : s8
{
	IsUp,
	WentDown,
	WentUp,
	IsDown
};

bool32 is_clicked (ButtonState button) { return button == ButtonState::WentDown; } 
bool32 is_released (ButtonState button) { return button == ButtonState::WentUp; } 
bool32 is_pressed (ButtonState button) { return (button == ButtonState::IsDown) || (button == ButtonState::WentDown);}

internal ButtonState update_button_state(ButtonState buttonState, bool32 buttonIsPressed)
{
	switch(buttonState)
	{
		case ButtonState::IsUp:
			buttonState = buttonIsPressed ? ButtonState::WentDown : ButtonState::IsUp;
			break;

		case ButtonState::WentDown:
			buttonState = buttonIsPressed ? ButtonState::IsDown : ButtonState::WentUp;
			break;

		case ButtonState::WentUp:
			buttonState = buttonIsPressed ? ButtonState::WentDown : ButtonState::IsUp;
			break;

		case ButtonState::IsDown:
			buttonState = buttonIsPressed ? ButtonState::IsDown : ButtonState::WentUp;
			break;

		default:
			AssertMsg(false, "Invalid ButtonState value");
	}

	return buttonState;
}

struct PlatformTimePoint
{
	s64 value;
};

/// ***********************************************************************
/// PLATFORM POINTER TYPES

struct PlatformInput
{
	v2 move;
	v2 look;

	// Todo(Leo): Do a proper separate mapping struct of meanings of specific buttons
	ButtonState jump;
	ButtonState confirm;
	ButtonState interact;

	// Note(Leo): Nintendo mapping
	ButtonState X, Y, A, B;

	// Note(Leo): Start and Select as in controller
	ButtonState start;
	ButtonState select;

	ButtonState left;
	ButtonState right;
	ButtonState down;
	ButtonState up;

	ButtonState zoomIn;
	ButtonState zoomOut;
};

struct PlatformTime
{
	f32 elapsedTime;
};

struct PlatformMemory
{
	void * memory;
	u64 size;
};

struct PlatformNetwork
{
	bool32 isConnected;

	PlatformNetworkPackage inPackage;
	PlatformNetworkPackage outPackage;
};

struct PlatformSoundOutput
{
	s32 sampleCount;
	PlatformStereoSoundSample * samples;

	PlatformStereoSoundSample * begin() { return samples; }
	PlatformStereoSoundSample * end() { return samples + sampleCount; }
};

/// ***********************************************************************
/// PLATFORM API DESCRIPTION

struct PlatformApi
{
	// GRAPHICS SCENE FUNCTIONS
	MeshHandle 	(*push_mesh) 			(PlatformGraphics*, MeshAsset * asset);
	TextureHandle (*push_texture) 		(PlatformGraphics*, TextureAsset * asset);
	TextureHandle (*push_cubemap) 		(PlatformGraphics*, StaticArray<TextureAsset, 6> * asset);
	MaterialHandle (*push_material) 	(PlatformGraphics*, GraphicsPipeline, s32 textureCount, TextureHandle * textures);
	GuiTextureHandle (*push_gui_texture) (PlatformGraphics*, TextureAsset * asset);

	// Todo(Leo): Maybe remove 'push_model', we can render also just passing mesh and material handles directly
	ModelHandle (*push_model) (PlatformGraphics*, MeshHandle mesh, MaterialHandle material);

	void (*unload_scene) 	(PlatformGraphics*);
	void (*reload_shaders) 	(PlatformGraphics*);

	// GRAPHICS DRAW FUNCTIONS
	void (*prepare_frame) 	(PlatformGraphics*);
	void (*finish_frame) 	(PlatformGraphics*);
	void (*update_camera) 	(PlatformGraphics*, Camera const *);
	void (*update_lighting)	(PlatformGraphics*, Light const *, Camera const * camera, v3 ambient);
	void (*draw_model) 		(PlatformGraphics*, ModelHandle model, m44 transform, bool32 castShadow, m44 const * bones, u32 boneCount);

	void (*draw_meshes)			(PlatformGraphics*, s32 count, m44 const * transforms, MeshHandle mesh, MaterialHandle material);
	void (*draw_screen_rects)	(PlatformGraphics*, s32 count, ScreenRect const * rects, GuiTextureHandle texture, v4 color);
	void (*draw_lines)			(PlatformGraphics*, s32 pointCount, v3 const * points, v4 color);
	void (*draw_procedural_mesh)(	PlatformGraphics*,
									s32 vertexCount, Vertex const * vertices,
									s32 indexCount, u16 const * indices,
									m44 transform, MaterialHandle material);
	void (*draw_leaves)			(PlatformGraphics*, s32 count, m44 const * transforms);

	// WINDOW FUNCTIONS	
	u32 (*get_window_width) 		(PlatformWindow const *);
	u32 (*get_window_height) 		(PlatformWindow const *);
	bool32 (*is_window_fullscreen) 	(PlatformWindow const *);
	void (*set_window_fullscreen) 	(PlatformWindow*, bool32 value);

	// TIME FUNCTIONS
	PlatformTimePoint (*current_time) 	();
	f64 (*elapsed_seconds)				(PlatformTimePoint start, PlatformTimePoint end);
};

internal bool32 platform_all_functions_set(PlatformApi const * api)
{
	/* Note(Leo): this assumes that sizeof each pointer is same. 
	This site suggests that it is so, but I'm still not 100% sure.
	https://docs.oracle.com/cd/E19059-01/wrkshp50/805-4956/6j4mh6goi/index.html */

	using 			function_ptr = void(*)();
	u32 constexpr 	numFunctions = sizeof(PlatformApi) / sizeof(function_ptr);

	auto * funcArray = reinterpret_cast<function_ptr const *>(api);
	for (u32 i = 0; i < numFunctions; ++i)
	{
		if (funcArray[i] == nullptr)
		{
			logSystem() << FILE_ADDRESS << "Unset function at '" << i << "'\n";
			return false;
		}
	}
	return true;
};

using GameUpdateFunc = bool32(	const PlatformInput *,
								const PlatformTime *,
								PlatformMemory *,
								PlatformNetwork *,
								PlatformSoundOutput*,

								PlatformGraphics *,
								PlatformWindow *,
								PlatformApi *,

								std::ofstream * logFile);



#if MAZEGAME_INCLUDE_STD_IOSTREAM
namespace std
{
	ostream & operator << (ostream & os, ButtonState buttonState)
	{
		switch (buttonState)
		{
			case ButtonState::IsUp: 		os << "IsUp"; 		break;
			case ButtonState::WentDown: 	os << "WentDown"; 	break;
			case ButtonState::WentUp: 	os << "WentUp"; 	break;
			case ButtonState::IsDown: 	os << "IsDown"; 	break;

			default: os << "INVALID ButtonState VALUE";
		}

		return os;
	}
}

#endif


#define MAZEGAME_PLATFORM_HPP
#endif