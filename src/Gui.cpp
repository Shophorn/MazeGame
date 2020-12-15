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
	s32 			count;	
	ScreenRect * 	rects;
	MaterialHandle	material;
	v4 				color;
	GuiAlignment 	alignment;
};

enum GuiColorFlags : s32
{
	GUI_COLOR_FLAGS_NONE 	= 0,
	GUI_COLOR_FLAGS_HDR 	= 1,
};

struct GuiClampValuesF32
{
	f32 min = lowest_f32;
	f32 max = highest_f32;
};

struct GuiClampValuesS32
{
	s32 min = min_value_s32;	
	s32 max = max_value_s32;	
};

struct GuiClampValuesV2
{
	v2 min = {lowest_f32, lowest_f32};
	v2 max = {highest_f32, highest_f32};
};

struct Gui
{
	// References
	Font *			font;
	MaterialHandle 	fontMaterial;

	// Settings
	v2 referenceScreenSize = {1920, 1080};

	// Style
	v4 	textColor;
	f32 textSize;
	v4 	selectedTextColor;

	v4 	lineColor 	= {0.1, 0.1, 0.1, 1.0};
	// Todo(Leo): Line height actually
	f32 lineWidth 	= 2;


	f32 spacing;
	f32 padding;

	// Parsed input state
	bool8 event_escape;
	bool8 event_confirm;
	bool8 event_moveDown;
	bool8 event_moveUp;

	// Per frame state
	PlatformInput *	input;
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


	MaterialHandle	panelTexture;
	bool32 			isPanelActive;

	v4 				panelColor;
	v2 				panelPosition;
	v2 				panelSize;
	s32 			panelDrawCallsCapacity;
	s32 			panelDrawCallsCount;
	GuiDrawCall * 	panelDrawCalls;
	bool32 			panelHasTitle;

	// Todo(Leo): not gonna work with multiple panels...
	v2 panelSizeLastFrame;

	// f32 panelCursorX;
	f32 panelCursorY;

	f32 panelCurrentLineLength;
	f32 panelMaxLineLength;

	f32 panelLeftCursorX;
	f32 panelCenterCursorX;
	f32 panelRightCursorX;

	s32 drawCallCountOnLineBegin;

	// s32				frameScreenRectsCapacity;
	// s32 			frameScreenRectsCount;
	// ScreenRect * 	frameScreenRects;

};

// ............................................................................
// CONSTANT VALUES THAT CAN BE PASSED TO SOME WIDGETS

// Note(Leo): actual value does not matter, we only compare pointers, as long as it is unique
char const * const GUI_NO_TITLE = "GUI_NO_TITLE";
// constexpr MaterialHandle GUI_NO_TEXTURE = {-1};

// ............................................................................

Gui * global_currentGui = nullptr;

// Maintenance
internal void gui_start_frame(Gui & gui, PlatformInput * input, f32 elapsedTime);
internal void gui_end_frame();

// Control
internal void gui_position(v2 position);
internal void gui_reset_selection();
internal void gui_ignore_input();

// Widgets
internal void gui_start_panel(char const * label, v4 color);
internal void gui_end_panel();

internal bool gui_button(char const * label);
internal bool gui_toggle(char const * label, bool32 * value);
internal void gui_text(char const * label);
internal void gui_image(MaterialHandle texture, v2 size, v4 colour = colour_white);

internal bool gui_float_slider(char const * label, f32 * value, f32 minValue, f32 maxValue);
internal bool gui_float_field(char const * label, f32 * value, GuiClampValuesF32 clamp = {});
internal bool gui_int_field(char const * label, s32 * value, GuiClampValuesS32 clamp = {});

internal bool gui_vector_2_field(char const * label, v2 * value, GuiClampValuesV2 const & clamp = {});

internal bool gui_colour_rgb(char const * label, v3 * color, GuiColorFlags = GUI_COLOR_FLAGS_NONE);
// internal bool gui_colour_rgba(char const * label, v4 * color, bool enableHdr);
internal void gui_line();


