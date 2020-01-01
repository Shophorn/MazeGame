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
	void PrintAssert(const char * file, int line, const char * message, const char * expression)
	{
		std::cout
			<< "Assertion failed [" << file << ":" << line << "]: \""
			<< message << "\", expression (" << expression << ")\n";
	}
	#endif

	#define MAZEGAME_ASSERT(expr, msg) if (!(expr)) { PrintAssert(__FILE__, __LINE__, msg, #expr); abort(); }
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
	    bool32 enableDepth = true;
	};

	/* Note(Leo): This seems good, but for some reasoen I feel
	unease about using even this kind of simple inheritance */
	struct IGraphicsContext
	{
		virtual MeshHandle 		PushMesh(MeshAsset * mesh) 					= 0;
		virtual TextureHandle 	PushTexture (TextureAsset * asset) 			= 0;
		virtual MaterialHandle 	PushMaterial (MaterialAsset * material) 	= 0;
		
		virtual RenderedObjectHandle 	PushRenderedObject(MeshHandle mesh, MaterialHandle material) = 0;
		virtual GuiHandle 				PushGui(MeshHandle mesh, MaterialHandle material) = 0;

		virtual PipelineHandle push_pipeline(const char * vertexShaderPath, const char * fragmentShaderPath, PipelineOptions options = {}) = 0;

		virtual void Apply() = 0;
		virtual void UnloadAll() = 0;
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
	update_button_state(ButtonState current, bool32 newStateIsDown)
	{
		switch(current)
		{
			case ButtonState::IsUp:
				return 	newStateIsDown ? 
						ButtonState{ButtonState::WentDown} :
						ButtonState{ButtonState::IsUp};

			case ButtonState::WentDown:
				return 	newStateIsDown ?
						ButtonState{ButtonState::IsDown} :
						ButtonState{ButtonState::WentUp};

			case ButtonState::WentUp:
				return 	newStateIsDown ?
						ButtonState{ButtonState::WentDown} :
						ButtonState{ButtonState::IsUp};

			case ButtonState::IsDown:
				return 	newStateIsDown ?
						ButtonState{ButtonState::IsDown} :
						ButtonState{ButtonState::WentUp};

			default:
				MAZEGAME_ASSERT(false, "Invalid ButtonState value");
		}
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
		platform::IGraphicsContext * graphicsContext;

		int32 	windowWidth;
		int32 	windowHeight;
		bool32 	windowIsFullscreen;
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
		std::function<void(Matrix44 view, Matrix44 perspective)> set_camera;
		std::function<void(RenderedObjectHandle, Matrix44)> render;
		std::function<void(GuiHandle, Matrix44)> render_gui;
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

	struct UpdateResult
	{
		bool32 exit;
		enum { SET_WINDOW_NONE, SET_WINDOW_FULLSCREEN, SET_WINDOW_WINDOWED } setWindow;
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

// TODO(Leo): can we put this into namespace?
extern "C" game::UpdateResult
GameUpdate(
	game::Input * 			inpM,
	game::Memory * 			memory,
	game::PlatformInfo * 	platformInfo,
	game::Network *			network,
	game::SoundOutput *		soundOutput,
	game::RenderInfo * 		outRenderInfo);

#define MAZEGAME_PLATFORM_HPP
#endif