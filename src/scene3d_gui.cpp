enum MenuView : s32
{
	MENU_OFF,
	MENU_MAIN,
	MENU_CONFIRM_EXIT,
	MENU_CONFIRM_TELEPORT,
	MENU_EDIT_SKY,
	MENU_EDIT_MESH_GENERATION,
	MENU_SAVE_COMPLETE,
};

bool32 do_gui(Scene3d * scene, PlatformInput const & input)
{	
	if (is_clicked(input.start))
	{
		if (scene->menuView == MENU_OFF)
		{
			scene->menuView = MENU_MAIN;
			gui_reset_selection();
		}
		else
		{
			scene->menuView = MENU_OFF;
		}
	}

	bool32 keepScene = true;
	
	v4 menuColor = colour_rgb_alpha(colour_bright_blue.rgb, 0.5);
	switch(scene->menuView)
	{	
		case MENU_OFF:
			// Nothing to do
			break;

		case MENU_CONFIRM_EXIT:
		{
			gui_position({50, 50});

			gui_start_panel("Exit to Main Menu?", menuColor);

			if (gui_button("Yes"))
			{
				keepScene = false;
			}

			if (gui_button("No"))
			{
				scene->menuView = MENU_MAIN;
				gui_reset_selection();
			}

			gui_end_panel();
		} break;

		case MENU_MAIN:
		{
			gui_position({50, 50});	

			gui_start_panel("Menu", menuColor);


			if (gui_button("Continue"))
			{
				scene->menuView = MENU_OFF;
			}

			char const * const cameraModeLabels [] =
			{
				"Camera Mode: Player", 
				"Camera Mode: Free"
			};
			
			if (gui_button(cameraModeLabels[scene->cameraMode]))
			{
				scene->cameraMode += 1;
				scene->cameraMode %= CAMERA_MODE_COUNT;
			}

			char const * const debugLevelButtonLabels [] =
			{
				"Debug Level: Off",
				"Debug Level: Player",
				"Debug Level: Player, NPC",
				"Debug Level: Player, NPC, Background"
			};

			if(gui_button(debugLevelButtonLabels[global_DebugDrawLevel]))
			{
				global_DebugDrawLevel += 1;
				global_DebugDrawLevel %= DEBUG_LEVEL_COUNT;
			}

			char const * const drawDebugShadowLabels [] =
			{
				"Debug Shadow: Off",
				"Debug Shadow: On"
			};

			if (gui_button(drawDebugShadowLabels[scene->drawDebugShadowTexture]))
			{
				scene->drawDebugShadowTexture = !scene->drawDebugShadowTexture;
			}

			if (gui_button("Reload Shaders"))
			{
				platformApi->reload_shaders(platformGraphics);
			}

			char const * const timeScaleLabels [scene->timeScaleCount] =
			{
				"Time Scale: 1.0",
				"Time Scale: 0.1",
				"Time Scale: 0.5",
			};

			if (gui_button(timeScaleLabels[scene->timeScaleIndex]))
			{
				scene->timeScaleIndex += 1;
				scene->timeScaleIndex %= scene->timeScaleCount;
			}

			if (gui_button("Save Game"))
			{
				// Todo(Leo): Show some response
				write_save_file(scene);

				scene->menuView = MENU_SAVE_COMPLETE;
				gui_reset_selection();
			}

			if (gui_button("Edit Sky"))
			{
				scene->menuView = MENU_EDIT_SKY;
				gui_reset_selection();
			}

			if (gui_button("Edit Mesh Generation"))
			{
				scene->menuView = MENU_EDIT_MESH_GENERATION;
				gui_reset_selection();
			}


			if (gui_button("Exit Scene"))
			{
				scene->menuView = MENU_CONFIRM_EXIT;
				gui_reset_selection();
			}

			gui_end_panel();

		} break;

		case MENU_EDIT_MESH_GENERATION:
		{
			gui_position({50, 50});	

			gui_start_panel("Edit Mesh Generation", menuColor);

			scene->metaballGridScale = gui_float_slider("Grid Scale", scene->metaballGridScale, 0.2, 1);

			char const * const drawMCStuffTexts [] = 
			{
				"Draw: Off",
				"Draw: On"
			};

			if (gui_button(drawMCStuffTexts[scene->drawMCStuff]))
			{
				scene->drawMCStuff = !scene->drawMCStuff;
			}

			if (gui_button("Back"))
			{
				scene->menuView = MENU_MAIN;
				gui_reset_selection();
			}

			gui_end_panel();
		} break;
	
		case MENU_SAVE_COMPLETE:
		{
			gui_position({850, 400});
			gui_start_panel("Game Saved!", menuColor);

			if (gui_button("Ok"))
			{
				scene->menuView = MENU_MAIN;
				gui_reset_selection();
			}

			gui_end_panel();

		} break;

		case MENU_CONFIRM_TELEPORT:
		{
			gui_position({50, 50});

			gui_start_panel("Teleport Player Here?", menuColor);

			if (gui_button("Yes"))
			{
				scene->playerCharacterTransform.position = scene->freeCamera.position;
				scene->menuView = MENU_OFF;
				scene->cameraMode = CAMERA_MODE_PLAYER;
			}

			if (gui_button("No"))
			{
				scene->menuView = MENU_OFF;
			}

			gui_end_panel();
		} break;

		case MENU_EDIT_SKY:
		{
			gui_position({100, 100});

			gui_start_panel("Edit Sky", menuColor);

			scene->skyColorSelection = gui_float_slider("Sky Color", scene->skyColorSelection, 0,1);

			char const * const colorFromTreeDistanceTexts [] = 
			{
				"Color From Tree Distance: Off",
				"Color From Tree Distance: On"
			};
			if (gui_button(colorFromTreeDistanceTexts[scene->getSkyColorFromTreeDistance]))
			{
				scene->getSkyColorFromTreeDistance = !scene->getSkyColorFromTreeDistance;
			}


			if (gui_button("Back"))
			{
				scene->menuView = MENU_MAIN;
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

	gui_position({1550, 50});
	if (scene->drawDebugShadowTexture)
	{
		gui_image(GRAPHICS_RESOURCE_SHADOWMAP_GUI_TEXTURE, {300, 300});
	}

	gui_end_frame();

	return keepScene;
}