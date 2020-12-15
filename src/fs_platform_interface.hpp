/*
Leo Tamminen

Interface definition between Platform and Game.
*/

#if !defined FS_PLATFORM_INTERFACE_HPP

// Todo(Leo): should this be header or should they just be here
#include "platform_assets.cpp"

// Todo(Leo): these are actually platform assets as well
#include "Camera.cpp"
#include "Light.cpp"

enum GraphicsPipeline : s32
{
	GraphicsPipeline_normal,
	GraphicsPipeline_animated,
	GraphicsPipeline_triplanar,
	GraphicsPipeline_skybox,
	GraphicsPipeline_water,
	GraphicsPipeline_screen_gui,
	GraphicsPipeline_leaves,

	GraphicsPipelineCount
};

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

struct StereoSoundSample
{
	f32 left;
	f32 right;
};

struct StereoSoundOutput
{
	s32 sampleCount;
	StereoSoundSample * samples;
};

constexpr MaterialHandle GRAPHICS_RESOURCE_SHADOWMAP_GUI_MATERIAL = {-1};

/// ***********************************************************************
/// PLATFORM OPAQUE HANDLES
/* Note(Leo): these are defined in platform layer, and
can (and are supposed to) be used as opaque handles in
game layer. */

struct PlatformGraphics;
struct PlatformWindow;
struct PlatformInput;
struct PlatformAudio;
// struct PlatformNetwork;

struct PlatformApiDescription;

enum FileMode 
{
	FileMode_read,
	FileMode_write,
};

using PlatformFileHandle = void*;


/// ***********************************************************************
/// PLATFORM ENUMERATIONS

enum InputButton : s32
{
	// Note(Leo): invalid is "default" button, and it can be written values to,
	// but reading it will not result in anything sensible.
	InputButton_invalid,

	InputButton_xbox_y,
	InputButton_xbox_x,
	InputButton_xbox_b,
	InputButton_xbox_a, 
	InputButton_start,
	InputButton_select,
	InputButton_dpad_left,
	InputButton_dpad_right,
	InputButton_dpad_down,
	InputButton_dpad_up,
	InputButton_zoom_in,
	InputButton_zoom_out,

	InputButton_mouse_0,
	InputButton_mouse_1,

	InputButton_keyboard_enter,
	InputButton_keyboard_escape,
	InputButton_keyboard_backspace,
	InputButton_keyboard_space,

	InputButton_keyboard_left,
	InputButton_keyboard_right,
	InputButton_keyboard_down,
	InputButton_keyboard_up,

	InputButton_keyboard_f1,
	InputButton_keyboard_f2,
	InputButton_keyboard_f3,
	InputButton_keyboard_f4,

	InputButton_keyboard_w,
	InputButton_keyboard_a,
	InputButton_keyboard_s,
	InputButton_keyboard_d,

	InputButton_keyboard_1,
	InputButton_keyboard_2,
	InputButton_keyboard_3,
	InputButton_keyboard_4,
	InputButton_keyboard_5,
	InputButton_keyboard_6,
	InputButton_keyboard_7,
	InputButton_keyboard_8,
	InputButton_keyboard_9,
	InputButton_keyboard_0,

	InputButton_keyboard_left_alt,
	InputButton_keyboard_ctrl,

	InputButtonCount,

	// Note(Leo): these are merely different mappings, that may be more useful in certain contexts
	InputButton_wasd_up 	= InputButton_keyboard_w,
	InputButton_wasd_down 	= InputButton_keyboard_s,
	InputButton_wasd_right 	= InputButton_keyboard_d,
	InputButton_wasd_left 	= InputButton_keyboard_a,

	InputButton_nintendo_x 	= InputButton_xbox_y,
	InputButton_nintendo_y 	= InputButton_xbox_x,
	InputButton_nintendo_a 	= InputButton_xbox_b,
	InputButton_nintendo_b 	= InputButton_xbox_a, 

	InputButton_ps4_triangle 	= InputButton_xbox_y,
	InputButton_ps4_square 		= InputButton_xbox_x,
	InputButton_ps4_circle 		= InputButton_xbox_b,
	InputButton_ps4_cross 		= InputButton_xbox_a,
};