// Internal
internal void gui_render_texture(TextureHandle texture, ScreenRect rect);
internal bool gui_is_under_mouse(v2 position, v2 size);
internal bool gui_is_selected();
internal void gui_panel_new_line();
internal void gui_push_line_width(f32 width);
internal void gui_label(char const * label, v4 colour);

  //////////////////////////////////
 ///  GUI IMPLEMENTATION 		///
//////////////////////////////////
internal void gui_start_frame(Gui & gui, PlatformInput * input, f32 elapsedTime)
{
	Assert(global_currentGui == nullptr);

	global_currentGui 			= &gui;
	// Todo(Leo): we currently copy this here, think throgh if it is really necessary
	gui.input 					= input;

	gui.currentPosition 		= {0,0};
	gui.currentSelectableIndex 	= 0;

	gui.elapsedTime 			= elapsedTime;


	gui.screenSize = { 	(f32)platform_window_get_width(platformWindow),
						(f32)platform_window_get_height(platformWindow) };

	gui.screenSizeRatio = gui.screenSize.x / gui.referenceScreenSize.x;


	// -------------- Trigger events ---------------------
	gui.event_confirm = input_button_went_down(input, InputButton_nintendo_a)
						|| input_button_went_down(input, InputButton_keyboard_enter);

	gui.event_escape = input_button_went_down(input, InputButton_nintendo_b)
						|| input_button_went_down(input, InputButton_keyboard_escape);

	gui.event_moveDown = input_button_went_down(input, InputButton_dpad_down)
							|| input_button_went_down(input, InputButton_keyboard_down)
							|| input_button_went_down(input, InputButton_wasd_down);

	gui.event_moveUp = input_button_went_down(input, InputButton_dpad_up)
						|| input_button_went_down(input, InputButton_keyboard_up)
						|| input_button_went_down(input, InputButton_wasd_up);


	// ---------------- Handle selection -----------------
	// Note(Leo): these use events specified in this same function, but we are maybe moving triggerin
	// events outside of this function
	if (gui.selectableCountLastFrame > 0)
	{
		if (gui.event_moveDown)
		{
			gui.selectedIndex += 1;
			gui.selectedIndex %= gui.selectableCountLastFrame;
		}
		if(gui.event_moveUp)
		{
			gui.selectedIndex -= 1;
			if (gui.selectedIndex < 0)
			{
				gui.selectedIndex += gui.selectableCountLastFrame;
			}
		}
	}


}

