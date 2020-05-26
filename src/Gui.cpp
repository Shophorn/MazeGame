/*=============================================================================
Leo Tamminen
shophorn @ internet

Immediate mode gui structure.
=============================================================================*/

#define BEGIN_C_BLOCK extern "C" {
#define END_C_BLOCK }

/* Note(Leo): We do this to get compile errors when forward declaration and 
definition signatures do not match. C does not allow overloading and having 
same name on two functions with different signatures generates error. With c++
this could be solved with namespaces by forward declaring in namespace block
and defining with fully qualified name, but I am currently experimenting with
more C-like coding style. 22.5.2020 */
BEGIN_C_BLOCK

struct Gui
{
	// References
	Font 			font;
	MaterialHandle 	fontMaterial;

	// Settings
	v2 referenceScreenSize = {1920, 1080};

	// Style
	v4 	textColor;
	f32 textSize;
	v4 	selectedTextColor;
	f32 padding;

	// Per frame state
	game::Input * 	input;
	v2 				currentPosition;
	s32 			currentSelectableIndex;
	v2 				screenSize;
	f32 			screenSizeRatio;

	// Per visible period state
	s32 			selectedIndex;
	s32 			selectableCountLastFrame;
};

Gui * global_currentGui = nullptr;

// Maintenance
void gui_start(Gui & gui, game::Input * input);
void gui_end();
void gui_generate_font_material(Gui & gui);

// Layout
void gui_position(v2 position);
void gui_reset_selection();

// Widgets
bool gui_button(char const * label);
void gui_text(char const * label);
void gui_image(TextureHandle texture, v2 size);

// Internal
void gui_render_texture(TextureHandle texture, ScreenRect rect);
void gui_render_text(char const * text, v2 position, v4 color);

  //////////////////////////////////
 ///  GUI IMPLEMENTATION 		///
//////////////////////////////////
void gui_start(Gui & gui, game::Input * input)
{
	Assert(global_currentGui == nullptr);

	global_currentGui 			= &gui;
	gui.input 					= input;
	gui.currentPosition 		= {0,0};
	gui.currentSelectableIndex 	= 0;

	if (gui.selectableCountLastFrame > 0)
	{
		if (is_clicked(input->down))
		{
			gui.selectedIndex += 1;
			gui.selectedIndex %= gui.selectableCountLastFrame;
		}
		if(is_clicked(input->up))
		{
			gui.selectedIndex -= 1;
			if (gui.selectedIndex < 0)
			{
				gui.selectedIndex += gui.selectableCountLastFrame;
			}
		}
	}

	gui.screenSize = { 	(f32)platformApi->get_window_width(platformWindow),
						(f32)platformApi->get_window_height(platformWindow) };

	gui.screenSizeRatio = gui.screenSize.x / gui.referenceScreenSize.x;
}

void gui_end()
{
	global_currentGui->selectableCountLastFrame = global_currentGui->currentSelectableIndex;
	global_currentGui = nullptr;
}

void gui_position(v2 position)
{
	global_currentGui->currentPosition = position;
}

void gui_reset_selection()
{
	global_currentGui->selectedIndex = 0;
}

v2 gui_transform_screen_point (v2 point)
{
	Gui & gui = *global_currentGui;

	point = {(point.x / gui.screenSize.x) * gui.screenSizeRatio * 2.0f - 1.0f,
			(point.y / gui.screenSize.y) * gui.screenSizeRatio * 2.0f - 1.0f };

	return point;
};

v2 gui_transform_screen_size (v2 size)
{
	Gui & gui = *global_currentGui;

	size = {(size.x / gui.screenSize.x) * gui.screenSizeRatio * 2.0f,
			(size.y / gui.screenSize.y) * gui.screenSizeRatio * 2.0f };

	return size;
};

void gui_render_text (char const * text, v2 position, v4 color)
{
	Gui & gui = *global_currentGui;

	f32 size 					= gui.textSize;
	s32 firstCharacter 			= ' ';
	s32 charactersPerDirection 	= 10;
	f32 characterUvSize 		= 1.0f / charactersPerDirection;

	ScreenRect rects [256];
	s32 rectIndex = 0;

	while(*text != 0)
	{
		Assert(rectIndex < array_count(rects) && "Too little room for such a long text");

		if (*text == ' ')
		{
			position.x += size * gui.font.spaceWidth;
			++text;
			continue;
		}

		s32 index = *text - firstCharacter;
		v2 glyphStart 		= {position.x + gui.font.leftSideBearings[index], position.y};

		// Todo(Leo): also consider glyph's actual height
		v2 glyphSize 		= {gui.font.characterWidths[index] * size, size};

		rects[rectIndex] = 
		{
			.position 	= gui_transform_screen_point(glyphStart),
			.size 		= gui_transform_screen_size(glyphSize),
			.uvPosition = gui.font.uvPositionsAndSizes[index].xy,
			.uvSize 	= gui.font.uvPositionsAndSizes[index].zw,
		};

		++rectIndex;

		position.x += size * gui.font.advanceWidths[index];
		++text;
	}
	platformApi->draw_screen_rects(platformGraphics, rectIndex, rects, gui.fontMaterial, color);
};

bool gui_button(char const * label)
{
	Gui & gui = *global_currentGui;

	bool isSelected = (gui.currentSelectableIndex == gui.selectedIndex);
	++gui.currentSelectableIndex;

	gui_render_text(label, gui.currentPosition, isSelected ? gui.selectedTextColor : gui.textColor);
	gui.currentPosition.y += gui.textSize + gui.padding;

	bool result = isSelected && is_clicked(gui.input->confirm);
	return result;
}

void gui_text(char const * text)
{
	Gui & gui = *global_currentGui;

	gui_render_text(text, gui.currentPosition, gui.textColor);
	gui.currentPosition.y += gui.textSize + gui.padding;
}

void gui_image(TextureHandle texture, v2 size)
{
	// platformApi->draw_screen_rects
}

void gui_generate_font_material(Gui & gui)
{
	// Todo(Leo): We need something like this, but this won't work since TextureHandles do have default index -1, 
	// but if we have allocted this in array etc, it may not have been initialized to it properly...
	Assert(gui.font.atlasTexture >= 0);

	auto guiPipeline = platformApi->push_pipeline (	platformGraphics,
													"assets/shaders/gui_vert3.spv",
					    							"assets/shaders/gui_frag2.spv",
					    							{
										                .textureCount           = 1,
										                .pushConstantSize       = sizeof(v2) * 4 + sizeof(v4),

										                .primitiveType          = platform::RenderingOptions::PRIMITIVE_TRIANGLE_STRIP,
										                .cullMode               = platform::RenderingOptions::CULL_NONE,

														.enableDepth 			= false,
										                .useVertexInput         = false,
										                .useSceneLayoutSet      = false,
										                .useMaterialLayoutSet   = true,
										                .useModelLayoutSet      = false,
										                .enableTransparency     = true
										            });

	MaterialAsset guiFontMaterialInfo = {guiPipeline, allocate_array<TextureHandle>(*global_transientMemory, {gui.font.atlasTexture})};
	gui.fontMaterial = platformApi->push_material(platformGraphics, &guiFontMaterialInfo);
}

END_C_BLOCK