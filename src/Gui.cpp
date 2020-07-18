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

struct GuiDrawCall
{
	s32 				count;	
	ScreenRect * 		rects;
	GuiTextureHandle	texture;
	v4 					color;
};

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
	PlatformInput 	input;
	f32 			elapsedTime;

	v2 				currentPosition;
	s32 			currentSelectableIndex;
	v2 				screenSize;
	f32 			screenSizeRatio;

	// Per visible period state
	s32 			selectedIndex;
	s32 			selectableCountLastFrame;

	GuiTextureHandle	panelTexture;
	bool32 				isPanelActive;

	v4 				panelColor;
	v2 				panelPosition;
	v2 				panelSize;
	s32 			panelDrawCallCapacity;
	s32 			panelDrawCallCount;
	GuiDrawCall * 	panelDrawCalls;
	bool32 			panelHasTitle;
};

char const * const GUI_NO_TITLE = "GUI_NO_TITLE";

Gui * global_currentGui = nullptr;

// Maintenance
internal void gui_start_frame(Gui & gui, PlatformInput const & input, f32 elapsedTime);
internal void gui_end_frame();

// Control
internal void gui_position(v2 position);
internal void gui_reset_selection();
internal void gui_ignore_input();

// Widgets
internal bool gui_button(char const * label);
internal void gui_text(char const * label);
internal void gui_image(GuiTextureHandle texture, v2 size, v4 colour = colour_white);
internal f32 gui_float_slider(char const * label, f32 currentValue, f32 minValue, f32 maxValue);

// Internal
internal void gui_render_texture(TextureHandle texture, ScreenRect rect);
internal void gui_render_text(char const * text, v2 position, v4 color);

  //////////////////////////////////
 ///  GUI IMPLEMENTATION 		///
//////////////////////////////////
internal void gui_start_frame(Gui & gui, PlatformInput const & input, f32 elapsedTime)
{
	Assert(global_currentGui == nullptr);

	global_currentGui 			= &gui;
	// Todo(Leo): we currently copy this here, think throgh if it is really necessary
	gui.input 					= input;
	gui.currentPosition 		= {0,0};
	gui.currentSelectableIndex 	= 0;

	gui.elapsedTime 			= elapsedTime;

	if (gui.selectableCountLastFrame > 0)
	{
		if (is_clicked(input.down))
		{
			gui.selectedIndex += 1;
			gui.selectedIndex %= gui.selectableCountLastFrame;
		}
		if(is_clicked(input.up))
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

internal void gui_end_frame()
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
	global_currentGui->input = {};
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
	s32 rectCount = 0;

	while(*text != 0)
	{
		Assert(rectCount < array_count(rects) && "Too little room for such a long text");

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

		rects[rectCount] = 
		{
			.position 	= gui_transform_screen_point(glyphStart),
			.size 		= gui_transform_screen_size(glyphSize),
			.uvPosition = gui.font.uvPositionsAndSizes[index].xy,
			.uvSize 	= gui.font.uvPositionsAndSizes[index].zw,
		};

		++rectCount;

		position.x += size * gui.font.advanceWidths[index];
		++text;
	}
	platformApi->draw_screen_rects(platformGraphics, rectCount, rects, gui.font.atlasTexture, color);
};

internal f32 gui_panel_add_text_draw_call(char const * text, v4 color)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive == true);
	Assert(gui.panelDrawCallCount < gui.panelDrawCallCapacity);

	f32 size 					= gui.textSize;
	s32 firstCharacter 			= ' ';
	s32 charactersPerDirection 	= 10;
	f32 characterUvSize 		= 1.0f / charactersPerDirection;

	v2 position = gui.panelPosition;
	position.x 	+= gui.padding;
	position.y 	+= gui.panelSize.y;

	f32 textWidth = gui.padding;

	s32 rectCapacity 	= 256;
	s32 rectCount 		= 0;
	ScreenRect * rects 	= push_memory<ScreenRect>(*global_transientMemory, rectCapacity, ALLOC_NO_CLEAR);

	while(*text != 0)
	{
		Assert(rectCount < rectCapacity && "Too little room for such a long text");

		if (*text == ' ')
		{
			position.x += size * gui.font.spaceWidth;
			textWidth += size * gui.font.spaceWidth;
			++text;
			continue;
		}

		s32 index = *text - firstCharacter;
		v2 glyphStart 		= {position.x + gui.font.leftSideBearings[index], position.y};

		// Todo(Leo): also consider glyph's actual height
		v2 glyphSize 		= {gui.font.characterWidths[index] * size, size};

		rects[rectCount] = 
		{
			.position 	= gui_transform_screen_point(glyphStart),
			.size 		= gui_transform_screen_size(glyphSize),
			.uvPosition = gui.font.uvPositionsAndSizes[index].xy,
			.uvSize 	= gui.font.uvPositionsAndSizes[index].zw,
		};

		++rectCount;

		position.x += size * gui.font.advanceWidths[index];
		textWidth += size * gui.font.advanceWidths[index];
		++text;
	}

	gui.panelDrawCalls[gui.panelDrawCallCount] 	= {rectCount, rects, gui.font.atlasTexture, color};
	gui.panelDrawCallCount 						+= 1;

	gui.panelSize.x = max_f32(gui.panelSize.x, textWidth + gui.padding);
	gui.panelSize.y += gui.textSize + gui.padding;

	return textWidth;
}

