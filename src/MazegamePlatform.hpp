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
		
		virtual RenderedObjectHandle 	PushRenderedObject(MeshHandle mesh, MaterialHandle material) = 0;
		virtual GuiHandle 				PushGui(MeshHandle mesh, MaterialHandle material) = 0;

		virtual void Apply() = 0;
		virtual void UnloadAll() = 0;
	};
}

namespace game
{
	struct InputButtonState
	{
		enum : int32
		{
			/* Note(Leo): These specific values have been selected so that it 
			is easy to compute value with simple addition based on input values*/
			IsUp 		= 0,
			WentDown 	= 1,
			WentUp 		= 2,
			IsDown 		= 3
		} value;

		class_member InputButtonState
		FromCurrentAndPrevious(bool32 current, bool32 previous)
		{
			/* Note(Leo): Sadly bool32 may be other values than 0 or 1.
			Alternatively we could have taken in actual bool parameters,
			but it would probably just done same conversion as here, but
			implicitly, which is not nice. Not sure about this though, but
			this is not high pressure function so it does not matter so much.*/

			bool32 current01 = current ? 1 : 0;
			bool32 previous01 = previous ? 1 : 0;
		
			InputButtonState result = { (decltype(value))(current01 + 2 * previous01) };
			return result;
		}

		bool32 IsClicked() { return value == WentDown; }
		bool32 IsReleased() { return value == WentUp; }
		bool32 IsPressed() { return (value == IsDown) || (value == WentDown); }
	};


	struct Input
	{
		Vector2 move;
		Vector2 look;


		// Todo(Leo): Do a proper separate mapping struct of meanings of specific buttons
		InputButtonState jump;
		InputButtonState confirm;

		// Note(Leo): Start and Select as in controller
		InputButtonState start;
		InputButtonState select;

		InputButtonState left;
		InputButtonState right;
		InputButtonState down;
		InputButtonState up;

		InputButtonState zoomIn;
		InputButtonState zoomOut;

		real32 elapsedTime;
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
		// Scene (camera, lights, etc)
		Matrix44 cameraView;
		Matrix44 cameraPerspective;

		// Todo(Leo): Are these cool, since they kinda are just pointers to data somewhere else....?? How to know???
		ArenaArray<Matrix44, RenderedObjectHandle> renderedObjects;
		ArenaArray<Matrix44, GuiHandle> guiObjects;
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

	struct UpdateResult
	{
		bool32 exit;
	};
}

#if MAZEGAME_INCLUDE_STD_IOSTREAM
namespace std
{
	ostream & operator << (ostream & os, game::InputButtonState buttonState)
	{
		switch (buttonState.value)
		{
			case game::InputButtonState::IsUp: 		os << "IsUp"; 		break;
			case game::InputButtonState::WentDown: 	os << "WentDown"; 	break;
			case game::InputButtonState::WentUp: 	os << "WentUp"; 	break;
			case game::InputButtonState::IsDown: 	os << "IsDown"; 	break;

			default: os << "INVALID InputButtonState VALUE";
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