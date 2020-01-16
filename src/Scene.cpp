/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description definitions.

Note(Leo): Scenes will eventually be loaded runtime from textfile or a database
like structure. That is why 'SceneInfo' is not a template, as there will be no
type for template parameter.
=============================================================================*/

// Todo (Leo): this is subpar construct, do something else
enum MenuResult
{ 	
	MENU_NONE,
	MENU_EXIT,
	MENU_LOADLEVEL_2D,
	MENU_LOADLEVEL_3D,

	SCENE_CONTINUE,
	SCENE_EXIT
};

struct SceneInfo
{
	uint64 		(*get_alloc_size)();

	void 		(*load)( 	void *						scenePtr,
							MemoryArena *				persistentMemory,
							MemoryArena *				transientMemory,
							game::PlatformFunctions *	functions,
							platform::Graphics*,
							platform::Platform*);

	MenuResult 	(*update)(	void * 						guiPtr,
							game::Input * 				input,
							game::RenderInfo * 			renderer,
							game::PlatformFunctions * 	functions,
							platform::Graphics*,
							platform::Platform*);
};

internal SceneInfo
make_scene_info(decltype(SceneInfo::get_alloc_size) getAllocSizeFunc,
				decltype(SceneInfo::load) 			loadFunc,
				decltype(SceneInfo::update)			updateFunc)
{
	SceneInfo result = 
	{
		.get_alloc_size 	= getAllocSizeFunc,
		.load 				= loadFunc,
		.update 			= updateFunc
	};
	return result;
}
