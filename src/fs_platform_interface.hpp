/*=============================================================================
Leo Tamminen

Interface definition between Platform and Game.
===============================================================================*/

#if !defined FS_PLATFORM_INTERFACE_HPP

// Todo(Leo): Assets use these, thats why that is still here	
enum GraphicsPipeline : s64
{
	GRAPHICS_PIPELINE_NORMAL,
	GRAPHICS_PIPELINE_ANIMATED,
	GRAPHICS_PIPELINE_TRIPLANAR,
	GRAPHICS_PIPELINE_SKYBOX,
	GRAPHICS_PIPELINE_WATER,
	GRAPHICS_PIPELINE_SCREEN_GUI,
	GRAPHICS_PIPELINE_LEAVES,

	GRAPHICS_PIPELINE_COUNT
};
#include "Assets.cpp"


// Todo(Leo): Maybe post process
struct HdrSettings
{
	f32 exposure;
	f32 contrast;
};

struct ScreenRect
{
	v2 position;
	v2 size;
	v2 uvPosition;
	v2 uvSize;	
};

constexpr GuiTextureHandle GRAPHICS_RESOURCE_SHADOWMAP_GUI_TEXTURE = {-1};


/// ***********************************************************************
/// PLATFORM OPAQUE HANDLES
/* Note(Leo): these are defined in platform layer, and
can (and are supposed to) be used as opaque handles in
game layer. */

// Note(Leo): this represents graphics api, which is unknown
struct PlatformGraphics;

// Note(Leo): this represents windowing system, which is os dependent, and therefore unknown
struct PlatformWindow;
struct PlatformApiDescription;
// struct PlatformNetwork;
// struct PlatformAudio;

enum FileMode 
{
	FILE_MODE_READ,
	FILE_MODE_WRITE,
};

using PlatformFileHandle = void*;

/// ***********************************************************************
/// PLATFORM VALUE TYPES

struct PlatformNetworkPackage
{
	v3 			characterPosition;
	quaternion 	characterRotation;
};

constexpr s32 NETWORK_PACKAGE_SIZE = sizeof(PlatformNetworkPackage);
static_assert(NETWORK_PACKAGE_SIZE <= 512, "Time to deal with bigger network packages");

enum PlatformGraphicsFrameResult
{
	PGFR_FRAME_OK,
	PGFR_FRAME_RECREATE,
	PGFR_FRAME_BAD_PROBLEM,
};

struct PlatformStereoSoundSample
{
	float left;
	float right;
};

// Todo(Leo): These buttonstate things are implementation details and may even vary per platform,
// so consider moving them elsewhere. Maybe only expose is_clicked etc. as
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

internal ButtonState update_button_state(ButtonState buttonState, bool32 buttonIsPressed)
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
			AssertMsg(false, "Invalid ButtonState value");
	}

	return buttonState;
}

struct PlatformTimePoint
{
	s64 value;
};

/// ***********************************************************************
/// PLATFORM POINTER TYPES

// Todo(Leo): input is done rarely, this should be similar opaque handle like pointer like windows and graphics
struct PlatformInput
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

	v2 			mousePosition;
	f32 		mouseZoom;
	ButtonState mouse0;
};

struct PlatformTime
{
	f32 elapsedTime;
};

struct PlatformMemory
{
	void * 	memory;
	u64 	size;
};

struct PlatformNetwork
{
	bool32 isConnected;

	PlatformNetworkPackage inPackage;
	PlatformNetworkPackage outPackage;
};

struct PlatformSoundOutput
{
	s32 sampleCount;
	PlatformStereoSoundSample * samples;

	PlatformStereoSoundSample * begin() { return samples; }
	PlatformStereoSoundSample * end() { return samples + sampleCount; }
};

/// ----------------- GAME TO PLATFORM API -----------------------------

#if defined FS_PLATFORM
	#if defined FS_GAME_DLL
		#error "Both FS_PLATFORM and FS_GAME_DLL are defined!"
	#endif

	#define FS_PLATFORM_API(func) func
	#define FS_PLATFORM_API_TYPE(func) decltype(func)*
	#define FS_PLATFORM_API_SET_FUNCTION(func, ptr) ptr = func

	#define FS_GAME_API