internal void gui_start_panel(char const * label, v4 color)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive == false);
	Assert(global_transientMemory->checkpoint == 0);

	gui.isPanelActive 	= true;
	gui.panelColor 		= color;
	gui.panelPosition 	= gui.currentPosition;
	gui.panelSize 		= {gui.padding, gui.padding};

	gui.panelDrawCallCapacity 	= 100;
	gui.panelDrawCallCount 		= 0;
	gui.panelDrawCalls 			= push_memory<GuiDrawCall>(*global_transientMemory, gui.panelDrawCallCapacity, ALLOC_NO_CLEAR);

	if (label == GUI_NO_TITLE)
	{
		gui.panelHasTitle = false;
	}
	else
	{
		gui.panelHasTitle = true;
		gui_panel_add_text_draw_call(label, gui.textColor);
	}
}

internal void gui_end_panel()
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive == true);
	Assert(global_transientMemory->checkpoint == 0);

	gui.isPanelActive = false;

	v2 panelPosition = gui_transform_screen_point(gui.panelPosition);

	ScreenRect panelRects [] =
	{
		{panelPosition,	gui_transform_screen_size(gui.panelSize),	{0,0},	{1,1}},
		{panelPosition,	gui_transform_screen_size({gui.panelSize.x, gui.padding + gui.textSize + gui.padding}),	{0,0},	{1,1}},
	};

	// Note(Leo): If we do have title, we draw another rect to highlight it. (It becomes darker, since total alpha is more)
	s32 backrgroundDrawCount = gui.panelHasTitle ? 2 : 1;

	platformApi->draw_screen_rects(platformGraphics, backrgroundDrawCount, panelRects, gui.panelTexture, gui.panelColor);

	for (s32 i = 0; i < gui.panelDrawCallCount; ++i)
	{
		auto & call = gui.panelDrawCalls[i];
		platformApi->draw_screen_rects(platformGraphics, call.count, call.rects, call.texture, call.color);
	}
}

internal bool gui_button(char const * label)
{
	Gui & gui = *global_currentGui;

	bool isSelected = (gui.currentSelectableIndex == gui.selectedIndex);
	++gui.currentSelectableIndex;

	if(gui.isPanelActive)
	{
		gui_panel_add_text_draw_call(label, isSelected ? gui.selectedTextColor : gui.textColor);
	}
	else
	{
		gui_render_text(label, gui.currentPosition, isSelected ? gui.selectedTextColor : gui.textColor);
	}

	gui.currentPosition.y += gui.textSize + gui.padding;

	bool result = isSelected && is_clicked(gui.input.confirm);
	return result;
}

