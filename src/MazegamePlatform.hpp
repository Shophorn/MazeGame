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
	struct PipelineOptions
	{
	    bool32 enableDepth 	= true;
	    bool32 clampDepth 	= false;
	    int32 textureCount 	= 0;

	    enum { PRIMITIVE_LINE, PRIMITIVE_TRIANGLE } primitiveType = PRIMITIVE_TRIANGLE;
	};

	/* Note(Leo): This seems good, but for some reasoen I feel
	unease about using even this kind of simple inheritance */
	struct GraphicsContext;
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


	struct PlatformInfo
	{
		platform::GraphicsContext * graphicsContext;

		int32 	windowWidth;
		int32 	windowHeight;
		bool32 	windowIsFullscreen;

		// Todo(Leo): add context pointer so this too can be just a function pointer
		std::function<void(bool32)> set_window_fullscreen;
	
		MeshHandle 		(*push_mesh) 	(platform::GraphicsContext * context, MeshAsset * asset);
		TextureHandle 	(*push_texture) (platform::GraphicsContext * context, TextureAsset * asset);
		// Todo(Leo): We should use some more explicit argument than pointer to TextureAsset array
		TextureHandle 	(*push_cubemap) (platform::GraphicsContext * context, TextureAsset * asset);

		MaterialHandle 	(*push_material) 	(platform::GraphicsContext * context, MaterialAsset * asset);
		ModelHandle 	(*push_model) 		(platform::GraphicsContext * context, MeshHandle mesh, MaterialHandle material);
		PipelineHandle 	(*push_pipeline) 	(platform::GraphicsContext * context,
											char const * vertexShaderPath,
											char const * fragmentShaderPath,
											platform::PipelineOptions options);

		void 			(*unload_scene) (platform::GraphicsContext*);
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
		void (*update_camera) (platform::GraphicsContext*, Matrix44 view, Matrix44 perspective);
		void (*prepare_drawing) (platform::GraphicsContext*);
		void (*finish_drawing) (platform::GraphicsContext*);
		void (*draw) (platform::GraphicsContext*, ModelHandle, Matrix44);
		void (*draw_line) (platform::GraphicsContext*, Vector3 start, Vector3 end, float4 color);
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
	game::PlatformInfo * 	platformInfo,
	game::Network *			network,
	game::SoundOutput *		soundOutput,
	game::RenderInfo * 		outRenderInfo,
	platform::GraphicsContext * graphicsContext);

#define MAZEGAME_PLATFORM_HPP
#endif