internal void gui_end_frame()
{
	Gui & gui = *global_currentGui;

	gui.mousePositionLastFrame 		= input_cursor_get_position(gui.input);
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

internal f32 & gui_get_x_cursor(GuiAlignment alignment)
{
	Gui & gui = *global_currentGui;

	switch (alignment)
	{
		case GUI_ALIGN_RIGHT: return gui.panelRightCursorX;
		case GUI_ALIGN_LEFT:
		default:
			return gui.panelLeftCursorX;
	}
}

internal void gui_panel_add_text_draw_call(String const & text, v4 color, GuiAlignment alignment)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive == true);
	Assert(gui.panelDrawCallsCount < gui.panelDrawCallsCapacity);

	f32 size 					= gui.textSize;
	s32 charactersPerDirection 	= 10;
	f32 characterUvSize 		= 1.0f / charactersPerDirection;

	f32 textWidth 	= gui.padding;

	s32 rectCapacity 	= text.length;
	s32 rectCount 		= 0;
	ScreenRect * rects 	= push_memory<ScreenRect>(*global_transientMemory, rectCapacity, ALLOC_GARBAGE);

	f32 & xCursor = gui_get_x_cursor(alignment);

	for(s32 i = 0; i < text.length; ++i)
	{
		Assert(rectCount < rectCapacity && "Too little room for such a long text");

		u8 index = text[i] - Font::firstCharacter;
		Assert(index > 0 && index < Font::characterCount && "Character must be more than null terminator and less than fonts range.");

		v2 glyphStart = {xCursor + gui.font->characters[index].leftSideBearing, gui.panelCursorY};

		// Todo(Leo): also consider glyph's actual height
		v2 glyphSize = {gui.font->characters[index].characterWidth * size, size};

		rects[rectCount] = 
		{
			.position 	= (glyphStart),
			.size 		= (glyphSize),
			.uvPosition = gui.font->characters[index].uvPosition,
			.uvSize 	= gui.font->characters[index].uvSize,
		};

		// rects[rectCount].uvPosition.y += rects[rectCount].uvSize.y;
		// rects[rectCount].uvSize.y /= 2;

		++rectCount;

		f32 advance = size * gui.font->characters[index].advanceWidth;
		xCursor 	+= advance;
		textWidth 	+= advance;
	}


	gui.panelDrawCalls[gui.panelDrawCallsCount] 	= {rectCount, rects, gui.font->atlasTexture, color, alignment};
	gui.panelDrawCallsCount 						+= 1;

	gui.panelSize.x 			= f32_max(gui.panelSize.x, textWidth + gui.padding);
	gui.panelCurrentLineLength 	+= textWidth;
	xCursor 					+= gui.padding;

	// return textWidth;
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
	gui.panelDrawCalls 			= push_memory<GuiDrawCall>(*global_transientMemory, gui.panelDrawCallsCapacity, ALLOC_GARBAGE);

	// gui.panelCursorX = 0;
	gui.panelLeftCursorX = 0;
	gui.panelCenterCursorX = 0;
	gui.panelRightCursorX = 0;

	gui.panelCursorY = 0;
	gui.panelSize 	= {gui.padding, gui.padding};

	if (label == GUI_NO_TITLE)
	{
		gui.panelHasTitle = false;
	}
	else
	{
		gui_panel_add_text_draw_call(from_cstring(label), gui.textColor, GUI_ALIGN_LEFT);

		gui.panelHasTitle 	= true;
		gui.panelLeftCursorX= 0;
		gui.panelCursorY 	+= gui.spacing + gui.textSize + gui.spacing + gui.spacing;
		gui.panelSize.y 	+= gui.spacing + gui.textSize + gui.spacing + gui.spacing;
	}

	gui.panelCurrentLineLength 	= 0;
	gui.panelMaxLineLength 		= 0;
	gui.drawCallCountOnLineBegin = 0;
}

internal void gui_end_panel()
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive == true);
	Assert(global_transientMemory->checkpoint == 0);

	gui.isPanelActive 		= false;

	f32 drawAreaWidth = gui.panelMaxLineLength;

	v2 panelSize 	= gui.panelSize;
	panelSize.x 	= drawAreaWidth + 2 * gui.padding;
	panelSize.y 	+= gui.padding - gui.spacing;


	gui.panelSizeLastFrame 	= panelSize;

	v2 screenSpacePanelPosition 		= gui_transform_screen_point(gui.panelPosition);
	v2 screenSpacePanelTopLeftOffset 	= screenSpacePanelPosition - v2{-1,-1};
	v2 screenSpacePanelTopRightOffset 	= screenSpacePanelPosition - v2{1,-1};

	v2 pixelSpaceTopLeft 	= gui.panelPosition + v2{gui.padding, gui.padding};
	v2 pixelSpaceTopRight 	= gui.panelPosition + v2 {panelSize.x - gui.padding, gui.padding}; 

	ScreenRect panelRects [] =
	{
		{screenSpacePanelPosition,	gui_transform_screen_size(panelSize),	{0,0},	{1,1}},
		// Todo(Leo): its weird to add only half padding on other side, do something about that
		{screenSpacePanelPosition,	gui_transform_screen_size({panelSize.x, gui.padding + gui.textSize + gui.padding * 0.5f}), {0,0},	{1,1}},
	};

	// Note(Leo): If we do have title, we draw another rect to highlight it. (It becomes darker, since total alpha is more)
	s32 backrgroundDrawCount = gui.panelHasTitle ? 2 : 1;

	graphics_draw_screen_rects(platformGraphics, backrgroundDrawCount, panelRects, gui.panelTexture, gui.panelColor);

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
					// Note(Leo): Rect is stretched and its x size is used to shorten it width-wise
					rect.position.x = pixelSpaceTopLeft.x + rect.size.x;
					rect.position.y += pixelSpaceTopLeft.y;
					rect.size.x 	= drawAreaWidth - (2 * rect.size.x);
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
		graphics_draw_screen_rects(platformGraphics, call.count, call.rects, call.material, call.color);
	}
}

