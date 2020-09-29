internal Gui init_game_gui()
{
	Gui gui = {};

	gui 					= {};
	gui.textSize 			= 20;
	gui.textColor 			= colour_white;
	gui.selectedTextColor 	= colour_muted_red;
	gui.spacing 			= 5;
	gui.padding 			= 10;
	gui.font 				= load_font("assets/SourceCodePro-Regular.ttf");

	u32 guiTexturePixelColor 		= 0xffffffff;
	TextureAssetData guiTextureAsset 	= make_texture_asset(&guiTexturePixelColor, 1, 1, 4);
	gui.panelTexture				= graphics_memory_push_gui_texture(platformGraphics, &guiTextureAsset);

	return gui;
}

internal void gui_set_cursor_visible(bool menuVisible)
{
	bool windowIsFullscreen = platform_window_is_fullscreen(platformWindow);
	bool cursorVisible = !windowIsFullscreen || menuVisible;
	platform_window_set_cursor_visible(platformWindow, cursorVisible);

	log_application(1, "Set cursor visible: ", (cursorVisible ? "True" : "False"));
}


bool32 do_gui(Game * game, PlatformInput const & input)
{	
	constexpr v2 cornerPosition = {30, 30};
	constexpr v2 centerPosition = {850, 400};


	if (is_clicked(input.start))
	{
		if (game->menuView == MENU_OFF)
		{
			game->menuView = MENU_MAIN;
			gui_reset_selection();
			gui_set_cursor_visible(true);
		}
		else if (game->menuView == MENU_MAIN)
		{
			game->menuView = MENU_OFF;
			gui_set_cursor_visible(false);
		}
	}

	bool32 keepScene = true;
	
	v4 menuColor = colour_rgb_alpha(colour_bright_blue.rgb, 0.5);
	switch(game->menuView)
	{	
		case MENU_OFF:
			// Nothing to do
			break;

		case MENU_CONFIRM_EXIT:
		{
			gui_position(cornerPosition);

			gui_start_panel("Exit to Main Menu?", menuColor);

			if (gui_button("Yes"))
			{
				keepScene = false;
			}

			if (gui_button("No"))
			{
				game->menuView = MENU_MAIN;
				gui_reset_selection();
			}

			gui_end_panel();
		} break;

		case MENU_MAIN:
		{
			gui_position(cornerPosition);	

			gui_start_panel("MENU", menuColor);

			v2 mousePosition = input.mousePosition;

			// gui_float_slider_2("X", &mousePosition.x, -20000, 20000);
			// gui_float_slider_2("Y", &mousePosition.y, -20000, 20000);

			if (gui_button("Continue"))
			{
				game->menuView = MENU_OFF;
				gui_set_cursor_visible(false);
			}

			char const * const cameraModeLabels [] =
			{
				"Camera Mode: Player", 
				"Camera Mode: Free",
				"Camera Mode: Mouse",
			};
			
			if (gui_button(cameraModeLabels[game->cameraMode]))
			{
				game->cameraMode = (CameraMode)((game->cameraMode + 1) % CAMERA_MODE_COUNT);
			}

			char const * const debugLevelButtonLabels [] =
			{
				"Debug Level: Off",
				"Debug Level: Player",
				"Debug Level: Player, NPC",
				"Debug Level: Player, NPC, Background"
			};

			if(gui_button(debugLevelButtonLabels[global_debugLevel]))
			{
				global_debugLevel += 1;
				global_debugLevel %= DEBUG_LEVEL_COUNT;
			}

			gui_toggle("Debug Shadow", &game->drawDebugShadowTexture);

			if (gui_button("Reload Shaders"))
			{
				graphics_development_reload_shaders(platformGraphics);
			}

			char const * const timeScaleLabels [game->timeScaleCount] =
			{
				"Time Scale: 1.0",
				"Time Scale: 0.1",
				"Time Scale: 0.5",
			};

			if (gui_button(timeScaleLabels[game->timeScaleIndex]))
			{
				game->timeScaleIndex += 1;
				game->timeScaleIndex %= game->timeScaleCount;
			}

			if (gui_button("Edit Sky"))
			{
				game->menuView = MENU_EDIT_SKY;
				gui_reset_selection();
			}

			if (gui_button("Edit Mesh Generation"))
			{
				game->menuView = MENU_EDIT_MESH_GENERATION;
				gui_reset_selection();
			}

			if (gui_button("Edit Trees"))
			{
				game->menuView = MENU_EDIT_TREE;
				gui_reset_selection();
			}

			if (gui_button("Edit Monuments"))
			{
				game->menuView = MENU_EDIT_MONUMENTS;
				gui_reset_selection();
			}


			if (gui_button("Read Settings"))
			{
				read_settings_file(game_get_serialized_objects(*game));
				
				read_settings_file(monuments_get_serialized_objects(game->monuments));
				monuments_refresh_transforms(game->monuments, game->collisionSystem);
			}

			if (gui_button("Write Settings"))
			{
				PlatformFileHandle file = platform_file_open("settings", FILE_MODE_WRITE);

				write_settings_file(file, game_get_serialized_objects(*game));
				write_settings_file(file, monuments_get_serialized_objects(game->monuments));

				platform_file_close(file);
			}

			if (gui_button("Exit Scene"))
			{
				game->menuView = MENU_CONFIRM_EXIT;
				gui_reset_selection();
			}


			gui_end_panel();
		} break;

		case MENU_EDIT_MESH_GENERATION:
		{
			gui_position(cornerPosition);	

			gui_start_panel("EDIT MESH GENERATION", menuColor);
			gui_float_slider("Grid Scale", &game->metaballGridScale, 0.2, 1);
			gui_toggle("Draw", &game->drawMCStuff);

			if (gui_button("Back") || is_clicked(input.start))
			{
				game->menuView = MENU_MAIN;
				gui_reset_selection();
			}

			gui_end_panel();
		} break;
	
		case MENU_EDIT_TREE:
		{
			gui_position(cornerPosition);	
			gui_start_panel("EDIT TREES", menuColor);
		
			gui_int_field("Tree Index", &game->inspectedTreeIndex, {.min = 0, .max = (s32)game->trees.count - 1});	

			tree_gui(game->trees[game->inspectedTreeIndex]);
			
			gui_line();
			if (gui_button("Back") || is_clicked(input.start))
			{
				game->menuView = MENU_MAIN;
				gui_reset_selection();
			}
			gui_end_panel();

		} break;

		case MENU_SAVE_COMPLETE:
		{
			gui_position(centerPosition);
			gui_start_panel("Game Saved!", menuColor);

			if (gui_button("Ok"))
			{
				game->menuView = MENU_MAIN;
				gui_reset_selection();
			}

			gui_end_panel();

		} break;

		case MENU_CONFIRM_TELEPORT:
		{
			gui_position(cornerPosition);

			gui_start_panel("Teleport Player Here?", menuColor);

			if (gui_button("Yes"))
			{
				game->playerCharacterTransform.position = game->freeCamera.position;
				game->menuView = MENU_OFF;
				game->cameraMode = CAMERA_MODE_PLAYER;
			}

			if (gui_button("No"))
			{
				game->menuView = MENU_OFF;
			}

			gui_end_panel();
		} break;

		case MENU_EDIT_SKY:
		{
			SkySettings & settings = game->skySettings;

			gui_position(cornerPosition);

			gui_start_panel("EDIT SKY", menuColor);

			sky_gui(game->skySettings);

			gui_line();

			if (gui_button("Back") || is_clicked(input.start))
			{
				game->menuView = MENU_MAIN;
			}

			gui_end_panel();

		} break;

		case MENU_SPAWN:
		{
			gui_position(centerPosition);
			gui_start_panel("SPAWN", menuColor);

			if (gui_button("10 Waters"))
			{
				game_spawn_water(*game, 10);
			}

			if (gui_button("1 Tree"))
			{
				game_spawn_tree_on_player(*game);
			}

			if (is_clicked(input.start))
			{
				game->menuView = MENU_OFF;
			}

			gui_end_panel();

		} break;

		case MENU_EDIT_MONUMENTS:
		{
			gui_position(cornerPosition);
			gui_start_panel("MONUMENTS", menuColor);

			constexpr char const * labels [] =
			{	
				"Monument 1",
				"Monument 2",
				"Monument 3",
				"Monument 4",
				"Monument 5",
			};

			for (s32 i = 0; i < game->monuments.count; ++i)
			{
				gui_text(labels[i]);

				float rotation = game->monuments.monuments[i].rotation;
				if (gui_float_field("Rotation", &rotation))
				{
					if (rotation > 360)
					{
						rotation = mod_f32(rotation, 360);
					}

					if (rotation < 0)
					{
						rotation = 360 - mod_f32(rotation, 360);
					}
	
					game->monuments.monuments[i].rotation = rotation;
					game->monuments.transforms[i].rotation 	= axis_angle_quaternion(v3_up, to_radians(rotation));
				}

				v2 xy = game->monuments.monuments[i].position;
				if (gui_vector_2_field("Position", &xy))
				{
					v3 position = {xy.x, xy.y, get_terrain_height(game->collisionSystem, xy)};
					game->monuments.monuments[i].position = position.xy;
					game->monuments.transforms[i].position = position;
				}


				FS_DEBUG_ALWAYS
				(
					v3 position = (game->monuments.transforms[i].position + v3{0,0,30});

					v3 right = rotate_v3(game->monuments.transforms[i].rotation, v3_right);
					debug_draw_line(position - right * 300, position + right * 300, colour_bright_red);										

					v3 forward = rotate_v3(game->monuments.transforms[i].rotation, v3_forward);
					debug_draw_line(position - forward * 300, position + forward * 300, colour_bright_red);


					if ((game->gui.selectedIndex / 3) == i)
					{
						debug_draw_circle_xy(game->monuments.transforms[i].position + v3{0,0, 30}, 16, colour_bright_red);
					}
				)


				gui_line();
			}

			float dummy;
			gui_float_field("Dummy", &dummy);

			if (gui_button("Back") || is_clicked(input.start))
			{
				game->menuView = MENU_MAIN;
			}

			gui_end_panel();

		} break;
	}

	// gui_position({100, 100});
	// gui_text("Sphinx of black quartz, judge my vow!");
	// gui_text("Sphinx of black quartz, judge my vow!");

	// gui_pivot(GUI_PIVOT_TOP_RIGHT);
	// gui_position({100, 100});
	// gui_image(shadowTexture, {300, 300});

	gui_position({1550, 30});
	if (game->drawDebugShadowTexture)
	{
		gui_image(GRAPHICS_RESOURCE_SHADOWMAP_GUI_TEXTURE, {300, 300});
	}

	gui_end_frame();

	return keepScene;
}