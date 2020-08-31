/*=============================================================================
Leo Tamminen
shophorn @ internet

Immediate mode gui structure.
=============================================================================*/

enum GuiAlignment : s32
{
	GUI_ALIGN_LEFT,
	GUI_ALIGN_RIGHT,
	GUI_ALIGN_HORIZONTAL_STRETCH,
};

struct GuiDrawCall
{
	s32 				count;	
	ScreenRect * 		rects;
	GuiTextureHandle	texture;
	v4 					color;
	GuiAlignment 		alignment;
};

enum GuiColorFlags : s32
{
	GUI_COLOR_FLAGS_HDR 	= 1,
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

	v4 	lineColor 	= {0.1, 0.1, 0.1, 1.0};
	f32 lineWidth 	= 2;

	f32 spacing;
	f32 padding;

	// Per frame state
	PlatformInput 	input;
	v2 				mousePositionLastFrame;
	f32 			elapsedTime;

	v2 				currentPosition;
	s32 			currentSelectableIndex;
	v2 				screenSize;
	f32 			screenSizeRatio;

	// Per visible period state
	s32 			selectedIndex;
	s32 			selectableCountLastFrame;

	// Todo(Leo): do this
	// s32 			lockedSliderIndex;


	GuiTextureHandle	panelTexture;
	bool32 				isPanelActive;

	v4 				panelColor;
	v2 				panelPosition;
	v2 				panelSize;
	s32 			panelDrawCallsCapacity;
	s32 			panelDrawCallsCount;
	GuiDrawCall * 	panelDrawCalls;
	bool32 			panelHasTitle;

	// Todo(Leo): not gonna work with multiple panels...
	v2 panelCursor;
	v2 panelSizeLastFrame;

	// s32				frameScreenRectsCapacity;
	// s32 			frameScreenRectsCount;
	// ScreenRect * 	frameScreenRects;

};

// Note(Leo): actual value does not matter, we only compare pointers, as long as it is unique
char const * const GUI_NO_TITLE = "GUI_NO_TITLE";
constexpr GuiTextureHandle GUI_NO_TEXTURE = {-1};

Gui * global_currentGui = nullptr;

// Maintenance
internal void gui_start_frame(Gui & gui, PlatformInput const & input, f32 elapsedTime);
internal void gui_end_frame();

// Control
internal void gui_position(v2 position);
internal void gui_reset_selection();
internal void gui_ignore_input();

// Widgets
internal void gui_start_panel(char const * label, v4 color);
internal void gui_end_panel();

internal bool gui_button(char const * label);
internal bool gui_toggle(char const * label, bool32 & value);
internal void gui_text(char const * label);
internal void gui_image(GuiTextureHandle texture, v2 size, v4 colour = colour_white);
internal bool gui_float_slider(char const * label, f32 * value, f32 minValue, f32 maxValue);
internal bool gui_float_slider_2(char const * label, f32 * value, f32 minValue, f32 maxValue);
internal bool gui_float_field(char const * label, f32 * value);
internal bool gui_color_rgb(char const * label, v3 * color, bool enableHdr);
// internal bool gui_color_rgba(char const * label, v4 * color, bool enableHdr);
internal void gui_line();

// Internal
internal void gui_render_texture(TextureHandle texture, ScreenRect rect);
internal void gui_render_text(char const * text, v2 position, v4 color);
internal bool gui_is_under_mouse(v2 position, v2 size);
internal bool gui_is_selected();
internal void gui_panel_new_line();

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
	Gui & gui = *global_currentGui;

	gui.mousePositionLastFrame 		= gui.input.mousePosition;
	gui.selectableCountLastFrame 	= gui.currentSelectableIndex;

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

// Note(Leo): transforms point from reference sized screen space to normalized device space
internal v2 gui_transform_screen_point (v2 point)
{
	Gui & gui = *global_currentGui;

	point = {(point.x / gui.screenSize.x) * gui.screenSizeRatio * 2.0f - 1.0f,
			(point.y / gui.screenSize.y) * gui.screenSizeRatio * 2.0f - 1.0f };

	return point;
};