internal bool gui_button(char const * label)
{
	Gui & gui 		= *global_currentGui;
	bool isSelected = gui_is_selected();

	Assert(gui.isPanelActive);
	gui_panel_add_text_draw_call(from_cstring(label), isSelected ? gui.selectedTextColor : gui.textColor, GUI_ALIGN_LEFT);

	gui_panel_new_line();

	bool result = isSelected && gui.event_confirm;
	return result;
}

internal bool gui_toggle(char const * label, bool32 * value)
{
	constexpr String offString 	= from_cstring("OFF");
	constexpr String onString 	= from_cstring("ON");

	Gui & gui 		= *global_currentGui;
	Assert(gui.isPanelActive);

	bool isSelected = gui_is_selected();
	bool modified 	= false;

	if (isSelected && gui.event_confirm)
	{
		*value 		= !*value;
		modified 	= true;
	}

	String labelString = push_temp_string(128);
	string_append_format(labelString, 128, label, ": ");

	String const & valueString 	= *value ? onString : offString;
	v4 colour 					= isSelected ? gui.selectedTextColor : gui.textColor;

	gui_panel_add_text_draw_call(labelString, colour, GUI_ALIGN_LEFT);
	gui_panel_add_text_draw_call(valueString, colour, GUI_ALIGN_RIGHT);

	gui_panel_new_line();

	return modified;
}

internal void gui_text(char const * text)
{
	Gui & gui = *global_currentGui;

	Assert (gui.isPanelActive);
	gui_panel_add_text_draw_call(from_cstring(text), gui.textColor, GUI_ALIGN_LEFT);

	gui_panel_new_line();
}

internal void gui_image(MaterialHandle material, v2 size, v4 colour)
{	
	Gui & gui = *global_currentGui;

	f32 yMovement = size.y;

	v2 position = gui_transform_screen_point(gui.currentPosition);
	size = gui_transform_screen_size(size);

	gui.currentPosition.y += yMovement + gui.padding;

	ScreenRect rect = {position, size, {0, 0}, {1, 1}};

	// if (material == GUI_NO_TEXTURE)
	// {
	// 	material = gui.panelTexture;
	// }

	graphics_draw_screen_rects(platformGraphics, 1, &rect, material, colour);
}

