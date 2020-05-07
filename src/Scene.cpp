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