// Note(Leo): transforms size from reference sized screen space to normalized device space
internal v2 gui_transform_screen_size (v2 size)
{
	Gui & gui = *global_currentGui;

	size = {(size.x / gui.screenSize.x) * gui.screenSizeRatio * 2.0f,
			(size.y / gui.screenSize.y) * gui.screenSizeRatio * 2.0f};

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

internal f32 gui_panel_add_text_draw_call(char const * text, v4 color, bool appendNewLine)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive == true);
	Assert(gui.panelDrawCallsCount < gui.panelDrawCallsCapacity);

	f32 size 					= gui.textSize;
	s32 firstCharacter 			= ' ';
	s32 charactersPerDirection 	= 10;
	f32 characterUvSize 		= 1.0f / charactersPerDirection;

	v2 & cursor		= gui.panelCursor;
	f32 textWidth 	= gui.padding;

	s32 rectCapacity 	= 256;
	s32 rectCount 		= 0;
	ScreenRect * rects 	= push_memory<ScreenRect>(*global_transientMemory, rectCapacity, ALLOC_NO_CLEAR);

	while(*text != 0)
	{
		Assert(rectCount < rectCapacity && "Too little room for such a long text");

		if (*text == ' ')
		{
			cursor.x += size * gui.font.spaceWidth;
			textWidth += size * gui.font.spaceWidth;
			++text;
			continue;
		}

		s32 index = *text - firstCharacter;
		v2 glyphStart 		= {cursor.x + gui.font.leftSideBearings[index], cursor.y};

		// Todo(Leo): also consider glyph's actual height
		v2 glyphSize 		= {gui.font.characterWidths[index] * size, size};

		rects[rectCount] = 
		{
			.position 	= (glyphStart),
			.size 		= (glyphSize),
			.uvPosition = gui.font.uvPositionsAndSizes[index].xy,
			.uvSize 	= gui.font.uvPositionsAndSizes[index].zw,
		};

		++rectCount;

		cursor.x += size * gui.font.advanceWidths[index];
		textWidth += size * gui.font.advanceWidths[index];
		++text;
	}

	gui.panelDrawCalls[gui.panelDrawCallsCount] 	= {rectCount, rects, gui.font.atlasTexture, color};
	gui.panelDrawCallsCount 						+= 1;

	gui.panelSize.x = max_f32(gui.panelSize.x, textWidth + gui.padding);

	if (appendNewLine)
	{
		gui_panel_new_line();

		// // cursor.x 		= gui.panelPosition.x + gui.padding;
		// cursor.x 		= 0;//gui.padding;
		// cursor.y 		+= gui.textSize + gui.padding;
		// gui.panelSize.y += gui.textSize + gui.padding;
	}
	else
	{
		cursor.x += gui.padding;
	}


	return textWidth;
}

