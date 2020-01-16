/*=============================================================================
Leo Tamminen

Interface definition between Platform and Game.
===============================================================================*/

#if !defined MAZEGAME_PLATFORM_HPP

#define MAZEGAME_INCLUDE_STD_IOSTREAM 1
#include <iostream>
#include <functional>

#if MAZEGAME_DEVELOPMENT
	#if MAZEGAME_INCLUDE_STD_IOSTREAM
	void PrintAssert(const char * file, int line, const char * message, const char * expression = nullptr)
	{
		std::cout << "Assertion failed [" << file << ":" << line << "]: \"" << message << "\"";
			
		if (expression)
			std::cout << ", expression (" << expression << ")";

		std::cout << "\n" << std::flush;
	}
	#endif

	#define DEVELOPMENT_BAD_PATH(msg) PrintAssert(__FILE__, __LINE__, msg); abort();

	#define DEVELOPMENT_ASSERT(expr, msg) if (!(expr)) { PrintAssert(__FILE__, __LINE__, msg, #expr); abort(); }

	#define DEVELOPMENT_ASSERT_POINTER(ptr, msg) if((ptr) == nullptr) { PrintAssert(__FILE__, __LINE__, msg), abort(); }

	// Note(Leo): Some things need to asserted in production too, this is a reminder for those only.
	#define PRODUCTION_ASSERT DEVELOPMENT_ASSERT


#endif

#include "MazegameEssentials.hpp"
#include "Array.hpp"

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

namespace platform
{
	struct RenderingOptions
	{
	    bool32 	enableDepth 		= true;
	    bool32 	clampDepth 			= false;
	    s32 	textureCount 		= 0;
	    u32	pushConstantSize 	= 0;

	    enum { PRIMITIVE_LINE, PRIMITIVE_TRIANGLE } primitiveType = PRIMITIVE_TRIANGLE;
	};


	/* Note(Leo): these are defined in platform layer, and
	can (and are supposed to) be used as opaque handles in
	game layer*/
	struct Graphics;

	using PushMeshFunc = MeshHandle (Graphics*, MeshAsset * asset);
	using PushTextureFunc = TextureHandle (Graphics*, TextureAsset * asset);
	using PushCubemapFunc = TextureHandle (Graphics*, StaticArray<TextureAsset, 6> * asset);

	using PushMaterialFunc = MaterialHandle	(Graphics*, MaterialAsset * asset);
	using PushGuiMaterialFunc = MaterialHandle (Graphics*, TextureHandle texture);

	// Todo(Leo): Remove 'PushModelFunc', we can render also just passing mesh and material handles directly
	using PushModelFunc = ModelHandle (Graphics*, MeshHandle mesh, MaterialHandle material);
	using PushPipelineFunc = PipelineHandle (Graphics*, char const * vertexShaderPath, char const * fragmentShaderPath, RenderingOptions options);

	using UnloadSceneFunc = void(Graphics*);

	using UpdateCameraFunc = void(Graphics*, Matrix44 view, Matrix44 perspective);
	using PrepareFrameFunc = void(Graphics*);
	using FinishFrameFunc = void (Graphics*);

	using DrawModelFunc = void(Graphics*, ModelHandle model, Matrix44 transform);
	using DrawLineFunc = void(Graphics*, float3 start, float3 end, float4 color);
	using DrawGuiFunc = void(Graphics*, float2 position, float2 size, MaterialHandle material, float4 color);

	struct Window;

	using GetWindowWidthFunc = u32 (Window const *);
	using GetWindowHeightFunc = u32 (Window const *);
	using IsWindowFullScreenFunc = bool32(Window const *);
	using SetWindowFullscreenFunc = void (Window*, bool32 value);

	struct Functions
	{
		// GRAPHICS FUNCTIONS
		PushMeshFunc * 			push_mesh;
		PushTextureFunc * 		push_texture;
		PushCubemapFunc * 		push_cubemap;

		PushMaterialFunc * 		push_material;
		PushGuiMaterialFunc * 	push_gui_material;

		PushModelFunc * 		push_model;
		PushPipelineFunc * 		push_pipeline;
		UnloadSceneFunc * 		unload_scene;

		UpdateCameraFunc * 		update_camera;
		PrepareFrameFunc * 		prepare_drawing;
		FinishFrameFunc * 		finish_drawing;
		DrawModelFunc * 		draw_model;
		DrawLineFunc * 			draw_line;
		DrawGuiFunc * 			draw_gui;
		
		// WINDOW FUNCTIONS	
		GetWindowWidthFunc * 		get_window_width;
		GetWindowHeightFunc * 		get_window_height;
		IsWindowFullScreenFunc * 	is_window_fullscreen;
		SetWindowFullscreenFunc * 	set_window_fullscreen;
	};
}

namespace game
{
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

	internal ButtonState
	update_button_state(ButtonState buttonState, bool32 buttonIsPressed)
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
				DEVELOPMENT_BAD_PATH("Invalid ButtonState value");
		}

		return buttonState;
	}

	struct Input
	{
		float2 move;
		float2 look;


		// Todo(Leo): Do a proper separate mapping struct of meanings of specific buttons
		ButtonState jump;
		ButtonState confirm;
		ButtonState interact;

		// Note(Leo): Start and Select as in controller
		ButtonState start;
		ButtonState select;

		ButtonState left;
		ButtonState right;
		ButtonState down;
		ButtonState up;

		ButtonState zoomIn;
		ButtonState zoomOut;

		real32 elapsedTime;
	};
	
	struct Memory
	{
		bool32 isInitialized;

		void * persistentMemory;
		u64 persistentMemorySize;

		void * transientMemory;
		u64 transientMemorySize;
	};

	
	struct NetworkPackage
	{
		Vector3 characterPosition;
		Quaternion characterRotation;
	};

	constexpr s32 NETWORK_PACKAGE_SIZE = sizeof(NetworkPackage);
	static_assert(NETWORK_PACKAGE_SIZE <= 512, "Time to deal with bigger network packages");

	struct Network
	{
		bool32 isConnected;

		NetworkPackage inPackage;
		NetworkPackage outPackage;
	};

	struct StereoSoundSample
	{
		float left;
		float right;
	};

	struct SoundOutput
	{
		s32 sampleCount;
		StereoSoundSample * samples;
	};
}

#if MAZEGAME_INCLUDE_STD_IOSTREAM
namespace std
{
	ostream & operator << (ostream & os, game::ButtonState buttonState)
	{
		switch (buttonState)
		{
			case game::ButtonState::IsUp: 		os << "IsUp"; 		break;
			case game::ButtonState::WentDown: 	os << "WentDown"; 	break;
			case game::ButtonState::WentUp: 	os << "WentUp"; 	break;
			case game::ButtonState::IsDown: 	os << "IsDown"; 	break;

			default: os << "INVALID ButtonState VALUE";
		}

		return os;
	}
}

#endif

// TODO(Leo): can we put this into namespace? Should we?
extern "C" bool32
GameUpdate(
	game::Input * 			input,
	game::Memory * 			memory,
	game::Network *			network,
	game::SoundOutput *		soundOutput,

	// Are these understandable enough?
	platform::Graphics*,
	platform::Window*,
	platform::Functions*);

#define MAZEGAME_PLATFORM_HPP
#endif