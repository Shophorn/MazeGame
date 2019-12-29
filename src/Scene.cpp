/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description definitions
=============================================================================*/

enum MenuResult
{ 	
	MENU_NONE,
	MENU_EXIT,
	MENU_LOADLEVEL_2D,
	MENU_LOADLEVEL_3D,

	SCENE_CONTINUE,
	SCENE_EXIT
};

using GetAllocSizeFunc 	= uint64();

using LoadSceneFunc 	= void( void *					scenePtr,
								MemoryArena *			persistentMemory,
								MemoryArena *			transientMemory,
								game::PlatformInfo *	platform);

using UpdateSceneFunc 	= void(	void * 					scenePtr,
								game::Input * 			input,
								game::RenderInfo * 		renderer,
								game::PlatformInfo * 	platform);

struct SceneInfo
{
	// Todo(Leo): std::function allocates somewhere we don't know, should change
	GetAllocSizeFunc * 	get_alloc_size;
	LoadSceneFunc *		load;
	UpdateSceneFunc *	update;
};

internal SceneInfo
make_scene_info(GetAllocSizeFunc * getAllocSizeFunc, LoadSceneFunc * loadFunc, UpdateSceneFunc * updateFunc)
{
	SceneInfo result = 
	{
		.get_alloc_size 	= getAllocSizeFunc,
		.load 				= loadFunc,
		.update 			= updateFunc
	};
	return result;
}

using LoadGuiFunc 	= LoadSceneFunc;

using UpdateGuiFunc	= MenuResult(	void * 					guiPtr,
									game::Input * 			input,
									game::RenderInfo * 		renderer,
									game::PlatformInfo * 	platform);

struct SceneGuiInfo
{
	GetAllocSizeFunc * 	get_alloc_size;
	LoadSceneFunc * 	load;
	UpdateGuiFunc *		update;
};

internal SceneGuiInfo
make_scene_gui_info(GetAllocSizeFunc * getAllocSizeFunc, LoadGuiFunc * loadFunc, UpdateGuiFunc * updateFunc)
{
	SceneGuiInfo result = 
	{
		.get_alloc_size 	= getAllocSizeFunc,
		.load 				= loadFunc,
		.update 			= updateFunc
	};
	return result;
}