internal f32 gui_panel_add_text_draw_call_2(String const & text, v4 color, GuiAlignment alignment)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive == true);
	Assert(gui.panelDrawCallsCount < gui.panelDrawCallsCapacity);

	f32 size 					= gui.textSize;
	s32 firstCharacter 			= ' ';
	s32 charactersPerDirection 	= 10;
	f32 characterUvSize 		= 1.0f / charactersPerDirection;

	f32 textWidth 	= gui.padding;

	s32 rectCapacity 	= 256;
	s32 rectCount 		= 0;
	ScreenRect * rects 	= push_memory<ScreenRect>(*global_transientMemory, rectCapacity, ALLOC_NO_CLEAR);

	for(s32 i = 0; i < text.length; ++i)
	{
		Assert(rectCount < rectCapacity && "Too little room for such a long text");

		if (text[i] == ' ')
		{
			gui.panelCursor.x += size * gui.font.spaceWidth;
			textWidth += size * gui.font.spaceWidth;
			continue;
		}

		s32 index = text[i] - firstCharacter;
		v2 glyphStart 		= {gui.panelCursor.x + gui.font.leftSideBearings[index], gui.panelCursor.y};

		// Todo(Leo): also consider glyph's actual height
		v2 glyphSize 		= {gui.font.characterWidths[index] * size, size};

		rects[rectCount] = 
		{
			.position 	= (glyphStart),
			.size 		= (glyphSize),
			.uvPosition = gui.font.uvPositionsAndSizes[index].xy,
			.uvSize 	= gui.font.uvPositionsAndSizes[index].zw,
		};

		++rectCount;

		gui.panelCursor.x += size * gui.font.advanceWidths[index];
		textWidth += size * gui.font.advanceWidths[index];
	}

	gui.panelDrawCalls[gui.panelDrawCallsCount] 	= {rectCount, rects, gui.font.atlasTexture, color, alignment};
	gui.panelDrawCallsCount 						+= 1;

	gui.panelSize.x = max_f32(gui.panelSize.x, textWidth + gui.padding);

	gui.panelCursor.x += gui.padding;

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

	gui.panelDrawCallsCapacity 	= 100;
	gui.panelDrawCallsCount 	= 0;
	gui.panelDrawCalls 			= push_memory<GuiDrawCall>(*global_transientMemory, gui.panelDrawCallsCapacity, ALLOC_NO_CLEAR);

	gui.panelCursor = {0,0};
	gui.panelSize 	= {gui.padding, gui.padding};

	if (label == GUI_NO_TITLE)
	{
		gui.panelHasTitle = false;
	}
	else
	{
		gui_panel_add_text_draw_call(label, gui.textColor, false);

		gui.panelHasTitle 	= true;
		gui.panelCursor.x 	= 0;
		gui.panelCursor.y 	+= gui.spacing + gui.textSize + gui.spacing + gui.spacing;
		gui.panelSize.y 	+= gui.spacing + gui.textSize + gui.spacing + gui.spacing;
	}

}

internal void gui_end_panel()
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive == true);
	Assert(global_transientMemory->checkpoint == 0);

	gui.isPanelActive 		= false;

	gui.panelSize.y += gui.padding - gui.spacing;

	gui.panelSizeLastFrame 	= gui.panelSize;

	v2 screenSpacePanelPosition 		= gui_transform_screen_point(gui.panelPosition);
	v2 screenSpacePanelTopLeftOffset 	= screenSpacePanelPosition - v2{-1,-1};
	v2 screenSpacePanelTopRightOffset 	= screenSpacePanelPosition - v2{1,-1};

	v2 pixelSpaceTopLeft 	= gui.panelPosition + v2{gui.padding, gui.padding};
	v2 pixelSpaceTopRight 	= gui.panelPosition + v2 {gui.panelSize.x - gui.padding, gui.padding}; 

	ScreenRect panelRects [] =
	{
		{screenSpacePanelPosition,	gui_transform_screen_size(gui.panelSize),	{0,0},	{1,1}},
		// Todo(Leo): its weird to add only half padding on other side, do something about that
		{screenSpacePanelPosition,	gui_transform_screen_size({gui.panelSize.x, gui.padding + gui.textSize + gui.padding * 0.5f}), {0,0},	{1,1}},
	};

	// Note(Leo): If we do have title, we draw another rect to highlight it. (It becomes darker, since total alpha is more)
	s32 backrgroundDrawCount = gui.panelHasTitle ? 2 : 1;

	platformApi->draw_screen_rects(platformGraphics, backrgroundDrawCount, panelRects, gui.panelTexture, gui.panelColor);

	/// TRANSFORM PANEL DRAW CALLS
	for (s32 callIndex = 0; callIndex < gui.panelDrawCallsCount; ++callIndex)
	{
		GuiDrawCall & call = gui.panelDrawCalls[callIndex]; 

		for (s32 rectIndex = 0; rectIndex < call.count; ++rectIndex)
		{
			ScreenRect & rect = call.rects[rectIndex];

			switch(call.alignment)
			{
				case GUI_ALIGN_HORIZONTAL_STRETCH:
					rect.position.x = pixelSpaceTopLeft.x;
					rect.position.y += pixelSpaceTopLeft.y;
					rect.size.x 	= pixelSpaceTopRight.x - pixelSpaceTopLeft.x;
					break;

				case GUI_ALIGN_LEFT:
					rect.position 	+= pixelSpaceTopLeft;
					break;
				
				case GUI_ALIGN_RIGHT:
					rect.position += pixelSpaceTopRight;
					break;
			}

			rect.position 	= gui_transform_screen_point(rect.position);
			rect.size 		= gui_transform_screen_size(rect.size);
		}

	}

	/// DRAW PANEL DRAW CALLS
	for (s32 i = 0; i < gui.panelDrawCallsCount; ++i)
	{
		auto & call = gui.panelDrawCalls[i];
		platformApi->draw_screen_rects(platformGraphics, call.count, call.rects, call.texture, call.color);
	}
}