enum InputAxis : s32
{
	InputAxis_move_x,
	InputAxis_move_y,
	InputAxis_look_x,
	InputAxis_look_y,

	InputAxis_mouse_move_x,
	InputAxis_mouse_move_y,
	InputAxis_mouse_scroll,

	InputAxisCount
};

enum InputDevice : s32
{
	InputDevice_gamepad,
	InputDevice_mouse_and_keyboard,
};

enum PlatformWindowSetting
{
	PlatformWindowSetting_cursor_hidden_and_locked,
	PlatformWindowSetting_fullscreen,
};


/// ----------------- GAME TO PLATFORM API -----------------------------

#if defined FS_PLATFORM
	#if defined FS_GAME_DLL
		#error "Both FS_PLATFORM and FS_GAME_DLL are defined!"
	#endif

	#define FS_PLATFORM_API(name) name
	#define FS_PLATFORM_FUNC_PTR(func) decltype(func)*
	#define FS_PLATFORM_API_SET_FUNCTION(func, ptr) ptr = func

	#define FS_GAME_API static

#elif defined FS_GAME_DLL

	#define FS_PLATFORM_API(name) (*name)
	#define FS_PLATFORM_FUNC_PTR(func) decltype(func)
	#define FS_PLATFORM_API_SET_FUNCTION(func, ptr) func = ptr

	#define FS_GAME_API extern "C" __declspec(dllexport)

#else
	#error "FS_PLATFORM or FS_GAME_DLL must be defined!"
#endif

FS_GAME_API bool32 game_update( MemoryBlock,
								PlatformInput *,
								PlatformGraphics *,
								PlatformWindow *,
								PlatformAudio*,
								f32 elapsedSeconds);
using GameUpdateFunc = decltype(game_update);

/// ------------------- PLATFORM TO GAME API ----------------------------

#pragma region PLATFORM API

static PlatformFileHandle 	FS_PLATFORM_API(platform_file_open) (char const * filename, FileMode);
static void 				FS_PLATFORM_API(platform_file_close) (PlatformFileHandle);
static s64 					FS_PLATFORM_API(platform_file_get_size) (PlatformFileHandle);
static void 				FS_PLATFORM_API(platform_file_read) (PlatformFileHandle, s64 position, s64 count, void * memory);
static void 				FS_PLATFORM_API(platform_file_write) (PlatformFileHandle, s64 position, s64 count, void * memory);

static void 				FS_PLATFORM_API(platform_log_write) (s32 count, char const * buffer);

static s64 					FS_PLATFORM_API(platform_time_now) ();
static f64 					FS_PLATFORM_API(platform_time_elapsed_seconds)(s64 start, s64 end);

static u32 					FS_PLATFORM_API(platform_window_get_width) (PlatformWindow const *);
static u32 					FS_PLATFORM_API(platform_window_get_height) (PlatformWindow const *);

static void 				FS_PLATFORM_API(platform_window_set)(PlatformWindow*, PlatformWindowSetting, bool32);
static bool32 				FS_PLATFORM_API(platform_window_get)(PlatformWindow*, PlatformWindowSetting);

static bool32 				FS_PLATFORM_API(input_button_went_down) (PlatformInput*, InputButton);
static bool32 				FS_PLATFORM_API(input_button_is_down) (PlatformInput*, InputButton);
static bool32 				FS_PLATFORM_API(input_button_went_up) (PlatformInput*, InputButton);
static f32 					FS_PLATFORM_API(input_axis_get_value) (PlatformInput * input, InputAxis axis);
static v2 					FS_PLATFORM_API(input_cursor_get_position) (PlatformInput * input);
static bool32 				FS_PLATFORM_API(input_is_device_used) (PlatformInput *, InputDevice);

static void 				FS_PLATFORM_API(graphics_drawing_update_camera)(PlatformGraphics*, Camera const *);
static void 				FS_PLATFORM_API(graphics_drawing_update_lighting)(PlatformGraphics*, Light const *, Camera const * camera, v3 ambient);
static void 				FS_PLATFORM_API(graphics_drawing_update_hdr_settings)(PlatformGraphics*, HdrSettings const *);