internal bool gui_float_slider(char const * label, f32 * value, f32 minValue, f32 maxValue)
{
	Gui & gui = *global_currentGui;

	Assert (gui.isPanelActive);

	constexpr f32 sliderMoveSpeed 	= 1.0;
	bool isSelected 				= gui_is_selected();

	bool modified 			= false;
	bool modifiedByMouse 	= isSelected && input_button_is_down(gui.input, InputButton_mouse_0);

	if (isSelected)
	{
		if (input_button_is_down(gui.input, InputButton_dpad_left))
		{
			*value 		-= sliderMoveSpeed * gui.elapsedTime * (maxValue - minValue);
			modified 	= true;
		}

		if (input_button_is_down(gui.input, InputButton_dpad_right))
		{
			*value 		+= sliderMoveSpeed * gui.elapsedTime * (maxValue - minValue);
			modified 	= true;
		}
	}

	f32 yPosition 	= gui.panelCursorY;
	v4 colour 		= isSelected ? gui.selectedTextColor : gui.textColor;
	gui_label(label, colour);

	*value = f32_clamp(*value, minValue, maxValue);

	String valueFormat = push_temp_string_format(16, *value);
	f32 backupPanelLineLength = gui.panelCurrentLineLength;
	gui_panel_add_text_draw_call(valueFormat, {0,0,0,1}, GUI_ALIGN_RIGHT);
	gui.panelCurrentLineLength = backupPanelLineLength;

	f32 rightCursorOffset = gui.panelRightCursorX;

	constexpr float sliderWidth = 150;
	float sliderHeight = gui.textSize;

	float handleWidth 	= gui.textSize * (5.0f / 8.0f);
	float handleHeight 	= gui.textSize;

	float sliderRectX = -sliderWidth + rightCursorOffset;
	float sliderRectY = yPosition + (gui.textSize / 2) - (sliderHeight / 2);

	float handlePositionTime = (*value - minValue) / (maxValue - minValue); 

	if (modifiedByMouse)
	{
		f32 x = input_cursor_get_position(gui.input).x / gui.screenSizeRatio;
		x -= (sliderRectX + handleWidth / 2) + gui.panelSizeLastFrame.x - sliderWidth;
		x /= (sliderWidth - handleWidth);

		handlePositionTime = x;
		handlePositionTime = f32_clamp(handlePositionTime, 0, 1);

		*value = f32_lerp(minValue, maxValue, handlePositionTime);
	}

	float handleRectX = (sliderWidth - handleWidth) * handlePositionTime - sliderWidth + rightCursorOffset;
	float handleRectY = yPosition + (gui.textSize / 2) - (handleHeight / 2);

	ScreenRect sliderRect =
	{ 
		{sliderRectX, sliderRectY},
		{sliderWidth, sliderHeight},
		{0, 0},
		{1, 1},
	};

	ScreenRect handleRect =
	{
		{handleRectX, handleRectY},
		{handleWidth, handleHeight},
		{0, 0},
		{1, 1}
	};

	v4 railColour = colour_multiply({0.3, 0.3, 0.3, 0.6}, isSelected ? gui.selectedTextColor : gui.textColor);
	v4 handleColour = colour_multiply({0.8, 0.8, 0.8, 1.0}, isSelected ? gui.selectedTextColor : gui.textColor);

	ScreenRect * rects = push_memory<ScreenRect>(*global_transientMemory, 2, ALLOC_GARBAGE);
	rects[0] = sliderRect;
	rects[1] = handleRect;
	
	// Todo(Leo): This is totally fukin häxör :D
	gui.panelDrawCalls[gui.panelDrawCallsCount + 1] = gui.panelDrawCalls[gui.panelDrawCallsCount - 1];
	gui.panelDrawCalls[gui.panelDrawCallsCount - 1] = {1, &rects[0], gui.panelTexture, railColour, GUI_ALIGN_RIGHT };
	gui.panelDrawCalls[gui.panelDrawCallsCount] 	= {1, &rects[1]	, gui.panelTexture, handleColour, GUI_ALIGN_RIGHT };
	gui.panelDrawCallsCount += 2;


	gui_push_line_width(sliderWidth);
	gui_panel_new_line();
	
	return modified;
}

internal bool gui_float_field(char const * label, f32 * value, GuiClampValuesF32 clamp)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive);

	constexpr f32 sliderMoveSpeed 	= 1.0;
	bool isSelected 				= gui_is_selected();

	bool modified 			= false;
	bool modifiedByMouse 	= isSelected && input_button_is_down(gui.input, InputButton_mouse_0);

	if (isSelected)
	{
		if (input_button_is_down(gui.input, InputButton_dpad_left))
		{
			*value 		-= sliderMoveSpeed * gui.elapsedTime;
			modified 	= true;
		}

		if (input_button_is_down(gui.input, InputButton_dpad_right))
		{
			*value 		+= sliderMoveSpeed * gui.elapsedTime;
			modified 	= true;
		}

		float joystickDelta = input_axis_get_value(gui.input, InputAxis_move_x);
		if (abs_f32(joystickDelta) >= 0.1)
		{
			// Note(Leo): pow3 provides more accuracy closer to zero and preserves sign
			joystickDelta 	= joystickDelta*joystickDelta*joystickDelta;
			*value 			+= joystickDelta * sliderMoveSpeed;
			modified 		= true; 
		}
	}
	
	constexpr f32 mouseMoveScale = 0.2;
	if (modifiedByMouse)
	{
		f32 mouseDelta 	= input_cursor_get_position(gui.input).x - gui.mousePositionLastFrame.x;
		mouseDelta 		= mouseMoveScale * mouseDelta * mouseDelta;
		*value 			+= mouseDelta;
	}

	*value = f32_clamp(*value, clamp.min, clamp.max);

	v4 colour = isSelected ? gui.selectedTextColor : gui.textColor;

	gui_label(label, colour);

	// string_append_format(string, 128, label, ":");
	// gui_panel_add_text_draw_call(string, isSelected ? gui.selectedTextColor : gui.textColor, GUI_ALIGN_LEFT);

	String string = push_temp_string(128);
	// reset_string(string, 128);
	string_append_f32(string, 128, *value);

	// f32 backUpCursorX = gui.panelCursorX;
	// gui.panelCursorX = 0;
	gui_panel_add_text_draw_call(string, colour, GUI_ALIGN_RIGHT);
	// gui.panelCursorX = backUpCursorX;

	// Todo(Leo): expand panel width

	gui_panel_new_line();

	return modified;	
}