internal bool gui_button(char const * label)
{
	Gui & gui 		= *global_currentGui;
	bool isSelected = gui_is_selected();

	if(gui.isPanelActive)
	{
		gui_panel_add_text_draw_call(label, isSelected ? gui.selectedTextColor : gui.textColor, true);
	}
	else
	{
		gui_render_text(label, gui.currentPosition, isSelected ? gui.selectedTextColor : gui.textColor);
		gui.currentPosition.y += gui.textSize + gui.padding;
	}

	bool result = isSelected && (is_clicked(gui.input.confirm) || is_clicked(gui.input.mouse0));
	return result;
}

internal bool gui_toggle(char const * label, bool32 * value)
{
	Gui & gui 		= *global_currentGui;
	Assert(gui.isPanelActive);

	bool isSelected = gui_is_selected();
	bool modified 	= false;

	if (isSelected && is_clicked(gui.input.confirm) || is_clicked(gui.input.mouse0))
	{
		logDebug(0) << "Toggle toggled";

		*value 		= !*value;
		modified 	= true;
	}

	CStringBuilder builder = label;
	if (*value)
	{
		builder.append_cstring(": ON");	
	}
	else
	{
		builder.append_cstring(": OFF");	
	}

	gui_panel_add_text_draw_call(builder, isSelected ? gui.selectedTextColor : gui.textColor, true);

	return modified;
}


internal void gui_text(char const * text)
{
	Gui & gui = *global_currentGui;

	if(gui.isPanelActive)
	{
		gui_panel_add_text_draw_call(text, gui.textColor, true);
	}
	else
	{
		gui_render_text(text, gui.currentPosition, gui.textColor);
		gui.currentPosition.y += gui.textSize + gui.padding;
	}
}

internal void gui_image(GuiTextureHandle texture, v2 size, v4 colour)
{	
	Gui & gui = *global_currentGui;

	f32 yMovement = size.y;

	v2 position = gui_transform_screen_point(gui.currentPosition);
	size = gui_transform_screen_size(size);

	gui.currentPosition.y += yMovement + gui.padding;

	ScreenRect rect = {position, size, {0, 0}, {1, 1}};

	if (texture == GUI_NO_TEXTURE)
	{
		texture = gui.panelTexture;
	}

	platformApi->draw_screen_rects(platformGraphics, 1, &rect, texture, colour);
}