static void 				FS_PLATFORM_API(graphics_draw_model) (PlatformGraphics*, ModelHandle model, m44 transform, bool32 castShadow, m44 const * bones, u32 boneCount);
static void 				FS_PLATFORM_API(graphics_draw_meshes) (PlatformGraphics*, s32 count, m44 const * transforms, MeshHandle mesh, MaterialHandle material);
static void 				FS_PLATFORM_API(graphics_draw_screen_rects) (PlatformGraphics*, s32 count, ScreenRect const * rects, MaterialHandle material, v4 color);
static void 				FS_PLATFORM_API(graphics_draw_lines) (PlatformGraphics*, s32 pointCount, v3 const * points, v4 color);
static void 				FS_PLATFORM_API(graphics_draw_procedural_mesh)(	PlatformGraphics*,
																	s32 vertexCount, Vertex const * vertices,
																	s32 indexCount, u16 const * indices,
																	m44 transform, MaterialHandle material);
static void 				FS_PLATFORM_API(graphics_draw_leaves) (PlatformGraphics*, s32 count, m44 const * transforms, s32 colourIndex, v3 colour, MaterialHandle material);

static MeshHandle 			FS_PLATFORM_API(graphics_memory_push_mesh) (PlatformGraphics*, MeshAssetData * asset);
static TextureHandle 		FS_PLATFORM_API(graphics_memory_push_texture) (PlatformGraphics*, TextureAssetData * asset);
static MaterialHandle 		FS_PLATFORM_API(graphics_memory_push_material) (PlatformGraphics*, GraphicsPipeline, s32 textureCount, TextureHandle * textures);
// Todo(Leo): Maybe remove 'push_model', we can render also just passing mesh and material handles directly
static ModelHandle 			FS_PLATFORM_API(graphics_memory_push_model) (PlatformGraphics*, MeshHandle mesh, MaterialHandle material);
static void 				FS_PLATFORM_API(graphics_memory_unload) (PlatformGraphics*);

// Todo(Leo): this may be needed for development only, should it be handled differently???
static void 				FS_PLATFORM_API(graphics_development_update_texture) (PlatformGraphics*, TextureHandle, TextureAssetData*);
static void 				FS_PLATFORM_API(graphics_development_reload_shaders) (PlatformGraphics*);

static StereoSoundOutput 	FS_PLATFORM_API(audio_get_output_buffer) (PlatformAudio*);
static void 				FS_PLATFORM_API(audio_release_output_buffer) (PlatformAudio*, StereoSoundOutput);

#pragma endregion PLATFORM API

// Todo(Leo): maybe add condition if not FS_RELEASE
struct PlatformApiDescription
{
	FS_PLATFORM_FUNC_PTR(platform_file_open) fileOpen;
	FS_PLATFORM_FUNC_PTR(platform_file_close) fileClose;
	FS_PLATFORM_FUNC_PTR(platform_file_get_size) fileSize;
	FS_PLATFORM_FUNC_PTR(platform_file_read) fileRead;
	FS_PLATFORM_FUNC_PTR(platform_file_write) fileWrite;

	FS_PLATFORM_FUNC_PTR(platform_log_write) logWrite;

	FS_PLATFORM_FUNC_PTR(platform_time_now) timeNow;
	FS_PLATFORM_FUNC_PTR(platform_time_elapsed_seconds) timeElapsedSeconds;

	FS_PLATFORM_FUNC_PTR(platform_window_get_width) windowGetWidth;
	FS_PLATFORM_FUNC_PTR(platform_window_get_height) windowGetHeight;
	// FS_PLATFORM_FUNC_PTR(platform_window_get_fullscreen) windowIsFullscreen;
	// FS_PLATFORM_FUNC_PTR(platform_window_set_fullscreen) windowSetFullscreen;
	// FS_PLATFORM_FUNC_PTR(platform_window_set_cursor_locked) windowSetCursorVisible;
	FS_PLATFORM_FUNC_PTR(platform_window_get) platformWindowGet;
	FS_PLATFORM_FUNC_PTR(platform_window_set) platformWindowSet;


	FS_PLATFORM_FUNC_PTR(input_button_went_down) buttonWentDown;
	FS_PLATFORM_FUNC_PTR(input_button_is_down) buttonIsDown;
	FS_PLATFORM_FUNC_PTR(input_button_went_up) buttonWentUp;