internal bool gui_int_field(char const * label, s32 * value, GuiClampValuesS32 clamp)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive);

	bool isSelected 				= gui_is_selected();

	bool modified 			= false;
	// bool modifiedByMouse 	= isSelected && is_pressed(gui.input->mouse0);

	if (isSelected)
	{
		if (input_button_went_down(gui.input, InputButton_dpad_left))
		{
			*value 		-= 1;
			modified 	= true;
		}

		if (input_button_went_down(gui.input, InputButton_dpad_right))
		{
			*value 		+= 1;
			modified 	= true;
		}
	}
	*value = s32_clamp(*value, clamp.min, clamp.max);


	v4 colour = isSelected ? gui.selectedTextColor : gui.textColor;

	// string_append_format(string, 128, label, ":");
	// gui_panel_add_text_draw_call(string, colour, GUI_ALIGN_LEFT);

	gui_label(label, colour);

	String string = push_temp_string(128);
	// reset_string(string, 128);
	string_append_s32(string, 128, *value);

	// f32 backUpCursorX = gui.panelCursorX;
	// gui.panelCursorX = 0;
	gui_panel_add_text_draw_call(string, colour, GUI_ALIGN_RIGHT);
	// gui.panelCursorX = backUpCursorX;

	// Todo(Leo): expand panel width

	gui_panel_new_line();

	return modified;	
}

internal bool gui_float_value_field(char const * label, f32 * value, GuiClampValuesF32 clamp, GuiAlignment alignment)
{
	Gui & gui = *global_currentGui;

	Assert(gui.isPanelActive);

	constexpr f32 sliderMoveSpeed 	= 1.0;
	bool isSelected 				= gui_is_selected();

	bool modified 			= false;
	bool modifiedByMouse 	= isSelected && input_button_is_down(gui.input, InputButton_mouse_0);

	if (isSelected)
	{
		if (input_button_is_down(gui.input, InputButton_dpad_left))
		{
			*value 		-= sliderMoveSpeed * gui.elapsedTime;
			modified 	= true;
		}

		if (input_button_is_down(gui.input, InputButton_dpad_right))
		{
			*value 		+= sliderMoveSpeed * gui.elapsedTime;
			modified 	= true;
		}

		float joystickDelta = input_axis_get_value(gui.input, InputAxis_move_x);
		if (abs_f32(joystickDelta) >= 0.1)
		{
			// Note(Leo): pow3 provides more accuracy closer to zero and preserves sign
			joystickDelta 	= joystickDelta*joystickDelta*joystickDelta;
			*value 			+= joystickDelta * sliderMoveSpeed;
			modified 		= true; 
		}
	}
	
	constexpr f32 mouseMoveScale = 0.2;
	if (modifiedByMouse)
	{
		f32 mouseDelta 	= input_cursor_get_position(gui.input).x - gui.mousePositionLastFrame.x;
		mouseDelta 		= mouseMoveScale * mouseDelta * mouseDelta;
		*value 			+= mouseDelta;
	}

	*value = f32_clamp(*value, clamp.min, clamp.max);

	v4 colour = isSelected ? gui.selectedTextColor : gui.textColor;

	s32 capacity = 16;
	String string = push_temp_string(capacity);
	string_append_format(string, capacity, label, ": ");
	string_append_f32(string, capacity, *value);

	// f32 backUpCursorX = gui.panelCursorX;
	// gui.panelCursorX = 0;
	gui_panel_add_text_draw_call(string, colour, alignment);
	// gui.panelCursorX = backUpCursorX;

	// Todo(Leo): expand panel width


	return modified;	
}