internal bool gui_float_slider(char const * label, f32 * value, f32 minValue, f32 maxValue)
{
	Gui & gui = *global_currentGui;

	Assert (gui.isPanelActive);

	constexpr f32 sliderMoveSpeed 	= 1.0;
	bool isSelected 				= gui_is_selected();

	bool modified 			= false;
	bool modifiedByMouse 	= isSelected && is_pressed(gui.input.mouse0);

	if (isSelected)
	{
		if (is_pressed(gui.input.left))
		{
			*value 		-= sliderMoveSpeed * gui.elapsedTime * (maxValue - minValue);
			modified 	= true;
		}

		if (is_pressed(gui.input.right))
		{
			*value 		+= sliderMoveSpeed * gui.elapsedTime * (maxValue - minValue);
			modified 	= true;
		}
	}

	v2 position = gui.panelCursor;
	f32 labelTextWidth;
	// if(gui.isPanelActive)
	// {
	labelTextWidth = gui_panel_add_text_draw_call(label, isSelected ? gui.selectedTextColor : gui.textColor, false);
	// }
	// else
	// {
	// 	gui_render_text(label, gui.currentPosition, isSelected ? gui.selectedTextColor : gui.textColor);
	// 	gui.currentPosition.y += gui.padding;
	// 	position = gui.currentPosition;
	// }

	*value = clamp_f32(*value, minValue, maxValue);

	constexpr float sliderWidth = 200;
	float sliderHeight = gui.textSize;

	float handleWidth 	= gui.textSize * (5.0f / 8.0f);
	float handleHeight 	= gui.textSize;

	float sliderRectX = position.x - sliderWidth;
	float sliderRectY = position.y + (gui.textSize / 2) - (sliderHeight / 2);
	float sliderRectW = sliderWidth;
	float sliderRectH = sliderHeight;

	float handlePositionTime = (*value - minValue) / (maxValue - minValue); 

	if (modifiedByMouse)
	{
		f32 x = gui.input.mousePosition.x / gui.screenSizeRatio;
		x -= (sliderRectX + handleWidth / 2) + gui.panelSizeLastFrame.x - sliderWidth;
		x /= (sliderRectW - handleWidth);

		handlePositionTime = x;
		handlePositionTime = clamp_f32(handlePositionTime, 0, 1);

		*value = lerp_f32(minValue, maxValue, handlePositionTime);
	}

	float handleRectX = position.x + (sliderWidth - handleWidth) * handlePositionTime - sliderWidth;
	float handleRectY = position.y + (gui.textSize / 2) - (handleHeight / 2);
	float handleRectW = handleWidth;
	float handleRectH = handleHeight;

	gui.panelSize.x = max_f32(gui.panelSize.x, labelTextWidth + sliderWidth + 2 * gui.padding);

	ScreenRect sliderRect =
	{ 
		{sliderRectX, sliderRectY},
		{sliderRectW, sliderRectH},
		{0, 0},
		{1, 1},
	};

	ScreenRect handleRect =
	{
		{handleRectX, handleRectY},
		{handleRectW, handleRectH},
		{0, 0},
		{1, 1}
	};

	v4 railColour = colour_multiply({0.3, 0.3, 0.3, 0.6}, isSelected ? gui.selectedTextColor : gui.textColor);
	v4 handleColour = colour_multiply({0.8, 0.8, 0.8, 1.0}, isSelected ? gui.selectedTextColor : gui.textColor);

	ScreenRect * rects = push_memory<ScreenRect>(*global_transientMemory, 2, ALLOC_NO_CLEAR);
	rects[0] = sliderRect;
	rects[1] = handleRect;
	
	gui.panelDrawCalls[gui.panelDrawCallsCount] 		= {1, &rects[0], gui.panelTexture, railColour, GUI_ALIGN_RIGHT };
	gui.panelDrawCalls[gui.panelDrawCallsCount + 1] 	= {1, &rects[1]	, gui.panelTexture, handleColour, GUI_ALIGN_RIGHT };
	gui.panelDrawCallsCount += 2;

	char buffer [16] 	= {};
	String valueFormat 	= {0, buffer};
	string_append_f32(valueFormat, *value, 16);

	v2 backUpCursor 	= gui.panelCursor;
	gui.panelCursor.x 	= sliderRectX + gui.padding;
	gui_panel_add_text_draw_call_2(valueFormat, {0,0,0,1}, GUI_ALIGN_RIGHT);
	gui.panelCursor 	= backUpCursor;

	gui_panel_new_line();
	
	return modified;
}

