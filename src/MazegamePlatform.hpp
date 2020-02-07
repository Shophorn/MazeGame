/*=============================================================================
Leo Tamminen

Interface definition between Platform and Game.
===============================================================================*/

#if !defined MAZEGAME_PLATFORM_HPP

#define MAZEGAME_INCLUDE_STD_IOSTREAM 1
#include <iostream>

#if MAZEGAME_DEVELOPMENT
	#include <cassert>

	#if MAZEGAME_INCLUDE_STD_IOSTREAM
	void PrintAssert(const char * file, int line, const char * message, const char * expression = nullptr)
	{
		std::cout << "Assertion failed [" << file << ":" << line << "]: \"" << message << "\"";
			
		if (expression)
			std::cout << ", expression (" << expression << ")";

		std::cout << "\n" << std::flush;
	}
	#endif

	#define DEBUG_ASSERT(expr, msg) if (!(expr)) { PrintAssert(__FILE__, __LINE__, msg, #expr); abort(); }

	// Note(Leo): Some things need to asserted in production too, this is a reminder for those only.
	#define RELEASE_ASSERT DEBUG_ASSERT


#endif

#include "MazegameEssentials.hpp"

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

namespace platform
{
	enum FrameResult
	{
		FRAME_OK,
		FRAME_RECREATE,
		FRAME_BAD_PROBLEM,
	};

	struct RenderingOptions
	{
	    bool32 	enableDepth 		= true;
	    bool32 	clampDepth 			= false;
	    s32 	textureCount 		= 0;
	    u32		pushConstantSize 	= 0;

	    float 	lineWidth 			= 1.0f;
	    bool32	useVertexInput		= true;

	    enum {
	    	PRIMITIVE_LINE,
    		PRIMITIVE_TRIANGLE,
    		PRIMITIVE_TRIANGLE_STRIP
	    } primitiveType = PRIMITIVE_TRIANGLE;

	    enum {
	    	CULL_BACK,
	    	CULL_FRONT,
	    	CULL_NONE
	    } cullMode = CULL_BACK;

	    bool32 useSceneLayoutSet 	= true;
	    bool32 useMaterialLayoutSet = true;
	    bool32 useModelLayoutSet 	= true;
	    bool32 useLighting			= true;
	};

	/* 
	Note(Leo): this is honest forward declaration to couple lines downward
	Note(Leo): this will not be needed in final game, where game is same
	executable instead of separate dll like now.
	*/
	struct Functions;

	/* Note(Leo): these are defined in platform layer, and
	can (and are supposed to) be used as opaque handles in
	game layer*/
	struct Graphics;

	// Note(Leo): these are for platform layer only.
	FrameResult prepare_frame(Graphics*);
	void set_functions(Graphics*, Functions*);

	struct Window;
	// struct Network;
	// struct Audio;

	struct Functions
	{
		// GRAPHICS SCENE FUNCTIONS
		MeshHandle 	(*push_mesh) 			(Graphics*, MeshAsset * asset);
		TextureHandle (*push_texture) 		(Graphics*, TextureAsset * asset);
		TextureHandle (*push_cubemap) 		(Graphics*, StaticArray<TextureAsset, 6> * asset);
		MaterialHandle (*push_material) 	(Graphics*, MaterialAsset * asset);
		MaterialHandle (*push_gui_material) (Graphics*, TextureHandle texture);

		// Todo(Leo): Maybe remove 'push_model', we can render also just passing mesh and material handles directly
		ModelHandle (*push_model) 			(Graphics*, MeshHandle mesh, MaterialHandle material);
		PipelineHandle (*push_pipeline) 	(Graphics*, char const * vertexShaderFilename,
														char const * fragmentShaderFilename,
														RenderingOptions optios);
		void (*unload_scene) 	(Graphics*);

		// GRAPHICS DRAW FUNCTIONS
		void (*prepare_frame) 	(Graphics*);
		void (*finish_frame) 	(Graphics*);
		void (*update_camera) 	(Graphics*, Camera const *);
		void (*update_lighting)	(Graphics*, Light const *, Camera const * camera, float3 ambient);
		void (*draw_model) 		(Graphics*, ModelHandle model, Matrix44 transform, bool32 castShadow);
		void (*draw_line) 		(Graphics*, vector3 start, vector3 end, float width, float4 color);
		void (*draw_gui) 		(Graphics*, vector2 position, vector2 size, MaterialHandle material, float4 color);

		// WINDOW FUNCTIONS	
		u32 (*get_window_width) 		(Window const *);
		u32 (*get_window_height) 		(Window const *);
		bool32 (*is_window_fullscreen) 	(Window const *);
		void (*set_window_fullscreen) 	(Window*, bool32 value);
	};


	internal bool32
	all_functions_set(Functions const * functions)
	{
		/* Note(Leo): this assumes that sizeof each pointer is same. 
		This site suggests that it is so, but I'm still not 100% sure.
		https://docs.oracle.com/cd/E19059-01/wrkshp50/805-4956/6j4mh6goi/index.html */

		using 			function_ptr = void(*)();
		u32 constexpr 	numFunctions = sizeof(Functions) / sizeof(function_ptr);

		auto * funcArray = reinterpret_cast<function_ptr const *>(functions);
		for (u32 i = 0; i < numFunctions; ++i)
		{
			if (funcArray[i] == nullptr)
			{
				std::cout << "[all_functions_set()]: Unset function at '" << i << "'\n";
				return false;
			}
		}
		return true;
	};

	struct Memory
	{
		void * memory;
		u64 size;
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
				DEBUG_ASSERT(false, "Invalid ButtonState value");
		}

		return buttonState;
	}

	struct Input
	{
		vector2 move;
		vector2 look;

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

		float elapsedTime;
	};
	
	
	struct NetworkPackage
	{
		vector3 characterPosition;
		quaternion characterRotation;
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
update_game(
	game::Input * 			input,
	platform::Memory * 		memory,
	game::Network *			network,
	game::SoundOutput *		soundOutput,

	// Are these understandable enough?
	platform::Graphics*,
	platform::Window*,
	platform::Functions*);

#define MAZEGAME_PLATFORM_HPP
#endif