internal Gui init_game_gui(GameAssets & assets)
{
	Gui gui = {};

	
	return gui;
}


bool32 do_gui(Game * game, PlatformInput * input)
{	

	local_persist bool demoWindowVisible = false;

	bool32 keepScene = true;

	if (ImGui::Begin("Main Editor"))
	{
		using namespace ImGui;

		if(Button("Exit to Main Menu"))
		{
			OpenPopup("Exit?");
		}

		ImVec2 center =
		{
			platform_window_get_width(platformWindow) * 0.5f,	
			platform_window_get_height(platformWindow) * 0.5f
		};
		SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});

		if (BeginPopupModal("Exit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			Text("Exit to Main Menu?");


			if (Button("YES", {80, 0}))
			{
				keepScene = false;
				log_debug(FILE_ADDRESS, "He said YES");
				CloseCurrentPopup();
			}
			SameLine();
			if (Button("NO", {80, 0}))
			{
				log_debug(FILE_ADDRESS, "He said NO");
				CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		{
				constexpr char const * const cameraModeLabels [] = { "Player", "Editor" };

	 			if(BeginCombo("Camera Mode", cameraModeLabels[game->cameraMode]))
				{
					for (s32 i = 0; i < CameraModeCount; ++i)
					{
						if (Selectable(cameraModeLabels[i], game->cameraMode == static_cast<CameraMode>(i)))
						{
							game->cameraMode = static_cast<CameraMode>(i);
						}
					}
					EndCombo();
				}

				constexpr char const * const debugLevelLabels [] = { "Off", "Player", "Player, NPC", "Player, NPC; Background" };

				if (BeginCombo("Debug Level", debugLevelLabels[global_debugLevel]))
				{
					for (s32 i = 0; i < array_count(debugLevelLabels); ++i)
					{
						if (Selectable(debugLevelLabels[i], global_debugLevel == i))
						{
							global_debugLevel = i;
						}
					}
					EndCombo();
				}

				bool drawDebugShadowTexture = game->drawDebugShadowTexture;
				Checkbox("Debug Shadow", &drawDebugShadowTexture);
				game->drawDebugShadowTexture = drawDebugShadowTexture;

				if (Button("Reload Shaders"))
				{
					graphics_development_reload_shaders(platformGraphics);
				}

				InputFloat("Time Scale", &game->timeScale, 0.1f);

				if (Button("Read Settings"))
				{
					read_settings_file(game_get_serialized_objects(*game));
					
					read_settings_file(monuments_get_serialized_objects(game->monuments));
					monuments_refresh_transforms(game->monuments, game->collisionSystem);
				}

				if (Button("Write Settings"))
				{
					PlatformFileHandle file = platform_file_open("settings", FileMode_write);

					write_settings_file(file, *game);

					platform_file_close(file);
				}
		}

		PushStyleVar(ImGuiStyleVar_IndentSpacing, 16);
		s32 pushedStyleVarCount = 1;

		local_persist bool skyEditorVisible 		= false;
		local_persist bool monumentsEditorVisible 	= false;
		local_persist bool treesEditorVisible 		= false;
		local_persist bool meshGenEditorVisible 	= false;
		local_persist bool cloudsEditorVisible 		= false;
		local_persist bool blocksEditorVisible 		= false;
		local_persist bool cameraEditorVisible 		= false;


		float indentWidth = 16;

		if (CollapsingHeader("Select Views"))
		{
			Indent();
			
			Checkbox("Sky", &skyEditorVisible);
			Checkbox("Monuments", &monumentsEditorVisible);
			Checkbox("Trees", &treesEditorVisible);
			Checkbox("Mesh Generation", &meshGenEditorVisible);
			Checkbox("Clouds", &cloudsEditorVisible);
			Checkbox("Blocks", &blocksEditorVisible);
			Checkbox("Camera", &cameraEditorVisible);

			Checkbox("ImGui Demo", &demoWindowVisible);

			Unindent();
		}

		if (skyEditorVisible && CollapsingHeader("Sky", &skyEditorVisible))
		{
			Indent();
			sky_editor(game->skySettings);
			Unindent();
		}

		if (monumentsEditorVisible && CollapsingHeader("Monuments", &monumentsEditorVisible))
		{
			Indent();
			monuments_editor(game->monuments, game->collisionSystem);
			Unindent();
		}

		if (treesEditorVisible && CollapsingHeader("Trees", &treesEditorVisible))
		{
			Indent();
			trees_editor(game->trees);
			Unindent();
		}

		if (cloudsEditorVisible && CollapsingHeader("Clouds", &cloudsEditorVisible))
		{
			Indent();
			clouds_editor(game->clouds);
			Unindent();
		}

		if (blocksEditorVisible && CollapsingHeader("Blocks", &blocksEditorVisible))
		{
			Indent();
			building_blocks_editor(	game->scene.buildingBlocks,
									game->selectedBuildingBlockIndex,
									game->scene,
									game->worldCamera,
									input);
			Unindent();
		}

		if (cameraEditorVisible && CollapsingHeader("Camera", &cameraEditorVisible))
		{
			Indent();

			PushID("playercamera");
			Text("Player Camera");
			DragFloat("Distance", &game->playerCamera.distance, 0.1, 0.1, highest_f32);
			DragFloat("Base Offset", &game->playerCamera.baseOffset, 0.1);
			DragFloat("Gamepad Rotate Speed", &game->playerCamera.gamepadRotateSpeed, 0.1);
			DragFloat("Mouse Rotate Speed", &game->playerCamera.mouseRotateSpeed, 0.1);
			PopID();

			Spacing();

			PushID("Editor");
			Text("Editor Camera");
			DragFloat("Distance", &game->editorCamera.distance, 0.1, 0.1, highest_f32);
			DragFloat("Base Offset", &game->editorCamera.baseOffset, 0.1);
			DragFloat("Gamepad Rotate Speed", &game->editorCamera.gamepadRotateSpeed, 0.1);
			DragFloat("Mouse Rotate Speed", &game->editorCamera.mouseRotateSpeed, 0.1);
			DragFloat("Right/Up Speed", &game->editorCamera.rightAndUpMoveSpeed, 0.1);
			DragFloat("Forward Speed", &game->editorCamera.forwardMoveSpeed, 0.1);
			PopID();

			Unindent();
		}

		if (meshGenEditorVisible && CollapsingHeader("Mesh Generation", &meshGenEditorVisible))
		{
			Indent();

			SliderFloat("Grid Scale", &game->metaballGridScale, 0.2, 1);
			Checkbox32("Draw", &game->drawMCStuff);

			Unindent();
		}

		PopStyleVar(pushedStyleVarCount);
	}
	ImGui::End();

	if (demoWindowVisible)
	{
		ImGui::ShowDemoWindow(&demoWindowVisible);
	}

	// -----------------------------------------------------------

	// Todo(Leo) Study: https://www.reddit.com/r/vulkan/comments/fe40bw/vulkan_image_imgui_integration/
	gui_position({1550, 30});
	if (game->drawDebugShadowTexture)
	{
		gui_image(GRAPHICS_RESOURCE_SHADOWMAP_GUI_MATERIAL, {300, 300});
	}

	// -----------------------------------------------------------

	return keepScene;
}