internal bool gui_float_slider_2(char const * label, f32 * value, f32 minValue, f32 maxValue)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive);

	constexpr f32 sliderMoveSpeed = 1.0;
	bool isSelected = gui_is_selected();

	bool modified 			= false;
	bool modifiedByMouse 	= isSelected && is_pressed(gui.input.mouse0);

	if (isSelected)
	{
		if (is_pressed(gui.input.left))
		{
			*value 		-= sliderMoveSpeed * gui.elapsedTime * (maxValue - minValue);
			modified 	= true;
		}

		if (is_pressed(gui.input.right))
		{
			*value 		+= sliderMoveSpeed * gui.elapsedTime * (maxValue - minValue);
			modified 	= true;
		}
	}
	
	if (modifiedByMouse)
	{
		f32 mouseDelta 	= gui.input.mousePosition.x - gui.mousePositionLastFrame.x;
		*value 			+= mouseDelta / 100;
	}

	*value = clamp_f32(*value, minValue, maxValue);

	char valueTextBuffer [10] = {};
	{
		s32 valueTextIndex = 0;
	
		if (*value < 0)
		{
			valueTextBuffer[valueTextIndex++] = '-';
		}

		f32 _value = abs_f32(*value);

		s32 digits [7] = {};
		digits[0] = ((s32)floor_f32(_value / 1000)) % 10;
		digits[1] = ((s32)floor_f32(_value / 100)) % 10;
		digits[2] = ((s32)floor_f32(_value / 10)) % 10;
		digits[3] = ((s32)floor_f32(_value)) % 10;
		digits[4] = ((s32)floor_f32(_value * 10)) % 10;
		digits[5] = ((s32)floor_f32(_value * 100)) % 10;
		digits[6] = ((s32)floor_f32(_value * 1000)) % 10;

		bool leadingZero = true;

		if (digits[0] > 0)
		{
			valueTextBuffer[valueTextIndex++] = '0' + digits[0];
			leadingZero = false;
		}

		if(digits[1] > 0 || leadingZero == false)
		{
			valueTextBuffer[valueTextIndex++] = '0' + digits[1];
			leadingZero = false;
		}

		if (digits[2] > 0 || leadingZero == false)
		{
			valueTextBuffer[valueTextIndex++] = '0' + digits[2];
			leadingZero = false;
		}

		valueTextBuffer[valueTextIndex++] = '0' + digits[3];
		valueTextBuffer[valueTextIndex++] = '.';
		valueTextBuffer[valueTextIndex++] = '0' + digits[4];
		valueTextBuffer[valueTextIndex++] = '0' + digits[5];
		valueTextBuffer[valueTextIndex++] = '0' + digits[6];
	}

	v2 position;
	f32 labelTextWidth;
	// if(gui.isPanelActive)
	// {
		// Todo(Leo): This is a lie, it also contains padding, 18.07.2020
		labelTextWidth 	= gui_panel_add_text_draw_call(label, isSelected ? gui.selectedTextColor : gui.textColor, false);

		// position 		= gui.panelPosition;

		// position.x 		+= labelTextWidth + gui.padding;
		// position.y 		+= gui.panelSize.y - gui.padding - gui.textSize;

		// gui.panelPosition = position;

		labelTextWidth 	+= gui_panel_add_text_draw_call(valueTextBuffer, isSelected ? gui.selectedTextColor : gui.textColor, false);

		gui_panel_new_line();
	// }
	// else
	// {
	// 	gui_render_text(label, gui.currentPosition, isSelected ? gui.selectedTextColor : gui.textColor);
	// 	// gui_render_text(valueTextBuffer, isSelected ? gui.selectedTextColor : gui.textColor);
	// 	gui.currentPosition.y += gui.padding;
	// 	position = gui.currentPosition;
	// }

	return modified;
}