#elif defined FS_GAME_DLL

	#define FS_PLATFORM_API(func) (*func)
	#define FS_PLATFORM_API_TYPE(func) decltype(func)
	#define FS_PLATFORM_API_SET_FUNCTION(func, ptr) func = ptr

	#define FS_GAME_API extern "C" __declspec(dllexport)

#else
	#error "FS_PLATFORM or FS_GAME_DLL must be defined!"
#endif







// #if defined FS_GAME_DLL 
// extern "C" __declspec(dllexport)
// #endif
FS_GAME_API bool32 update_game(	const PlatformInput *,
								const PlatformTime *,
								PlatformMemory *,
								PlatformNetwork *,
								PlatformSoundOutput*,
								PlatformGraphics *,
								PlatformWindow *,
								PlatformApiDescription *,
								bool32 reInitializeGlobalVariables);
using UpdateGameFunc = decltype(update_game);

/// ------------------- PLATFORM TO GAME API ----------------------------

PlatformFileHandle FS_PLATFORM_API(platform_file_open) (char const * filename, FileMode);
void FS_PLATFORM_API(platform_file_close) (PlatformFileHandle);
s64 FS_PLATFORM_API(platform_file_get_size) (PlatformFileHandle);
void FS_PLATFORM_API(platform_file_read) (PlatformFileHandle, s64 position, s64 count, void * memory);
void FS_PLATFORM_API(platform_file_write) (PlatformFileHandle, s64 position, s64 count, void * memory);

void FS_PLATFORM_API(platform_log_write) (s32 count, char const * buffer);

PlatformTimePoint FS_PLATFORM_API(platform_time_now) ();
f64 FS_PLATFORM_API(platform_time_elapsed_seconds)(PlatformTimePoint start, PlatformTimePoint end);

u32 FS_PLATFORM_API(platform_window_get_width) (PlatformWindow const *);
u32 FS_PLATFORM_API(platform_window_get_height) (PlatformWindow const *);
bool32 FS_PLATFORM_API(platform_window_is_fullscreen) (PlatformWindow const *);
void FS_PLATFORM_API(platform_window_set_fullscreen) (PlatformWindow*, bool32 value);
void FS_PLATFORM_API(platform_window_set_cursor_visible) (PlatformWindow*, bool32 value);

void FS_PLATFORM_API(graphics_drawing_prepare_frame)(PlatformGraphics*);
void FS_PLATFORM_API(graphics_drawing_finish_frame)(PlatformGraphics*);

void FS_PLATFORM_API(graphics_drawing_update_camera)(PlatformGraphics*, Camera const *);
void FS_PLATFORM_API(graphics_drawing_update_lighting)(PlatformGraphics*, Light const *, Camera const * camera, v3 ambient);
void FS_PLATFORM_API(graphics_drawing_update_hdr_settings)(PlatformGraphics*, HdrSettings const *);

void FS_PLATFORM_API(graphics_draw_model) (PlatformGraphics*, ModelHandle model, m44 transform, bool32 castShadow, m44 const * bones, u32 boneCount);
void FS_PLATFORM_API(graphics_draw_meshes) (PlatformGraphics*, s32 count, m44 const * transforms, MeshHandle mesh, MaterialHandle material);
void FS_PLATFORM_API(graphics_draw_screen_rects) (PlatformGraphics*, s32 count, ScreenRect const * rects, GuiTextureHandle texture, v4 color);
void FS_PLATFORM_API(graphics_draw_lines) (PlatformGraphics*, s32 pointCount, v3 const * points, v4 color);
void FS_PLATFORM_API(graphics_draw_procedural_mesh)(	PlatformGraphics*,
								s32 vertexCount, Vertex const * vertices,
								s32 indexCount, u16 const * indices,
								m44 transform, MaterialHandle material);
void FS_PLATFORM_API(graphics_draw_leaves) (PlatformGraphics*, s32 count, m44 const * transforms, s32 colourIndex, v3 colour, MaterialHandle material);