internal void gui_text(char const * text)
{
	Gui & gui = *global_currentGui;

	if(gui.isPanelActive)
	{
		gui_panel_add_text_draw_call(text, gui.textColor);
	}
	else
	{
		gui_render_text(text, gui.currentPosition, gui.textColor);
	}

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

internal f32 gui_float_slider(char const * label, f32 currentValue, f32 minValue, f32 maxValue)
{
	Gui & gui = *global_currentGui;

	constexpr f32  sliderMoveSpeed = 1.0;

	bool isSelected = (gui.currentSelectableIndex == gui.selectedIndex);
	++gui.currentSelectableIndex;

	if (isSelected)
	{
		if (is_pressed(gui.input.left))
		{
			currentValue -= sliderMoveSpeed * gui.elapsedTime * (maxValue - minValue);
		}

		if (is_pressed(gui.input.right))
		{
			currentValue += sliderMoveSpeed * gui.elapsedTime * (maxValue - minValue);
		}
	}

	v2 position;
	f32 labelTextWidth;
	if(gui.isPanelActive)
	{
		// Todo(Leo): This is a lie, it also contains padding, 18.07.2020
		labelTextWidth = gui_panel_add_text_draw_call(label, isSelected ? gui.selectedTextColor : gui.textColor);

		position = gui.panelPosition;

	
		position.x 	+= labelTextWidth + gui.padding;
		position.y 	+= gui.panelSize.y - gui.padding - gui.textSize;
	}
	else
	{
		gui_render_text(label, gui.currentPosition, isSelected ? gui.selectedTextColor : gui.textColor);

		gui.currentPosition.y += gui.padding;

		position = gui.currentPosition;
	}

	currentValue = clamp_f32(currentValue, minValue, maxValue);

	constexpr float sliderWidth = 200;
	constexpr float sliderHeight = 10;

	constexpr float handleWidth = 20;
	constexpr float handleHeight = 35;

	float sliderRectX = position.x;
	float sliderRectY = position.y + (gui.textSize / 2) - (sliderHeight / 2);
	float sliderRectW = sliderWidth;
	float sliderRectH = sliderHeight;

	float handlePositionTime = (currentValue - minValue) / (maxValue - minValue); 
	float handleRectX = position.x + (sliderWidth - handleWidth) * handlePositionTime;
	float handleRectY = position.y + (gui.textSize / 2) - (handleHeight / 2);
	float handleRectW = handleWidth;
	float handleRectH = handleHeight;

	if (gui.isPanelActive)
	{
		// gui.panelSize.y += gui.textSize;
		// gui.panelSize.y += gui.padding;

		gui.panelSize.x = max_f32(gui.panelSize.x, labelTextWidth + sliderWidth + 2 * gui.padding);
	}
	else
	{
		gui.currentPosition.y += gui.textSize;
		gui.currentPosition.y += gui.padding;		
	}

	ScreenRect sliderRect =
	{ 
		gui_transform_screen_point({sliderRectX, sliderRectY}),
		gui_transform_screen_size({sliderRectW, sliderRectH}),
		{0, 0},
		{1, 1},
	};

	ScreenRect handleRect =
	{
		gui_transform_screen_point({handleRectX, handleRectY}),
		gui_transform_screen_size({handleRectW, handleRectH}),
		{0, 0},
		{1, 1}
	};

	v4 railColour = colour_multiply({0.3, 0.3, 0.3, 0.6}, isSelected ? gui.selectedTextColor : gui.textColor);
	v4 handleColour = colour_multiply({0.8, 0.8, 0.8, 1.0}, isSelected ? gui.selectedTextColor : gui.textColor);

	if (gui.isPanelActive)
	{
		ScreenRect * rects = push_memory<ScreenRect>(*global_transientMemory, 2, ALLOC_NO_CLEAR);
		rects[0] = sliderRect;
		rects[1] = handleRect;

		gui.panelDrawCalls[gui.panelDrawCallCount] 		= {1, &rects[0], gui.panelTexture, railColour};
		gui.panelDrawCalls[gui.panelDrawCallCount + 1] 	= {1, &rects[1]	, gui.panelTexture, handleColour};
		gui.panelDrawCallCount += 2;
	}
	else
	{
		platformApi->draw_screen_rects(platformGraphics, 1, &sliderRect, gui.panelTexture, railColour);
		platformApi->draw_screen_rects(platformGraphics, 1, &handleRect, gui.panelTexture, handleColour);
	}

	return currentValue;
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

// END_C_BLOCK