internal bool gui_float_field(char const * label, f32 * value)
{
Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive);

	constexpr f32 sliderMoveSpeed = 0.1;
	bool isSelected = gui_is_selected();

	bool modified 			= false;
	bool modifiedByMouse 	= isSelected && is_pressed(gui.input.mouse0);


	if (isSelected)
	{
		if (is_pressed(gui.input.left))
		{
			*value 		-= sliderMoveSpeed * gui.elapsedTime;
			modified 	= true;
		}

		if (is_pressed(gui.input.right))
		{
			*value 		+= sliderMoveSpeed * gui.elapsedTime;
			modified 	= true;
		}
	}
	
	constexpr f32 mouseMoveScale = 0.2;
	if (modifiedByMouse)
	{
		f32 mouseDelta 	= gui.input.mousePosition.x - gui.mousePositionLastFrame.x;
		mouseDelta 		= mouseMoveScale * mouseDelta * mouseDelta;
		*value 			+= mouseDelta;
	}

	// char valueTextBuffer [10] = {};
	// {
	// 	s32 valueTextIndex = 0;
	
	// 	if (*value < 0)
	// 	{
	// 		valueTextBuffer[valueTextIndex++] = '-';
	// 	}

	// 	f32 _value = abs_f32(*value);

	// 	s32 digits [7] = {};
	// 	digits[0] = ((s32)floor_f32(_value / 1000)) % 10;
	// 	digits[1] = ((s32)floor_f32(_value / 100)) % 10;
	// 	digits[2] = ((s32)floor_f32(_value / 10)) % 10;
	// 	digits[3] = ((s32)floor_f32(_value)) % 10;
	// 	digits[4] = ((s32)floor_f32(_value * 10)) % 10;
	// 	digits[5] = ((s32)floor_f32(_value * 100)) % 10;
	// 	digits[6] = ((s32)floor_f32(_value * 1000)) % 10;

	// 	bool leadingZero = true;

	// 	if (digits[0] > 0)
	// 	{
	// 		valueTextBuffer[valueTextIndex++] = '0' + digits[0];
	// 		leadingZero = false;
	// 	}

	// 	if(digits[1] > 0 || leadingZero == false)
	// 	{
	// 		valueTextBuffer[valueTextIndex++] = '0' + digits[1];
	// 		leadingZero = false;
	// 	}

	// 	if (digits[2] > 0 || leadingZero == false)
	// 	{
	// 		valueTextBuffer[valueTextIndex++] = '0' + digits[2];
	// 		leadingZero = false;
	// 	}

	// 	valueTextBuffer[valueTextIndex++] = '0' + digits[3];
	// 	valueTextBuffer[valueTextIndex++] = '.';
	// 	valueTextBuffer[valueTextIndex++] = '0' + digits[4];
	// 	valueTextBuffer[valueTextIndex++] = '0' + digits[5];
	// 	valueTextBuffer[valueTextIndex++] = '0' + digits[6];
	// }

	auto push_temp_string = [](s32 capacity) -> String
	{
		String result = {0, push_memory<char>(*global_transientMemory, capacity, ALLOC_CLEAR)};
		return result;
	};

	String string = push_temp_string(128);
	string_append_f32(string, *value, 128);

	gui_panel_add_text_draw_call(label, isSelected ? gui.selectedTextColor : gui.textColor, false);
	gui_panel_add_text_draw_call_2(string, isSelected ? gui.selectedTextColor : gui.textColor, GUI_ALIGN_LEFT);

	gui_panel_new_line();

	return modified;	
}


