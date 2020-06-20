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
// BEGIN_C_BLOCK

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
	game::Input 	input;
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
internal void gui_start(Gui & gui, game::Input * input);
internal void gui_end();
internal void gui_generate_font_material(Gui & gui);

// Control
internal void gui_position(v2 position);
internal void gui_reset_selection();
internal void gui_ignore_input();

// Widgets
internal bool gui_button(char const * label);
internal void gui_text(char const * label);
internal void gui_image(GuiTextureHandle texture, v2 size, v4 colour = colors::white);
internal void gui_background_image(GuiTextureHandle, s32 rows, v4 colour = colors::white);

// Internal
internal void gui_render_texture(TextureHandle texture, ScreenRect rect);
internal void gui_render_text(char const * text, v2 position, v4 color);

  //////////////////////////////////
 ///  GUI IMPLEMENTATION 		///
//////////////////////////////////
internal void gui_start(Gui & gui, game::Input * input)
{
	Assert(global_currentGui == nullptr);

	global_currentGui 			= &gui;
	gui.input 					= *input;
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

internal void gui_end()
{
	global_currentGui->selectableCountLastFrame = global_currentGui->currentSelectableIndex;
	global_currentGui = nullptr;
}

internal void gui_position(v2 position)
{
	global_currentGui->currentPosition = position;
}

internal void gui_reset_selection()
{
	global_currentGui->selectedIndex = 0;
}

internal void gui_ignore_input()
{
	f32 elapsedTime 						= global_currentGui->input.elapsedTime;
	global_currentGui->input 				= {};
	global_currentGui->input.elapsedTime 	= elapsedTime;
}

internal v2 gui_transform_screen_point (v2 point)
{
	Gui & gui = *global_currentGui;

	point = {(point.x / gui.screenSize.x) * gui.screenSizeRatio * 2.0f - 1.0f,
			(point.y / gui.screenSize.y) * gui.screenSizeRatio * 2.0f - 1.0f };

	return point;
};

internal v2 gui_transform_screen_size (v2 size)
{
	Gui & gui = *global_currentGui;

	size = {(size.x / gui.screenSize.x) * gui.screenSizeRatio * 2.0f,
			(size.y / gui.screenSize.y) * gui.screenSizeRatio * 2.0f };

	return size;
};

internal void gui_render_text (char const * text, v2 position, v4 color)
{
	Gui & gui = *global_currentGui;

	f32 size 					= gui.textSize;
	s32 firstCharacter 			= ' ';
	s32 charactersPerDirection 	= 10;
	f32 characterUvSize 		= 1.0f / charactersPerDirection;

	ScreenRect rects [512];
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
	platformApi->draw_screen_rects(platformGraphics, rectIndex, rects, gui.font.atlasTexture, color);
};

internal bool gui_button(char const * label)
{
	Gui & gui = *global_currentGui;

	bool isSelected = (gui.currentSelectableIndex == gui.selectedIndex);
	++gui.currentSelectableIndex;

	gui_render_text(label, gui.currentPosition, isSelected ? gui.selectedTextColor : gui.textColor);
	gui.currentPosition.y += gui.textSize + gui.padding;

	bool result = isSelected && is_clicked(gui.input.confirm);
	return result;
}

internal void gui_text(char const * text)
{
	Gui & gui = *global_currentGui;

	gui_render_text(text, gui.currentPosition, gui.textColor);
	gui.currentPosition.y += gui.textSize + gui.padding;
}

internal void gui_image(GuiTextureHandle texture, v2 size, v4 colour)
{	
	Gui & gui = *global_currentGui;

	f32 yMovement = size.y;

	v2 position = gui_transform_screen_point(gui.currentPosition);
	size = gui_transform_screen_size(size);

	gui.currentPosition.y += yMovement + gui.padding;

	ScreenRect rect = {position, size, {0, 0}, {1, 1}};

	platformApi->draw_screen_rects(platformGraphics, 1, &rect, texture, colour);
}

internal void gui_background_image(GuiTextureHandle texture, s32 rows, v4 colour)
{	
	Gui & gui = *global_currentGui;

	v2 position = gui.currentPosition - v2{gui.padding, gui.padding};
	position = gui_transform_screen_point(position);

	f32 height = rows * gui.textSize + (rows - 1) * gui.padding;
	v2 size = {height, height};
	size += {2* gui.padding, 2*gui.padding};
	size = gui_transform_screen_size(size);

	ScreenRect rect = {position, size, {0, 0}, {1, 1}};

	platformApi->draw_screen_rects(platformGraphics, 1, &rect, texture, colour);
}

internal void gui_generate_font_material(Gui & gui)
{
	// Todo(Leo): We need something like this, but this won't work since TextureHandles do have default index -1, 
	// but if we have allocted this in array etc, it may not have been initialized to it properly...
	Assert(gui.font.atlasTexture >= 0);
	// gui.fontMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_SCREEN_GUI, 1, &gui.font.atlasTexture);
	// gui.fontTexture = platformApi->push_material(platformGraphics, )
}

// END_C_BLOCK