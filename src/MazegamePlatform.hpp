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
	/* Note(Leo): these are defined in platform layer, and
	can (and are supposed to) be used as opaque handles in
	game layer*/
	struct Graphics;
	struct Platform;

	struct RenderingOptions
	{
	    bool32 	enableDepth 		= true;
	    bool32 	clampDepth 			= false;
	    int32 	textureCount 		= 0;
	    uint32	pushConstantSize 	= 0;

	    enum { PRIMITIVE_LINE, PRIMITIVE_TRIANGLE } primitiveType = PRIMITIVE_TRIANGLE;
	};
}

namespace game
{
	enum struct ButtonState : int8
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
		Vector2 move;
		Vector2 look;


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


	struct PlatformFunctions
	{
		// Todo(Leo): add context pointer so this too can be just a function pointer
		std::function<void(bool32)> set_window_fullscreen;
		
		// void (*set_window_fullscreen) (platform::Platform*, bool32 fulls
		int32 (*get_window_width)(platform::Platform*);
		int32 (*get_window_height) (platform::Platform*);
		bool32 (*is_window_fullscreen) (platform::Platform*);


		MeshHandle 		(*push_mesh) 	(platform::Graphics * context, MeshAsset * asset);
		TextureHandle 	(*push_texture) (platform::Graphics * context, TextureAsset * asset);
		// Todo(Leo): We should use some more explicit argument than pointer to TextureAsset array
		TextureHandle 	(*push_cubemap) (platform::Graphics * context, TextureAsset * asset);

		MaterialHandle 	(*push_material) 	(platform::Graphics * context, MaterialAsset * asset);
		MaterialHandle 	(*push_gui_material)(platform::Graphics * context, TextureHandle texture);

		ModelHandle 	(*push_model) 		(platform::Graphics * context, MeshHandle mesh, MaterialHandle material);
		PipelineHandle 	(*push_pipeline) 	(platform::Graphics * context,
											char const * vertexShaderPath,
											char const * fragmentShaderPath,
											platform::RenderingOptions options);

		void 			(*unload_scene) (platform::Graphics*);
	};
	
	struct Memory
	{
		bool32 isInitialized;

		void * persistentMemory;
		uint64 persistentMemorySize;

		void * transientMemory;
		uint64 transientMemorySize;
	};

	struct RenderInfo
	{
		void (*update_camera) 	(platform::Graphics*, Matrix44 view, Matrix44 perspective);
		void (*prepare_drawing) (platform::Graphics*);
		void (*finish_drawing) 	(platform::Graphics*);
		void (*draw) 			(platform::Graphics*, ModelHandle, Matrix44);
		void (*draw_line) 		(platform::Graphics*, Vector3 start, Vector3 end, float4 color);
		void (*draw_gui) 		(platform::Graphics*, Vector2 position, Vector2 size, MaterialHandle material, float4 color);
	};
	
	struct NetworkPackage
	{
		Vector3 characterPosition;
		Quaternion characterRotation;
	};

	constexpr int32 NETWORK_PACKAGE_SIZE = sizeof(NetworkPackage);
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
		int32 sampleCount;
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
	game::PlatformFunctions * 	PlatformFunctions,
	game::Network *			network,
	game::SoundOutput *		soundOutput,
	game::RenderInfo * 		outRenderInfo,

	// Are these understandable enough?
	platform::Graphics*,
	platform::Platform*);

#define MAZEGAME_PLATFORM_HPP
#endif