internal bool gui_color_rgb(char const * label, v3 * color, GuiColorFlags flags)
{
	Gui & gui = *global_currentGui;
	
	gui_text(label);
	
	v2 startCursor = gui.panelCursor;

	bool hdr = (flags & GUI_COLOR_FLAGS_HDR) != 0;
	f32 max = hdr ? 10 : 1;

	bool modified 	= gui_float_slider_2("R:", &color->r, 0, max);
	modified 		= gui_float_slider_2("G:", &color->g, 0, max) || modified;
	modified 		= gui_float_slider_2("B:", &color->b, 0, max) || modified;

	// Todo(Leo): Add this to rgba version of this function
	// if ((flags & GUI_COLOR_FLAGS_ALPHA) != 0)
	// {
	// 	modified 		= gui_float_slider_2("A:", &color->a, 0, 1) || modified;
	// }

	v4 displayColor;
	displayColor.rgb 	= *color;
	displayColor.a 		= 1;

	v2 texturePosition = startCursor + v2{200, gui.padding};

	ScreenRect * rect = push_memory<ScreenRect>(*global_transientMemory, 2, 0);

	rect[0] = 
	{
		.position 	= texturePosition - v2 {gui.lineWidth, gui.lineWidth},
		.size 		= v2{gui.textSize, gui.textSize} * 3.0f + v2{gui.lineWidth * 2, gui.lineWidth * 2},
		.uvPosition = {0,0},
		.uvSize 	= {1,1},
	};

	rect[1] = 
	{
		.position 	= texturePosition,
		.size 		= v2{gui.textSize, gui.textSize} * 3.0f,
		.uvPosition = {0,0},
		.uvSize 	= {1,1},
	};

	gui.panelDrawCalls[gui.panelDrawCallsCount++] 	= {1, &rect[0], gui.panelTexture, gui.lineColor};
	gui.panelDrawCalls[gui.panelDrawCallsCount++] 	= {1, &rect[1], gui.panelTexture, displayColor};
	// gui.panelDrawCallsCount 						+= 2;

	return modified;
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

internal void gui_line()
{
	Gui & gui = *global_currentGui;
	Assert(gui.isPanelActive);

	f32 lineLength 		= 350;

	// Todo(Leo): allocate this to gui in the beninnginnein
	ScreenRect * rect = push_memory<ScreenRect>(*global_transientMemory, 1, 0);

	v2 position = gui.panelCursor;
	position.y += gui.padding;

	*rect = 
	{
		.position 	= position,
		.size 		= {lineLength, gui.lineWidth},
		.uvPosition = {0,0},
		.uvSize 	= {1,1},
	};

	gui.panelDrawCalls[gui.panelDrawCallsCount] 	= {1, rect, gui.panelTexture, gui.lineColor, GUI_ALIGN_HORIZONTAL_STRETCH};
	gui.panelDrawCallsCount 						+= 1;

	gui.panelCursor.y 	+= 2 * gui.padding + gui.lineWidth;
	gui.panelSize.y 	+= 2 * gui.padding + gui.lineWidth;
}


internal bool gui_is_under_mouse(v2 position, v2 size)
{
	position 	= position * global_currentGui->screenSizeRatio;
	size 		= size * global_currentGui->screenSizeRatio;

	v2 mousePosition 	= global_currentGui->input.mousePosition;
	mousePosition 		-= position;

	bool isUnderMouse = mousePosition.x > 0 && mousePosition.x < size.x && mousePosition.y > 0 && mousePosition.y < size.y;
	return isUnderMouse;
}

internal bool gui_is_selected()
{
	Gui & gui = *global_currentGui;

	// Todo(Leo): do some kind of "current rect" thing, or pass it here
	bool isUnderMouse 	= gui_is_under_mouse(gui.panelPosition + v2{gui.padding, gui.padding} + gui.panelCursor, {400, gui.textSize});// || isSelected;
	bool mouseHasMoved 	= magnitude_v2(gui.mousePositionLastFrame - gui.input.mousePosition) > 0.1f;

	if (isUnderMouse && mouseHasMoved)
	{
		gui.selectedIndex = gui.currentSelectableIndex;
	}

	bool isSelected = (gui.currentSelectableIndex == gui.selectedIndex);
	++gui.currentSelectableIndex;

	return isSelected;	
}

internal void gui_panel_new_line()
{
	Gui & gui = *global_currentGui;

	gui.panelCursor.x 	= 0;
	gui.panelCursor.y 	+= gui.textSize + gui.spacing;
	gui.panelSize.y 	+= gui.textSize + gui.spacing;
}