internal Gui init_game_gui(GameAssets & assets)
{
	Gui gui = {};

	gui 					= {};
	gui.textSize 			= 20;
	gui.textColor 			= colour_white;
	gui.selectedTextColor 	= colour_bright_green;
	gui.spacing 			= 5;
	gui.padding 			= 10;
	gui.font 				= assets_get_font(assets, FontAssetId_game);

	u32 guiTexturePixelColor 			= 0xffffffff;
	TextureAssetData guiTextureAsset 	= make_texture_asset(&guiTexturePixelColor, 1, 1, 4);
	TextureHandle guiTextureHandle		= graphics_memory_push_texture(platformGraphics, &guiTextureAsset);
	gui.panelTexture					= graphics_memory_push_material(platformGraphics, GraphicsPipeline_screen_gui, 1, &guiTextureHandle);

	return gui;
}

internal void game_gui_set_cursor_visible(bool menuVisible)
{
	bool windowIsFullscreen = platform_window_is_fullscreen(platformWindow);
	bool cursorVisible 		= !windowIsFullscreen || menuVisible;
	platform_window_set_cursor_visible(platformWindow, cursorVisible);
}

internal bool32 game_gui_menu_visible(Game * game)
{
	// log_debug(FILE_ADDRESS, game->menuStateIndex);
	bool32 result = (game->menuStateIndex > 0);
	return result;
}

internal void game_gui_push_menu (Game * game, MenuView view)
{
	Assert(game->menuStateIndex < array_count(game->menuStates));

	if (game->menuStateIndex == 0)
	{
		game_gui_set_cursor_visible(true);
	}

	game->menuStates[game->menuStateIndex].selectedIndex = game->gui.selectedIndex;
	game->menuStateIndex += 1;
	game->menuStates[game->menuStateIndex].view = view;


	gui_reset_selection();
}

internal void game_gui_pop_menu (Game * game)
{
	Assert(game->menuStateIndex >= 0);

	game->menuStateIndex -= 1;
	game->gui.selectedIndex = game->menuStates[game->menuStateIndex].selectedIndex;

	if (game->menuStateIndex == 0)
	{
		game_gui_set_cursor_visible(false);
	}
}

