/*=============================================================================
Leo Tamminen

Interface definition between Platform and Game.
===============================================================================*/

#if !defined MAZEGAME_PLATFORM_HPP

#define MAZEGAME_INCLUDE_STD_IOSTREAM 1
#include <iostream>


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
#include "Matrices.cpp"
#include "Quaternion.cpp"

#include "Memory.cpp"
#include "Assets.cpp"

namespace platform
{
	/* Note(Leo): This seems good, but for some reasoen I feel
	unease about using even this kind of simple inheritance */
	struct IGraphicsContext
	{
		virtual MeshHandle 		PushMesh(MeshAsset * mesh) 					= 0;
		virtual TextureHandle 	PushTexture (TextureAsset * asset) 			= 0;
		virtual MaterialHandle 	PushMaterial (MaterialAsset * material) 	= 0;
		
		virtual RenderedObjectHandle PushRenderedObject(MeshHandle mesh, MaterialHandle material) = 0;

		virtual void Apply() = 0;
		virtual void UnloadAll() = 0;
	};
}

namespace game
{
	enum struct InputButtonState
	{
		/* Note(Leo): These specific values have been selected so that it 
		is easy to compute value with simple addition based on input values*/
		IsUp 		= 0,
		WentDown 	= 1,
		WentUp 		= 2,
		IsDown 		= 3
	
	};

	bool32 IsPressed(InputButtonState button)
	{
		bool32 result = (button == InputButtonState::IsDown) || (button == InputButtonState::WentDown);
		return result;
	}

	struct Input
	{
		Vector2 move;
		Vector2 look;

		InputButtonState jump;

		// Note(Leo): Start and Select as in controller
		InputButtonState start;
		InputButtonState select;

		bool32 zoomIn;
		bool32 zoomOut;

		real32 timeDelta;
	};

	struct PlatformInfo
	{
		platform::IGraphicsContext * graphicsContext;

		int32 windowWidth;
		int32 windowHeight;
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
		/* Todo(Leo): Make a proper container for these
		It should be such that items are stored as per vulkan physical device's
		min uniformbuffer alignment, so we can just map the whole container at
		once to gpu memory. */

		// Scene (camera, lights, etc)
		Matrix44 cameraView;
		Matrix44 cameraPerspective;

		// Models
		uint32 modelMatrixCount;
		Matrix44 * modelMatrixArray;
	};
	
	struct NetworkPackage
	{
		Vector3 characterPosition;
		Quaternion characterRotation;
	};

	constexpr int32 NETWORK_PACKAGE_SIZE = sizeof(NetworkPackage);
	static_assert(NETWORK_PACKAGE_SIZE <= 512, "Time to deak with bigger network packages");

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
	ostream & operator << (ostream & os, game::InputButtonState buttonState)
	{
		switch (buttonState)
		{
			case game::InputButtonState::IsUp: 		os << "InputButtonState::IsUp"; 		break;
			case game::InputButtonState::WentDown: 	os << "InputButtonState::WentDown"; 	break;
			case game::InputButtonState::WentUp: 		os << "InputButtonState::WentUp"; 		break;
			case game::InputButtonState::IsDown: 		os << "InputButtonState::IsDown"; 		break;

			default: os << "INVALID InputButtonState VALUE";
		}

		return os;
	}
}

#endif


extern "C" void
GameUpdate(
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platformInfo,
	game::Network *			network,
	game::SoundOutput *		soundOutput,
	game::RenderInfo * 		outRenderInfo);

#define MAZEGAME_PLATFORM_HPP
#endif