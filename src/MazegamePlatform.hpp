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

	#define DEBUG_ASSERT(expr, msg) if (!(expr)) { PrintAssert(__FILE__, __LINE__, msg, #expr); abort(); }

	// Note(Leo): Some things need to asserted in production too, this is a reminder for those only.
	#define PRODUCTION_ASSERT DEBUG_ASSERT


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
	struct Window;
	struct Network;
	struct Audio;

	struct Functions
	{
		// GRAPHICS FUNCTIONS
		MeshHandle (*push_mesh) (Graphics*, MeshAsset * asset);
		TextureHandle (*push_texture) (Graphics*, TextureAsset * asset);
		TextureHandle (*push_cubemap) (Graphics*, StaticArray<TextureAsset, 6> * asset);

		MaterialHandle (*push_material) (Graphics*, MaterialAsset * asset);
		MaterialHandle (*push_gui_material) (Graphics*, TextureHandle texture);

		// Todo(Leo): Remove 'push_model', we can render also just passing mesh and material handles directly
		ModelHandle (*push_model) (Graphics*, MeshHandle mesh, MaterialHandle material);
		PipelineHandle (*push_pipeline) (Graphics*, char const * vertexShaderFilename,
													char const * fragmentShaderFilename,
													RenderingOptions optios);
		void (*unload_scene) (Graphics*);

		void (*update_camera) (Graphics*, Matrix44 view, Matrix44 perspective);
		void (*prepare_frame) (Graphics*);
		void (*finish_frame) (Graphics*);

		void (*draw_model) (Graphics*, ModelHandle model, Matrix44 transform);
		void (*draw_line) (Graphics*, float3 start, float3 end, float4 color);
		void (*draw_gui) (Graphics*, float2 position, float2 size, MaterialHandle material, float4 color);

		// WINDOW FUNCTIONS	
		u32 (*get_window_width) (Window const *);
		u32 (*get_window_height) (Window const *);
		bool32 (*is_window_fullscreen) (Window const *);
		void (*set_window_fullscreen) (Window*, bool32 value);
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

		float elapsedTime;
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
update_game(
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