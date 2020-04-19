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
	#define Assert(expr) if (!(expr)) { logDebug(0) << FILE_ADDRESS << "Assertion failed: " << #expr; abort(); }
	#define DEBUG_ASSERT(expr, msg) if (!(expr)) { logDebug(0) << FILE_ADDRESS << "Assertion failed: " << msg << "(" << #expr << ")"; abort(); }

	// Note(Leo): Some things need to asserted in production too, this is a reminder for those only.
	#define RELEASE_ASSERT DEBUG_ASSERT
#endif

#include "CStringUtility.cpp"
#include "SmallString.cpp"
	

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
	    s32 textureCount 		= 0;
	    u32	pushConstantSize 	= 0;
	    f32 lineWidth 			= 1.0f;

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


	    bool8 enableDepth 			= true;
	    bool8 clampDepth 			= false;
	    bool8 useVertexInput		= true;
	    bool8 useSceneLayoutSet 	= true;
	    bool8 useMaterialLayoutSet  = true;
	    bool8 useModelLayoutSet 	= true;
	    bool8 useLighting			= true;
	};

	/* 
	Note(Leo): this is honest forward declaration to couple lines downward
	Note(Leo): this will not be needed in final game, where game is same
	executable instead of separate dll like during development.
	*/
	struct Functions;

	/* Note(Leo): these are defined in platform layer, and
	can (and are supposed to) be used as opaque handles in
	game layer. */
	struct Graphics;

	// Note(Leo): these are for platform layer only.
	// Todo(Leo): probably do something else.
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
		void (*update_lighting)	(Graphics*, Light const *, Camera const * camera, v3 ambient);
		void (*draw_model) 		(Graphics*, ModelHandle model, m44 transform, bool32 castShadow, m44 const * bones, u32 boneCount);
		void (*draw_line) 		(Graphics*, v3 start, v3 end, float width, float4 color);
		void (*draw_gui) 		(Graphics*, v2 position, v2 size, MaterialHandle material, float4 color);

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
				logSystem() << FILE_ADDRESS << "Unset function at '" << i << "'\n";
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

	struct StereoSoundSample
	{
		float left;
		float right;
	};

	struct SoundOutput
	{
		s32 sampleCount;
		StereoSoundSample * samples;

		StereoSoundSample * begin() { return samples; }
		StereoSoundSample * end() { return samples + sampleCount; }
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

		float elapsedTime;
	};
	
	
	struct NetworkPackage
	{
		v3 characterPosition;
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

using GameUpdateFunc = bool32(	game::Input*,
								platform::Memory*,
								game::Network*,
								platform::SoundOutput*,

								platform::Graphics*,
								platform::Window*,
								platform::Functions*,

								std::ofstream * logFile);



#define MAZEGAME_PLATFORM_HPP
#endif