MeshHandle FS_PLATFORM_API(graphics_memory_push_mesh) (PlatformGraphics*, MeshAssetData * asset);
TextureHandle FS_PLATFORM_API(graphics_memory_push_texture) (PlatformGraphics*, TextureAssetData * asset);
MaterialHandle FS_PLATFORM_API(graphics_memory_push_material) (PlatformGraphics*, GraphicsPipeline, s32 textureCount, TextureHandle * textures);
GuiTextureHandle FS_PLATFORM_API(graphics_memory_push_gui_texture) (PlatformGraphics*, TextureAssetData * asset);
	// Todo(Leo): Maybe remove 'push_model', we can render also just passing mesh and material handles directly
ModelHandle FS_PLATFORM_API(graphics_memory_push_model) (PlatformGraphics*, MeshHandle mesh, MaterialHandle material);
void FS_PLATFORM_API(graphics_memory_unload) (PlatformGraphics*);

// Note(Leo): this may be needed for development only, should it be handled differently
void FS_PLATFORM_API(graphics_development_update_texture) (PlatformGraphics*, TextureHandle, TextureAssetData*);
void FS_PLATFORM_API(graphics_development_reload_shaders) (PlatformGraphics*);


// Todo(Leo): maybe add condition if not FS_FULL_GAME
struct PlatformApiDescription
{
	FS_PLATFORM_API_TYPE(platform_file_open) fileOpen;
	FS_PLATFORM_API_TYPE(platform_file_close) fileClose;
	FS_PLATFORM_API_TYPE(platform_file_get_size) fileSize;
	FS_PLATFORM_API_TYPE(platform_file_read) fileRead;
	FS_PLATFORM_API_TYPE(platform_file_write) fileWrite;

	FS_PLATFORM_API_TYPE(platform_log_write) logWrite;

	FS_PLATFORM_API_TYPE(platform_time_now) timeNow;
	FS_PLATFORM_API_TYPE(platform_time_elapsed_seconds) timeElapsedSeconds;

	FS_PLATFORM_API_TYPE(platform_window_get_width) windowGetWidth;
	FS_PLATFORM_API_TYPE(platform_window_get_height) windowGetHeight;
	FS_PLATFORM_API_TYPE(platform_window_is_fullscreen) windowIsFullscreen;
	FS_PLATFORM_API_TYPE(platform_window_set_fullscreen) windowSetFullscreen;
	FS_PLATFORM_API_TYPE(platform_window_set_cursor_visible) windowSetCursorVisible;

	FS_PLATFORM_API_TYPE(graphics_drawing_prepare_frame) drawingPrepareFrame;
	FS_PLATFORM_API_TYPE(graphics_drawing_finish_frame) drawingFinishFrame;
	FS_PLATFORM_API_TYPE(graphics_drawing_update_camera) drawingUpdateCamera;
	FS_PLATFORM_API_TYPE(graphics_drawing_update_lighting) drawingUpdateLighting;
	FS_PLATFORM_API_TYPE(graphics_drawing_update_hdr_settings) drawingUpdateHdrSettings;

	FS_PLATFORM_API_TYPE(graphics_draw_model) drawModel;
	FS_PLATFORM_API_TYPE(graphics_draw_meshes) drawMeshes;
	FS_PLATFORM_API_TYPE(graphics_draw_screen_rects) drawScreenRects;
	FS_PLATFORM_API_TYPE(graphics_draw_lines) drawLines;
	FS_PLATFORM_API_TYPE(graphics_draw_procedural_mesh) drawProceduralMesh;
	FS_PLATFORM_API_TYPE(graphics_draw_leaves) drawLeaves;

	FS_PLATFORM_API_TYPE(graphics_memory_push_mesh) memoryPushMesh;
	FS_PLATFORM_API_TYPE(graphics_memory_push_texture) memoryPushTexture;
	FS_PLATFORM_API_TYPE(graphics_memory_push_material) memoryPushMaterial;
	FS_PLATFORM_API_TYPE(graphics_memory_push_gui_texture) memoryPushGuiTexture;
	FS_PLATFORM_API_TYPE(graphics_memory_push_model) memoryPushModel;
	FS_PLATFORM_API_TYPE(graphics_memory_unload) memoryUnload;