bool32 do_gui(Game * game, PlatformInput * input)
{	
	if (game_gui_menu_visible(game) == false)
	{
		// Note(Leo): return whether or not we want to keep game alive
		return true;
	}

	// Note(Leo): idk, testing alternative convention
	constexpr v2 corner_position = {30, 30};
	constexpr v2 center_position = {850, 400};
	v4 menuColor = {};

	bool32 event_escape = input_button_went_down(input, InputButton_nintendo_b)
							|| input_button_went_down(input, InputButton_keyboard_backspace);

	if (event_escape)
	{
		game_gui_pop_menu(game);
	}

	bool32 keepScene = true;
	
	switch(game->menuStates[game->menuStateIndex].view)
	{	
		case MenuView_off:
			// Nothing to do
			break;

		case MenuView_confirm_exit:
		{
			gui_position(corner_position);

			gui_start_panel("Exit to Main Menu?", menuColor);

			if (gui_button("Yes"))
			{
				keepScene = false;
			}

			if (gui_button("No"))
			{
				game_gui_pop_menu(game);
				// gui_reset_selection();
			}

			gui_end_panel();
		} break;

		case MenuView_main:
		{
			gui_position(corner_position);	

			gui_start_panel("MENU", menuColor);

			v2 mousePosition = input_cursor_get_position(input);

			if (gui_button("Continue"))
			{
				game_gui_pop_menu(game);
				game_gui_set_cursor_visible(false);
			}

			char const * const cameraModeLabels [] =
			{
				"Camera Mode: Player", 
				"Camera Mode: Free",
				"Camera Mode: Mouse",
			};
			
			if (gui_button(cameraModeLabels[game->cameraMode]))
			{
				game->cameraMode = (CameraMode)((game->cameraMode + 1) % CameraModeCount);
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
				game_gui_push_menu(game, MenuView_edit_sky);
				// gui_reset_selection();
			}

			if (gui_button("Edit Mesh Generation"))
			{
				game_gui_push_menu(game, MenuView_edit_mesh_generation);
				// gui_reset_selection();
			}

			if (gui_button("Edit Trees"))
			{
				game_gui_push_menu(game, MenuView_edit_tree);
				// gui_reset_selection();
			}

			if (gui_button("Edit Monuments"))
			{
				game_gui_push_menu(game, MenuView_edit_monuments);
				// gui_reset_selection();
			}

			if (gui_button("Edit Camera"))
			{
				game_gui_push_menu(game, MenuView_edit_camera);
				// gui_reset_selection();
			}

			if (gui_button("Edit Clouds"))
			{
				game_gui_push_menu(game, MenuView_edit_clouds);
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

				write_settings_file(file, *game);

				platform_file_close(file);
			}

			if (gui_button("Play Sound 1"))
			{
				game->audioClipsOnPlay.push({game->stepSFX, 0, random_range(0.8, 1.2)});
			}

			if (gui_button("Play Sound 2"))
			{
				game->audioClipsOnPlay.push({game->stepSFX2, 0, random_range(0.8, 1.2)});
			}

			if (gui_button("Play Sound 3"))
			{
				game->audioClipsOnPlay.push({game->stepSFX3, 0, random_range(0.8, 1.2)});
			}

			if (gui_button("Exit Scene"))
			{
				game_gui_push_menu(game, MenuView_confirm_exit);
				// gui_reset_selection();
			}


			gui_end_panel();
		} break;

		case MenuView_edit_mesh_generation:
		{
			gui_position(corner_position);	

			gui_start_panel("EDIT MESH GENERATION", menuColor);
			gui_float_slider("Grid Scale", &game->metaballGridScale, 0.2, 1);
			gui_toggle("Draw", &game->drawMCStuff);

			if (gui_button("Back") || event_escape)
			{
				game_gui_pop_menu(game);
				// gui_reset_selection();
			}

			gui_end_panel();
		} break;
	
		case MenuView_edit_tree:
		{
			gui_position(corner_position);	
			gui_start_panel("EDIT TREES", menuColor);
		
			gui_int_field("Tree Index", &game->inspectedTreeIndex, {.min = 0, .max = (s32)game->trees.count - 1});	

			tree_gui(game->trees[game->inspectedTreeIndex]);
			
			gui_line();
			if (gui_button("Back") || event_escape)
			{
				game_gui_pop_menu(game);
				// gui_reset_selection();
			}
			gui_end_panel();

		} break;

		case MenuView_save_complete:
		{
			gui_position(center_position);
			gui_start_panel("Game Saved!", menuColor);

			if (gui_button("Ok"))
			{
				game_gui_pop_menu(game);
				// gui_reset_selection();
			}

			gui_end_panel();

		} break;

		case MenuView_confirm_teleport:
		{
			gui_position(corner_position);

			gui_start_panel("Teleport Player Here?", menuColor);

			if (gui_button("Yes"))
			{
				game->playerCharacterTransform.position = game->freeCamera.position;
				game_gui_pop_menu(game);
				game->cameraMode = CameraMode_player;
			}

			if (gui_button("No"))
			{
				game_gui_pop_menu(game);
			}

			gui_end_panel();
		} break;

		case MenuView_edit_sky:
		{
			gui_position(corner_position);
			gui_start_panel("EDIT SKY", menuColor);

			sky_gui(game->skySettings);

			gui_line();
			if (gui_button("Back") || event_escape)
			{
				game_gui_pop_menu(game);
			}
			gui_end_panel();
		} break;

		case MenuView_spawn:
		{
			gui_position(center_position);
			gui_start_panel("SPAWN", menuColor);

			if (gui_button("10 Waters"))
			{
				game_spawn_water(*game, 10);
			}

			if (gui_button("1 Tree"))
			{
				game_spawn_tree_on_player(*game);
			}

			if (event_escape)
			{
				game_gui_pop_menu(game);
			}

			gui_end_panel();
		} break;

		case MenuView_edit_monuments:
		{
			gui_position(corner_position);
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
					game->monuments.transforms[i].rotation 	= quaternion_axis_angle(v3_up, to_radians(rotation));
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

			if (gui_button("Back") || event_escape)
			{
				game_gui_pop_menu(game);
			}

			gui_end_panel();

		} break;

		case MenuView_edit_clouds:
		{
			gui_position(corner_position);
			clouds_menu(game->clouds);
			if (event_escape)
			{
				game_gui_pop_menu(game);
			};
		} break;

		case MenuView_edit_camera:
		{
			gui_position(corner_position);
			gui_start_panel("EDIT CAMERA", menuColor);

			gui_float_field("Distance", &game->playerCamera.distance);
			gui_float_field("Base Offset", &game->playerCamera.baseOffset);
			gui_float_field("Gamepad Rotate Speed", &game->playerCamera.gamepadRotateSpeed);
			gui_float_field("Mouse Rotate Speed", &game->playerCamera.mouseRotateSpeed);

			gui_line();
			if (gui_button("Back") || event_escape)
			{
				game_gui_pop_menu(game);
			}
			gui_end_panel();
		};
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
		gui_image(GRAPHICS_RESOURCE_SHADOWMAP_GUI_MATERIAL, {300, 300});
	}


	return keepScene;
}