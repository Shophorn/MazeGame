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

			bool drawDebugShadowTexture = game->editorOptions.drawDebugShadowTexture;
			Checkbox("Debug Shadow", &drawDebugShadowTexture);
			game->editorOptions.drawDebugShadowTexture = drawDebugShadowTexture;

			if (Button("Reload Shaders"))
			{
				graphics_development_reload_shaders(platformGraphics);
			}

			InputFloat("Time Scale", &game->editorOptions.timeScale, 0.1f);

			if (Button("Read Settings"))
			{
				read_settings_file(game_get_serialized_objects(*game));
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

		float indentWidth = 16;

		if (TreeNodeEx("Sky", ImGuiTreeNodeFlags_Framed))
		{
			sky_editor(game->skySettings);
			TreePop();
		}

		if (TreeNodeEx("Trees", ImGuiTreeNodeFlags_Framed))
		{
			trees_editor(game->trees);
			TreePop();
		}

		if (TreeNodeEx("Blocks", ImGuiTreeNodeFlags_Framed))
		{
			game_scenery_editor(	game->sceneries[game->buildingBlockSceneryIndex].transforms,
									game->selectedBuildingBlockIndex,
									game->worldCamera,
									game->editorCamera.pivotPosition,
									input,
									game);
			TreePop();
		}

		if (TreeNodeEx("Pipes", ImGuiTreeNodeFlags_Framed))
		{
			game_scenery_editor(	game->sceneries[game->buildingPipeSceneryIndex].transforms,
									game->selectedBuildingPipeIndex,
									game->worldCamera,
									game->editorCamera.pivotPosition,
									input,
									game);
			TreePop();
		}

		if (TreeNodeEx("Camera", ImGuiTreeNodeFlags_Framed))
		{
			PushID("gamecamera");
			Text("Player Camera");
			DragFloat("Distance", &game->gameCamera.distance, 0.1, 0.1, highest_f32);
			DragFloat("Base Offset", &game->gameCamera.baseOffset, 0.1);
			DragFloat("Gamepad Rotate Speed", &game->gameCamera.gamepadRotateSpeed, 0.1);
			DragFloat("Mouse Rotate Speed", &game->gameCamera.mouseRotateSpeed, 0.1);
			PopID();

			Spacing();

			PushID("Editor");
			Text("Editor Camera");
			DragFloat("Gamepad Rotate Speed", &game->editorCamera.gamepadRotateSpeed, 0.1);
			DragFloat("Mouse Rotate Speed", &game->editorCamera.mouseRotateSpeed, 0.1);
			DragFloat("Right/Up Speed", &game->editorCamera.rightAndUpMoveSpeed, 0.1);
			DragFloat("Zoom Speed", &game->editorCamera.zoomSpeed, 0.1);
			Checkbox32("Show Pivot", &game->editorCamera.showPivot);
			PopID();

			TreePop();
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
	if (game->editorOptions.drawDebugShadowTexture)
	{
		gui_image(GRAPHICS_RESOURCE_SHADOWMAP_GUI_MATERIAL, {300, 300});
	}

	// -----------------------------------------------------------

	return keepScene;
}