	FS_PLATFORM_API_TYPE(graphics_development_update_texture) developmentUpdateTexture;
	FS_PLATFORM_API_TYPE(graphics_development_reload_shaders) developmentReloadShaders;
};

void platform_set_api(PlatformApiDescription * api)
{
	FS_PLATFORM_API_SET_FUNCTION(platform_file_open, api->fileOpen);
	FS_PLATFORM_API_SET_FUNCTION(platform_file_close, api->fileClose);
	FS_PLATFORM_API_SET_FUNCTION(platform_file_get_size, api->fileSize);
	FS_PLATFORM_API_SET_FUNCTION(platform_file_read, api->fileRead);
	FS_PLATFORM_API_SET_FUNCTION(platform_file_write, api->fileWrite);

	FS_PLATFORM_API_SET_FUNCTION(platform_log_write, api->logWrite);

	FS_PLATFORM_API_SET_FUNCTION(platform_time_now, api->timeNow);
	FS_PLATFORM_API_SET_FUNCTION(platform_time_elapsed_seconds, api->timeElapsedSeconds);

	FS_PLATFORM_API_SET_FUNCTION(platform_window_get_width, api->windowGetWidth);
	FS_PLATFORM_API_SET_FUNCTION(platform_window_get_height, api->windowGetHeight);
	FS_PLATFORM_API_SET_FUNCTION(platform_window_is_fullscreen, api->windowIsFullscreen);
	FS_PLATFORM_API_SET_FUNCTION(platform_window_set_fullscreen, api->windowSetFullscreen);
	FS_PLATFORM_API_SET_FUNCTION(platform_window_set_cursor_visible, api->windowSetCursorVisible);

	FS_PLATFORM_API_SET_FUNCTION(graphics_drawing_prepare_frame, api->drawingPrepareFrame);
	FS_PLATFORM_API_SET_FUNCTION(graphics_drawing_finish_frame, api->drawingFinishFrame);
	FS_PLATFORM_API_SET_FUNCTION(graphics_drawing_update_camera, api->drawingUpdateCamera);
	FS_PLATFORM_API_SET_FUNCTION(graphics_drawing_update_lighting, api->drawingUpdateLighting);
	FS_PLATFORM_API_SET_FUNCTION(graphics_drawing_update_hdr_settings, api->drawingUpdateHdrSettings);

	FS_PLATFORM_API_SET_FUNCTION(graphics_draw_model, api->drawModel);
	FS_PLATFORM_API_SET_FUNCTION(graphics_draw_meshes, api->drawMeshes);
	FS_PLATFORM_API_SET_FUNCTION(graphics_draw_screen_rects, api->drawScreenRects);
	FS_PLATFORM_API_SET_FUNCTION(graphics_draw_lines, api->drawLines);
	FS_PLATFORM_API_SET_FUNCTION(graphics_draw_procedural_mesh, api->drawProceduralMesh);
	FS_PLATFORM_API_SET_FUNCTION(graphics_draw_leaves, api->drawLeaves);

	FS_PLATFORM_API_SET_FUNCTION(graphics_memory_push_mesh, api->memoryPushMesh);
	FS_PLATFORM_API_SET_FUNCTION(graphics_memory_push_texture, api->memoryPushTexture);
	FS_PLATFORM_API_SET_FUNCTION(graphics_memory_push_material, api->memoryPushMaterial);
	FS_PLATFORM_API_SET_FUNCTION(graphics_memory_push_gui_texture, api->memoryPushGuiTexture);
	FS_PLATFORM_API_SET_FUNCTION(graphics_memory_push_model, api->memoryPushModel);
	FS_PLATFORM_API_SET_FUNCTION(graphics_memory_unload, api->memoryUnload);

	FS_PLATFORM_API_SET_FUNCTION(graphics_development_update_texture, api->developmentUpdateTexture);
	FS_PLATFORM_API_SET_FUNCTION(graphics_development_reload_shaders, api->developmentReloadShaders);
}

#define FS_PLATFORM_INTERFACE_HPP
#endif