internal bool gui_vector_2_field(char const * label, v2 * value, GuiClampValuesV2 const & clamp)
{
	gui_text(label);

	// Note(Leo): this must not short-circuit, or y field is not drawn that frame :)
	bool xModified 	= gui_float_field("X", &value->x, {clamp.min.x, clamp.max.x});
	bool yModified 	= gui_float_field("Y", &value->y, {clamp.min.y, clamp.max.y});
	
	return xModified || yModified;
}



internal bool gui_colour_rgb(char const * label, v3 * color, GuiColorFlags flags)
{
	Gui & gui = *global_currentGui;
	
	// gui_text(label);
	// String labelString = push_temp_string_format(64, label, ": ")
	// gui_panel_add_text_draw_call(from_cstring(label), gui.textColor, GUI_ALIGN_LEFT);
	// // gui_panel_new_line();
	gui_label(label, gui.textColor);

	
	bool hdr = (flags & GUI_COLOR_FLAGS_HDR) != 0;
	f32 max = hdr ? 10 : 1;
	GuiClampValuesF32 clamp = {.min = 0, .max = max};

	// f32 backUpCursorX = gui.panelCursorX;
	// gui.panelCursorX = 0;
	bool modified 	= gui_float_value_field("R", &color->r, clamp, GUI_ALIGN_RIGHT);
	modified 		= gui_float_value_field("G", &color->g, clamp, GUI_ALIGN_RIGHT) || modified;
	modified 		= gui_float_value_field("B", &color->b, clamp, GUI_ALIGN_RIGHT) || modified;
	// gui.panelCursorX = backUpCursorX;

	gui_panel_new_line();


	// Todo(Leo): Add this to rgba version of this function
	// if ((flags & GUI_COLOR_FLAGS_ALPHA) != 0)
	// {
	// 	modified 		= gui_float_slider_2("A:", &color->a, 0, 1) || modified;
	// }

	v4 displayColor;
	displayColor.rgb 	= *color;
	displayColor.a 		= 1;


	ScreenRect * rect = push_memory<ScreenRect>(*global_transientMemory, 2, ALLOC_GARBAGE);

	v2 position = {0, gui.panelCursorY};

	rect[0] = 
	{
		.position 	= position,
		.size 		= {0, gui.textSize},
		.uvPosition = {0,0},
		.uvSize 	= {1,1},
	};

	rect[1] = 
	{
		.position 	= position + v2{gui.lineWidth, gui.lineWidth},
		.size 		= {gui.lineWidth, gui.textSize - 2 * gui.lineWidth},
		.uvPosition = {0,0},
		.uvSize 	= {1,1},
	};

	gui.panelDrawCalls[gui.panelDrawCallsCount++] 	= {1, &rect[0], gui.panelTexture, gui.lineColor, GUI_ALIGN_HORIZONTAL_STRETCH};
	gui.panelDrawCalls[gui.panelDrawCallsCount++] 	= {1, &rect[1], gui.panelTexture, displayColor, GUI_ALIGN_HORIZONTAL_STRETCH};

	gui.panelCursorY 	+= gui.textSize + gui.spacing;
	gui.panelSize.y 	+= gui.textSize + gui.spacing;

	return modified;
}

internal void gui_background_image(MaterialHandle material, s32 rows, v4 colour)
{	
	Gui & gui = *global_currentGui;

	v2 position = gui.currentPosition - v2{gui.padding, gui.padding};
	position = gui_transform_screen_point(position);

	f32 height = rows * gui.textSize + (rows - 1) * gui.padding;
	v2 size = {height, height};
	size += {2* gui.padding, 2*gui.padding};
	size = gui_transform_screen_size(size);

	ScreenRect rect = {position, size, {0, 0}, {1, 1}};

	graphics_draw_screen_rects(platformGraphics, 1, &rect, material, colour);
}