	FS_PLATFORM_FUNC_PTR(input_axis_get_value) inputAxisGetValue;
	FS_PLATFORM_FUNC_PTR(input_cursor_get_position) inputCursorGetPosition;
	FS_PLATFORM_FUNC_PTR(input_is_device_used) inputDeviceIsUsed;

	FS_PLATFORM_FUNC_PTR(graphics_drawing_update_camera) drawingUpdateCamera;
	FS_PLATFORM_FUNC_PTR(graphics_drawing_update_lighting) drawingUpdateLighting;
	FS_PLATFORM_FUNC_PTR(graphics_drawing_update_hdr_settings) drawingUpdateHdrSettings;

	FS_PLATFORM_FUNC_PTR(graphics_draw_model) drawModel;
	FS_PLATFORM_FUNC_PTR(graphics_draw_meshes) drawMeshes;
	FS_PLATFORM_FUNC_PTR(graphics_draw_screen_rects) drawScreenRects;
	FS_PLATFORM_FUNC_PTR(graphics_draw_lines) drawLines;
	FS_PLATFORM_FUNC_PTR(graphics_draw_procedural_mesh) drawProceduralMesh;
	FS_PLATFORM_FUNC_PTR(graphics_draw_leaves) drawLeaves;

	FS_PLATFORM_FUNC_PTR(graphics_memory_push_mesh) memoryPushMesh;
	FS_PLATFORM_FUNC_PTR(graphics_memory_push_texture) memoryPushTexture;
	FS_PLATFORM_FUNC_PTR(graphics_memory_push_material) memoryPushMaterial;
	FS_PLATFORM_FUNC_PTR(graphics_memory_push_model) memoryPushModel;
	FS_PLATFORM_FUNC_PTR(graphics_memory_unload) memoryUnload;

	FS_PLATFORM_FUNC_PTR(graphics_development_update_texture) developmentUpdateTexture;
	FS_PLATFORM_FUNC_PTR(graphics_development_reload_shaders) developmentReloadShaders;

	FS_PLATFORM_FUNC_PTR(audio_get_output_buffer) audioGetOutputBuffer;
	FS_PLATFORM_FUNC_PTR(audio_release_output_buffer) audioReleaseOutputBuffer;
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
	// FS_PLATFORM_API_SET_FUNCTION(platform_window_get_fullscreen, api->windowIsFullscreen);
	// FS_PLATFORM_API_SET_FUNCTION(platform_window_set_fullscreen, api->windowSetFullscreen);
	// FS_PLATFORM_API_SET_FUNCTION(platform_window_set_cursor_locked, api->windowSetCursorVisible);

	FS_PLATFORM_API_SET_FUNCTION(platform_window_get, api->platformWindowGet);
	FS_PLATFORM_API_SET_FUNCTION(platform_window_set, api->platformWindowSet);

	FS_PLATFORM_API_SET_FUNCTION(input_button_went_down, api->buttonWentDown);
	FS_PLATFORM_API_SET_FUNCTION(input_button_is_down, api->buttonIsDown);
	FS_PLATFORM_API_SET_FUNCTION(input_button_went_up, api->buttonWentUp);


	FS_PLATFORM_API_SET_FUNCTION(input_axis_get_value, api->inputAxisGetValue);
	FS_PLATFORM_API_SET_FUNCTION(input_cursor_get_position, api->inputCursorGetPosition);
	FS_PLATFORM_API_SET_FUNCTION(input_is_device_used, api->inputDeviceIsUsed);

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
	FS_PLATFORM_API_SET_FUNCTION(graphics_memory_push_model, api->memoryPushModel);
	FS_PLATFORM_API_SET_FUNCTION(graphics_memory_unload, api->memoryUnload);

	FS_PLATFORM_API_SET_FUNCTION(graphics_development_update_texture, api->developmentUpdateTexture);
	FS_PLATFORM_API_SET_FUNCTION(graphics_development_reload_shaders, api->developmentReloadShaders);

	FS_PLATFORM_API_SET_FUNCTION(audio_get_output_buffer, api->audioGetOutputBuffer);
	FS_PLATFORM_API_SET_FUNCTION(audio_release_output_buffer, api->audioReleaseOutputBuffer);
}

#define FS_PLATFORM_INTERFACE_HPP
#endif

