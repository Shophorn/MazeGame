static void building_blocks_editor(	Array<m44> & 	blocks,
									s64 & 			selectedBuildingBlockIndex,
									Scene & 		scene,
									Camera & 		worldCamera,
									PlatformInput * input)
{
	// bool blockEditorOpen = ImGui::Begin("Edit Building Blocks");
	// if (blockEditorOpen)
	{
		using namespace ImGui;

		Value("Count", static_cast<u32>(blocks.count));
		Value("Capacity", static_cast<u32>(blocks.capacity));

		// Todo(Leo): this is just quick test without trying s64 aka long long
		int selectedIndex = selectedBuildingBlockIndex;
		if(InputInt("Selected Index", &selectedIndex))
		{
			selectedIndex = s32_clamp(selectedIndex, 0, blocks.count - 1);
			selectedBuildingBlockIndex = selectedIndex;
		}

		if(Button("Add Box"))
		{
			if (blocks.count < blocks.capacity)
			{
				v3 position = multiply_point(blocks[selectedBuildingBlockIndex], {0,0,0}) + v3{0,0,2};

				selectedBuildingBlockIndex = blocks.count;
				blocks.count += 1;

				blocks[selectedBuildingBlockIndex] = translation_matrix(position);
			}
		}

		if (Button("Delete Box"))
		{
			if(blocks.count > 0)
			{
				array_unordered_remove(blocks, selectedBuildingBlockIndex);
			}
		}

		if (Button("Save Data"))
		{
			editor_scene_asset_write(scene);
		}

	}
	// ImGui::End();

	local_persist ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
	local_persist ImGuizmo::MODE mode 			= ImGuizmo::WORLD;

	// if (blockEditorOpen)
	{
		if (input_button_went_down(input, InputButton_keyboard_1))
		{
			operation 	= ImGuizmo::TRANSLATE;
			mode 		= ImGuizmo::WORLD;
		}
		else if (input_button_went_down(input, InputButton_keyboard_2))
		{
			operation 	= ImGuizmo::ROTATE;
			mode 		= ImGuizmo::LOCAL;
		}
		else if (input_button_went_down(input, InputButton_keyboard_3))
		{
			operation 	= ImGuizmo::SCALE;
			mode 		= ImGuizmo::LOCAL;
		}

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::BeginFrame();

		// Note(Leo): This is because our projection matrix is probably upside down
		float width = platform_window_get_width(platformWindow);
		float height = platform_window_get_height(platformWindow);
		ImGuizmo::SetRect(0, height, width, -height);

		m44 view 		= camera_view_matrix(worldCamera.position, worldCamera.direction);
		m44 projection 	= camera_projection_matrix(&worldCamera);
		m44 transform 	= blocks[selectedBuildingBlockIndex];

		ImGuizmo::Manipulate(	reinterpret_cast<f32*>(&view),
								reinterpret_cast<f32*>(&projection),
								operation,
								mode,
								reinterpret_cast<f32*>(&transform));

		blocks[selectedBuildingBlockIndex] = transform;
	}
}