internal void gui_line()
{
	Gui & gui = *global_currentGui;
	Assert(gui.isPanelActive);

	// Todo(Leo): allocate this to gui in the beninnginnein
	ScreenRect * rect = push_memory<ScreenRect>(*global_transientMemory, 1, ALLOC_GARBAGE);

	v2 position = {0, gui.panelCursorY};
	position.y += gui.padding;

	*rect = 
	{
		.position 	= position,
		.size 		= {0, gui.lineWidth},
		.uvPosition = {0,0},
		.uvSize 	= {1,1},
	};

	gui.panelDrawCalls[gui.panelDrawCallsCount] 	= {1, rect, gui.panelTexture, gui.lineColor, GUI_ALIGN_HORIZONTAL_STRETCH};
	gui.panelDrawCallsCount 						+= 1;

	gui.panelCursorY 	+= 2 * gui.padding + gui.lineWidth;
	gui.panelSize.y 	+= 2 * gui.padding + gui.lineWidth;
}


internal bool gui_is_under_mouse(v2 position, v2 size)
{
	position 	= position * global_currentGui->screenSizeRatio;
	size 		= size * global_currentGui->screenSizeRatio;

	v2 mousePosition 	= input_cursor_get_position(global_currentGui->input);
	mousePosition 		-= position;

	bool isUnderMouse = mousePosition.x > 0 && mousePosition.x < size.x && mousePosition.y > 0 && mousePosition.y < size.y;
	return isUnderMouse;
}

internal bool gui_is_selected()
{
	Gui & gui = *global_currentGui;

	// Todo(Leo): do some kind of "current rect" thing, or pass it here
	bool isUnderMouse 	= false;//gui_is_under_mouse(gui.panelPosition + v2{gui.padding, gui.padding} + v2{gui.panelCursorX, gui.panelCursorY}, {400, gui.textSize});// || isSelected;
	bool mouseHasMoved 	= v2_length(gui.mousePositionLastFrame - input_cursor_get_position(gui.input)) > 0.1f;

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



	if (gui.panelDrawCallsCount > gui.drawCallCountOnLineBegin)
	{
		for (s32 callIndex = gui.drawCallCountOnLineBegin; callIndex < gui.panelDrawCallsCount; ++callIndex)
		{
			auto & call = gui.panelDrawCalls[callIndex];

			if (call.alignment == GUI_ALIGN_RIGHT)
			{

				for(s32 rectIndex = 0; rectIndex < call.count; ++rectIndex)
				{
					ScreenRect & rect = call.rects[rectIndex];
					rect.position.x -= gui.panelRightCursorX;
				}
			}
			
		}
	}
	gui.drawCallCountOnLineBegin = gui.panelDrawCallsCount;




	// f32 lineLength = gui.panelLeftCursorX + gui.panelCenterCursorX + gui.panelRightCursorX;

	// gui.panelMaxLineLength 		= f32_max(lineLength, gui.panelMaxLineLength);
	gui.panelMaxLineLength 		= f32_max(gui.panelCurrentLineLength, gui.panelMaxLineLength);
	gui.panelCurrentLineLength 	= 0;

	// gui.panelCursorX 	= 0;
	gui.panelLeftCursorX = 0;
	gui.panelCenterCursorX = 0;
	gui.panelRightCursorX = 0;

	gui.panelCursorY 	+= gui.textSize + gui.spacing;
	gui.panelSize.y 	+= gui.textSize + gui.spacing;

	gui.panelLeftCursorX 	= 0;
	gui.panelRightCursorX 	= 0;
	gui.panelCenterCursorX 	= 0;



}

internal void gui_push_line_width(f32 width)
{
	Gui & gui = *global_currentGui;

	gui.panelCurrentLineLength += width;
}

internal void gui_label(char const * label, v4 colour)
{
	String string = push_temp_string_format(128, label, ":");
	gui_panel_add_text_draw_call(string, colour, GUI_ALIGN_LEFT);
}