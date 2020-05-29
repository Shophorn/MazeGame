/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for 3d development scene
=============================================================================*/
#include "CharacterMotor.cpp"
#include "PlayerController3rdPerson.cpp"
#include "FollowerController.cpp"

enum CameraMode : s32
{ 
	CAMERA_MODE_PLAYER, 
	CAMERA_MODE_ELEVATOR,

	CAMERA_MODE_COUNT
};

enum CarryMode : s32
{ 	
	CARRY_NONE,
	CARRY_POT,
	CARRY_WATER,
	CARRY_SEED,
};

struct Trees
{
	f32 * 			growths;
	v3 * 			positions;
	quaternion * 	rotations;

	s32 			count;
};

struct Scene3d
{
	// Components
	Array<Transform3D> 			transforms;

	Array<SkeletonAnimator> 	skeletonAnimators;	
	Array<CharacterMotor>		characterMotors;
	Array<CharacterInput>		characterInputs;
	Array<Renderer> 			renderers;
	Array<AnimatedRenderer> 	animatedRenderers;

	// Systems
	// Todo(Leo): Make components?
	CollisionSystem3D 			collisionSystem;

	// Player
	Camera 						worldCamera;
	PlayerCameraController 		playerCamera;
	FreeCameraController		freeCamera;


	PlayerInputState 			playerInputState;
	Transform3D * 				playerCharacterTransform;

	static constexpr s32 potCapacity = 10;
	Transform3D potTransforms[potCapacity];
	s32 potsWithTreeCount;
	s32 potsWithSeedCount;
	s32 totalPotsCount;

	static constexpr s32 waterCapacity = 20;
	Transform3D waterTransforms[waterCapacity];
	s32	waterCount;

	static constexpr s32 seedCapacity = 20;
	Transform3D	seedTransforms[seedCapacity];
	s32 currentSeedCount;

	Trees trees;
	
	f32 treeGrowths	[seedCapacity];
	Transform3D treeTransforms[seedCapacity];
	s32 treesInPotCount;
	s32 treesOnGroundCount;

	MeshHandle 		dropMesh;
	MaterialHandle 	waterDropMaterial;
	MaterialHandle 	seedMaterial;

	// ModelHandle waterModel;
	// ModelHandle seedModel;
	ModelHandle treeModel;
	ModelHandle potModel;

	s32 playerCarryState;
	s32 carriedItemIndex;

	// Big Scenery
	s32 			stoneWallCount;
	m44 * 			stoneWallTransforms;
	MeshHandle 		stoneWallMesh;
	MaterialHandle 	stoneWallMaterial;

	s32 			buildingCount;
	m44 *			buildingTransforms;
	MeshHandle 		buildingMesh;
	MaterialHandle 	buildingMaterial;

	s32 			gateCount;
	m44 * 			gateTransforms;
	MeshHandle 		gateMesh;
	MaterialHandle 	gateMaterial;

	// struct ArrayRenderer
	// {
	// 	s32 			count;
	// 	Transform3D * 	transforms;
	// 	MeshHandle 		mesh;
	// 	MaterialHandle 	material;
	// };

	// ArrayRenderer stoneWalls;
	// ArrayRenderer buildings;

	// Other actors
	static constexpr s32 followerCapacity = 30;
	FollowerController followers[followerCapacity];

	static constexpr s32 walkerCapacity = 10;
	RandomWalkController randomWalkers [walkerCapacity];

	// Data
	Animation characterAnimations [CharacterAnimations::ANIMATION_COUNT];

	// Random
	ModelHandle skybox;
	Gui 		gui;
	s32 		cameraMode;
	bool32		drawDebugShadowTexture;

	enum MenuView : s32
	{
		MENU_OFF,
		MENU_MAIN,
		MENU_CONFIRM_EXIT,
	};

	MenuView menuView;

	GuiTextureHandle testGuiHandle;
};

internal bool32 update_scene_3d(void * scenePtr, game::Input * input)
{
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);

	/* Sadly, we need to draw skybox before game logic, because otherwise
	debug lines would be hidden. This can be fixed though, just make debug pipeline similar to shadows. */ 
	platformApi->draw_model(platformGraphics, scene->skybox, identity_m44, false, nullptr, 0);

	/*
	1. update input
	2. update animation controller
	3. update animator
	4. update animated renderer
	5. draw
	
	*/

	// Game Logic section
	if(scene->cameraMode == CAMERA_MODE_PLAYER)
	{
		update_player_input(scene->playerInputState, scene->characterInputs, scene->worldCamera, *input);
		update_camera_controller(&scene->playerCamera, scene->playerCharacterTransform->position, input);

		scene->worldCamera.position = scene->playerCamera.position;
		scene->worldCamera.direction = scene->playerCamera.direction;
	}
	else
	{
		scene->characterInputs[scene->playerInputState.inputArrayIndex] = {};
		update_free_camera(scene->freeCamera, *input);

		scene->worldCamera.position = scene->freeCamera.position;
		scene->worldCamera.direction = scene->freeCamera.direction;
	}	

	/*
	RULES:
		- Seed, water and pot can be carried
		- Seed can be put into a pot, and never removed
		- Water can be used once to water a seed or tree
		- When watered, seed will turn into a tree
			- if seed is in pot, tree will be in pot
			- if seed is on ground, tree will be on ground	
			- also, seed will disappear

		- When watered, tree will grow

		- Tree in pot:
			- will move with pot
			- has growth limit
		- Tree on ground:
			- cannot be moved
			- does not have growth limit
	*/

	/*
	check pickups
	check drops
	update carried item
	*/


	if (is_clicked(input->A))
	{
		v3 playerPosition = scene->playerCharacterTransform->position;
		f32 grabDistance = 1.0f;
		// s32 oldCarryState = scene->playerCarryState;
		// s32 newCarryState = scene->playerCarryState;

		switch(scene->playerCarryState)
		{
			case CARRY_NONE: {
				s32 newCarryMode = CARRY_NONE;
				s32 newCarryIndex = -1;

				for (s32 i = 0; i < scene->totalPotsCount; ++i)
				{
					if (magnitude_v3(playerPosition - scene->potTransforms[i].position) < grabDistance)
					{
						scene->playerCarryState = CARRY_POT;
						scene->carriedItemIndex = i;
						goto CARRY_NONE_ESCAPE;
					}
				}

				for (s32 i = 0; i < scene->waterCount; ++i)
				{
					if(magnitude_v3(playerPosition - scene->waterTransforms[i].position) < grabDistance)
					{
						scene->playerCarryState = CARRY_WATER;
						scene->carriedItemIndex = i;
						goto CARRY_NONE_ESCAPE;
					}
				}

				for (s32 i = 0; i < scene->currentSeedCount; ++i)
				{
					if (magnitude_v3(playerPosition - scene->seedTransforms[i].position) < grabDistance)
					{
						scene->playerCarryState = CARRY_SEED;
						scene->carriedItemIndex = i;
						goto CARRY_NONE_ESCAPE;
					}
				}
			
				// Note(Leo): Goto here if we start carrying in some of the loops above, so we dont override it immediately.
				CARRY_NONE_ESCAPE:;
			} break;

			case CARRY_POT:
				scene->playerCarryState = CARRY_NONE;
				scene->potTransforms[scene->carriedItemIndex].position.z
					= get_terrain_height(&scene->collisionSystem, scene->potTransforms[scene->carriedItemIndex].position.xy);
				break;
				
			case CARRY_WATER:
			{
				Transform3D & waterTransform = scene->waterTransforms[scene->carriedItemIndex];

				scene->playerCarryState = CARRY_NONE;
				waterTransform.position.z = get_terrain_height(&scene->collisionSystem, waterTransform.position.xy);

				/// LOOK FOR NEARBY SEEDS ON GROUND
				for (s32 seedIndex = 0; seedIndex < scene->currentSeedCount; ++seedIndex)
				{
					// Todo(Leo): Once we have some benchmarking, test if this was faster with all distances computed before
					f32 distanceToSeed = magnitude_v3(waterTransform.position - scene->seedTransforms[seedIndex].position);
					if (distanceToSeed < 0.5f)
					{
						s32 treeIndex = scene->treesInPotCount + scene->treesOnGroundCount;
						scene->treeGrowths[treeIndex] = 0.5f;
						scene->treeTransforms[treeIndex].position = scene->seedTransforms[seedIndex].position;
						scene->treeTransforms[treeIndex].rotation = identity_quaternion;
						scene->treeTransforms[treeIndex].scale = make_uniform_v3(scene->treeGrowths[treeIndex]);

						++scene->treesOnGroundCount;

						scene->currentSeedCount -= 1;
						scene->seedTransforms[seedIndex] = scene->seedTransforms[scene->currentSeedCount];

						scene->waterCount -= 1;
						scene->waterTransforms[scene->carriedItemIndex] = scene->waterTransforms[scene->waterCount]; 
						break;
					}
				}

				/// LOOK FOR NEARBY SEEDS IN POTS
				s32 startPotWithSeed = scene->potsWithTreeCount;
				s32 endPotWithSeed = scene->potsWithTreeCount + scene->potsWithSeedCount;
				for (s32 potIndex = startPotWithSeed; potIndex < endPotWithSeed; ++potIndex)
				{
					f32 distanceToPotWithSeed = magnitude_v3(waterTransform.position - scene->potTransforms[potIndex].position);
					if (distanceToPotWithSeed < 0.5f)
					{
						s32 newTreeInPotIndex = scene->treesInPotCount;

						if (scene->treesOnGroundCount > 0)
						{
							s32 treeMoveFromIndex = newTreeInPotIndex;
							s32 treeMoveToIndex = scene->treesInPotCount + scene->treesOnGroundCount;
							scene->treeGrowths[treeMoveToIndex] = scene->treeGrowths[treeMoveFromIndex];
							scene->treeTransforms[treeMoveToIndex] = scene->treeTransforms[treeMoveFromIndex];
							// Note(Leo): no need to worry about pot index here; we are on ground!
						}

						scene->treeGrowths[newTreeInPotIndex] = 0.5f;
						// scene->treePotIndices[newTreeInPotIndex] = potIndex;
						scene->treeTransforms[newTreeInPotIndex].position = scene->potTransforms[potIndex].position;
						scene->treeTransforms[newTreeInPotIndex].rotation = identity_quaternion;
						scene->treeTransforms[newTreeInPotIndex].scale = make_uniform_v3(scene->treeGrowths[newTreeInPotIndex]);

						/* Note(Leo): this seems to work, we can rely on it for a while, until we make proper hashmaps.
						It works because there is equal number of pots with trees and trees in pots, naturally, and they
						both are stored in order of introduction at the beningning of their storing arrays. Kinda wild. */
						Assert(newTreeInPotIndex == potIndex);


						swap(scene->potTransforms[startPotWithSeed], scene->potTransforms[potIndex]);

						++scene->treesInPotCount;
						++scene->potsWithTreeCount;
						--scene->potsWithSeedCount;

						scene->waterCount -= 1;
						scene->waterTransforms[scene->carriedItemIndex] = scene->waterTransforms[scene->waterCount]; 

						break;
					}
				}

				/// LOOK FOR NEARBY TREES IN POTS
				for (s32 treeIndex = 0; treeIndex < scene->treesInPotCount; ++treeIndex)
				{
					f32 distanceToTreeInPot = magnitude_v3(waterTransform.position - scene->treeTransforms[treeIndex].position);
					if (distanceToTreeInPot < 0.5f)
					{
						// scene->treeGrowths[treeIndex] += 0.5f;
						scene->treeGrowths[treeIndex] = math::min(scene->treeGrowths[treeIndex] + 0.5f, 1.0f);
						scene->treeTransforms[treeIndex].scale = make_uniform_v3(scene->treeGrowths[treeIndex]);

						scene->waterCount -= 1;
						scene->waterTransforms[scene->carriedItemIndex] = scene->waterTransforms[scene->waterCount];

						break;
					}
				}
				
				/// LOOK FOR NEARBY TREES ON GROUND
				for (s32 treeIndex = scene->treesInPotCount; treeIndex < (scene->treesInPotCount + scene->treesOnGroundCount); ++treeIndex)
				{
					f32 distnceToTreeOnGround = magnitude_v3(waterTransform.position - scene->treeTransforms[treeIndex].position);
					if (distnceToTreeOnGround < 0.5f)
					{
						scene->treeGrowths[treeIndex] += 0.5f;
						scene->treeTransforms[treeIndex].scale = make_uniform_v3(scene->treeGrowths[treeIndex]);

						scene->waterCount -= 1;
						scene->waterTransforms[scene->carriedItemIndex] = scene->waterTransforms[scene->waterCount];

						break;
					}
				}

			} break;

			case CARRY_SEED:
				scene->playerCarryState = CARRY_NONE;
				scene->seedTransforms[scene->carriedItemIndex].position.z = get_terrain_height(&scene->collisionSystem, scene->seedTransforms[scene->carriedItemIndex].position.xy);

				s32 firstEmptyPot = scene->potsWithTreeCount + scene->potsWithSeedCount;
				for (s32 potIndex = firstEmptyPot; potIndex < scene->totalPotsCount; ++potIndex)
				{
					f32 distanceToEmptyPot = magnitude_v3(scene->potTransforms[potIndex].position - scene->seedTransforms[scene->carriedItemIndex].position);
					if(distanceToEmptyPot < 0.5f)
					{
						swap (scene->potTransforms[firstEmptyPot], scene->potTransforms[potIndex]);

						++scene->potsWithSeedCount;

						scene->currentSeedCount -= 1;
						scene->seedTransforms[scene->carriedItemIndex] = scene->seedTransforms[scene->currentSeedCount];

						break;
					}

				}
				break;
		}
	} // endif input


	v3 carriedPosition = multiply_point(transform_matrix(*scene->playerCharacterTransform), {0, 0.7, 0.7});
	quaternion carriedRotation = scene->playerCharacterTransform->rotation;
	switch(scene->playerCarryState)
	{
		case CARRY_POT:
			scene->potTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->potTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_WATER:
			scene->waterTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->waterTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_SEED:
			scene->seedTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->seedTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		default:
			break;
	}

	// Todo(Leo): We really do not need to update these unless they are carried, figure how to know which one is carried,
	// since they do not directly map to pot indices
	for (s32 i = 0; i < scene->treesInPotCount; ++i)
	{
		scene->treeTransforms[i].position = multiply_point(transform_matrix(scene->potTransforms[i]), v3{0, 0, 0.25});
		scene->treeTransforms[i].rotation = scene->potTransforms[i].rotation;
	}
	
	// -----------------------------------------------------------------------------------------------------------

	// Todo(Leo): Rather use something like submit_collider() with what every collider can decide themselves, if they want to
	// contribute to collisions this frame or not.
	precompute_colliders(scene->collisionSystem);

	for (auto const & collider : scene->collisionSystem.cylinderColliders)
	{
		debug_draw_circle_xy(collider.transform->position + collider.center, collider.radius, colors::brightYellow, DEBUG_BACKGROUND);
	}
	debug_draw_circle_xy(scene->playerCharacterTransform->position + v3{0,0,0.7}, 0.25f, colors::brightGreen, DEBUG_BACKGROUND);


	for (auto & randomWalker : scene->randomWalkers)
	{
		update_random_walker_input(randomWalker, scene->characterInputs, input->elapsedTime);
	}

	for (auto & follower : scene->followers)
	{
		update_follower_input(follower, scene->characterInputs, input->elapsedTime);
	}
	
	for (int i = 0; i < scene->characterMotors.count(); ++i)
	{
		update_character_motor(	scene->characterMotors[i],
								scene->characterInputs[i],
								scene->collisionSystem,
								input->elapsedTime);
	}


	for (auto & animator : scene->skeletonAnimators)
	{
		update_skeleton_animator(animator, input->elapsedTime);
	}

	// Rebellious(Leo): stop using systems, and instead just do the stuff you need to >:)
	/// DRAW UNUSED WATER
	m44 * waterTransforms = push_memory<m44>(*global_transientMemory, scene->waterCount, ALLOC_NO_CLEAR);
	for (s32 i = 0; i < scene->waterCount; ++i)
	{
		waterTransforms[i] = transform_matrix(scene->waterTransforms[i]);
	}
	platformApi->draw_meshes(platformGraphics, scene->waterCount, waterTransforms, scene->dropMesh, scene->waterDropMaterial);
	
	/// DRAW UNUSED SEEDS
	m44 * seedTransforms = push_memory<m44>(*global_transientMemory, scene->currentSeedCount, ALLOC_NO_CLEAR);
	for (s32 i = 0; i < scene->currentSeedCount; ++i)
	{
		seedTransforms[i] = transform_matrix(scene->seedTransforms[i]);
	}
	platformApi->draw_meshes(platformGraphics, scene->currentSeedCount, seedTransforms, scene->dropMesh, scene->seedMaterial);

	/// DRAW TREES
	s32 totalTreeCount = scene->treesInPotCount + scene->treesOnGroundCount;
	for (s32 i = 0; i < totalTreeCount; ++i)
	{
		platformApi->draw_model(platformGraphics, scene->treeModel,
								transform_matrix(scene->treeTransforms[i]),
								true,
								nullptr, 0);
	}

	/// DRAW POTS
	for (s32 i = 0; i < scene->totalPotsCount; ++i)
	{
		platformApi->draw_model(platformGraphics, scene->potModel,
								transform_matrix(scene->potTransforms[i]),
								true,
								nullptr, 0);
	}

	/// DRAW SEEDS INSIDE POTS
	if (scene->potsWithSeedCount > 0)
	{
		m44 * seedsInPotTransforms = push_memory<m44>(*global_transientMemory, scene->potsWithSeedCount, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < scene->potsWithSeedCount; ++i)
		{
			// Note(Leo): pots are ordered so that first pots have trees, then seeds, and last empty
			s32 seedInPotIndex = i + scene->potsWithTreeCount;
			seedsInPotTransforms[i] = transform_matrix(scene->potTransforms[seedInPotIndex]);
		}
		platformApi->draw_meshes(platformGraphics, scene->potsWithSeedCount, seedsInPotTransforms, scene->dropMesh, scene->seedMaterial);
	}

	/// DRAW STONE WALLS
	{
		platformApi->draw_meshes(platformGraphics, scene->stoneWallCount, scene->stoneWallTransforms, scene->stoneWallMesh, scene->stoneWallMaterial);
		platformApi->draw_meshes(platformGraphics, scene->buildingCount, scene->buildingTransforms, scene->buildingMesh, scene->buildingMaterial);
		platformApi->draw_meshes(platformGraphics, scene->gateCount, scene->gateTransforms, scene->gateMesh, scene->gateMaterial);
	}


	/// DEBUG DRAW COLLIDERS
	{
		for (auto const & collider : scene->collisionSystem.precomputedColliders)
		{
			debug_draw_box(collider.transform, colors::mutedGreen, DEBUG_BACKGROUND);
		}

		for (auto const & collider : scene->collisionSystem.staticBoxColliders)
		{
			debug_draw_box(collider.transform, colorDarkGreen, DEBUG_BACKGROUND);
		}
	}

	  //////////////////////////////
	 /// 	RENDERING 			///
	//////////////////////////////
	/*
		1. Update camera and lights
		2. Render normal models
		3. Render animated models
	*/


	update_camera_system(&scene->worldCamera, input, platformGraphics, platformWindow);

	Light light = { .direction 	= normalize_v3({1, -1.2, -3}), 
					.color 		= v3{0.95, 0.95, 0.9}};
	v3 ambient 	= {0.1, 0.1, 0.5};
	platformApi->update_lighting(platformGraphics, &light, &scene->worldCamera, ambient);

	for (auto & renderer : scene->renderers)
	{
		platformApi->draw_model(platformGraphics, renderer.model,
								transform_matrix(*renderer.transform),
								renderer.castShadows,
								nullptr, 0);
	}


	for (auto & renderer : scene->animatedRenderers)
	{
		m44 boneTransformMatrices [32];

		update_animated_renderer(boneTransformMatrices, renderer.skeleton->bones);

		platformApi->draw_model(platformGraphics, renderer.model, transform_matrix(*renderer.transform),
								renderer.castShadows, boneTransformMatrices, array_count(boneTransformMatrices));
	}

	// ------------------------------------------------------------------------

	gui_start(scene->gui, input);

	if (is_clicked(input->start))
	{
		if (scene->menuView == Scene3d::MENU_OFF)
		{
			scene->menuView = Scene3d::MENU_MAIN;
			gui_reset_selection();
		}
		else
		{
			scene->menuView = Scene3d::MENU_OFF;
		}
	}

	bool32 keepScene = true;
	
	switch(scene->menuView)
	{	
		case Scene3d::MENU_OFF:
			// Nothing to do
			break;

		case Scene3d::MENU_CONFIRM_EXIT:
			gui_position({900, 300});
			gui_text("Exit to Main Menu");

			if (gui_button("Yes"))
			{
				keepScene = false;
			}

			if (gui_button("No"))
			{
				scene->menuView = Scene3d::MENU_MAIN;
				gui_reset_selection();
			}
			break;

		case Scene3d::MENU_MAIN:
			gui_position({900, 300});	
			if (gui_button("Continue"))
			{
				scene->menuView = Scene3d::MENU_OFF;
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

			if (gui_button("Exit Scene"))
			{
				scene->menuView = Scene3d::MENU_CONFIRM_EXIT;
				gui_reset_selection();
			}
			break;
	}

	gui_position({100, 100});
	gui_text("Sphinx of black quartz, judge my vow!");

	// gui_pivot(GUI_PIVOT_TOP_RIGHT);
	// gui_position({100, 100});
	// gui_image(shadowTexture, {300, 300});

	gui_position({1550, 50});
	if (scene->drawDebugShadowTexture)
	{
		gui_image(GRAPHICS_RESOURCE_SHADOWMAP_GUI_TEXTURE, {300, 300});
	}
	gui_image(scene->testGuiHandle, {300, 300}, v4{0.8,0.8,0.8, 0.5});

	gui_end();

	return keepScene;
}

void * load_scene_3d(MemoryArena & persistentMemory)
{
	void * scenePtr = allocate(persistentMemory, sizeof(Scene3d), 0);
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);
	*scene = {};

	// ----------------------------------------------------------------------------------

	scene->gui 						= {};
	scene->gui.textSize 			= 40;
	scene->gui.textColor 			= colors::white;
	scene->gui.selectedTextColor 	= colors::mutedRed;
	scene->gui.padding 				= 10;
	scene->gui.font 				= load_font("c:/windows/fonts/arial.ttf");
	gui_generate_font_material(scene->gui);
	
	// ----------------------------------------------------------------------------------

	// Note(Leo): amounts are SWAG, rethink.
	scene->transforms 			= allocate_array<Transform3D>(persistentMemory, 1200);
	scene->skeletonAnimators 	= allocate_array<SkeletonAnimator>(persistentMemory, 600);
	scene->animatedRenderers 	= allocate_array<AnimatedRenderer>(persistentMemory, 600);
	scene->renderers 			= allocate_array<Renderer>(persistentMemory, 600);

	// Todo(Leo): Probaly need to allocate these more coupledly, at least they must be able to reordered together
	scene->characterMotors		= allocate_array<CharacterMotor>(persistentMemory, 600);
	scene->characterInputs		= allocate_array<CharacterInput>(persistentMemory, scene->characterMotors.capacity(), ALLOC_FILL | ALLOC_NO_CLEAR);

	scene->collisionSystem.boxColliders 		= allocate_array<BoxCollider>(persistentMemory, 600);
	scene->collisionSystem.cylinderColliders 	= allocate_array<CylinderCollider>(persistentMemory, 600);
	scene->collisionSystem.staticBoxColliders 	= allocate_array<StaticBoxCollider>(persistentMemory, 2000);

	logSystem() << "Allocations succesful! :)";

	// ----------------------------------------------------------------------------------

	{
		TextureAsset testGuiAsset = load_texture_asset( "assets/textures/texture.jpg", global_transientMemory);
		scene->testGuiHandle = platformApi->push_gui_texture(platformGraphics, &testGuiAsset);
	}

	struct MaterialCollection {
		MaterialHandle character;
		MaterialHandle environment;
		MaterialHandle ground;
		MaterialHandle sky;
	} materials;

	auto load_and_push_texture = [](const char * path) -> TextureHandle
	{
		auto asset = load_texture_asset(path, global_transientMemory);
		auto result = platformApi->push_texture(platformGraphics, &asset);
		return result;
	};

	TextureAsset whiteTextureAsset 			= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xffffffff}), 1, 1, 4);
	TextureAsset blackTextureAsset	 		= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff000000}), 1, 1, 4);
	TextureAsset neutralBumpTextureAsset 	= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff8080ff}), 1, 1, 4);
	TextureAsset waterBlueTextureAsset 		= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xffffdb00}), 1, 1, 4);
	TextureAsset seedBrownTextureAsset 		= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff003399}), 1, 1, 4);

	TextureHandle whiteTexture 			= platformApi->push_texture(platformGraphics, &whiteTextureAsset);
	TextureHandle blackTexture 			= platformApi->push_texture(platformGraphics, &blackTextureAsset);
	TextureHandle neutralBumpTexture 	= platformApi->push_texture(platformGraphics, &neutralBumpTextureAsset);
	TextureHandle waterTextureHandle	= platformApi->push_texture(platformGraphics, &waterBlueTextureAsset);
	TextureHandle seedTextureHandle		= platformApi->push_texture(platformGraphics, &seedBrownTextureAsset);

	auto push_material = [](GraphicsPipeline pipeline, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
	{
		TextureHandle textures [] = {a, b, c};
		MaterialHandle handle = platformApi->push_material(platformGraphics, pipeline, 3, textures);
		return handle;
	};

	// Create MateriaLs
	{
		auto tilesTexture 	= load_and_push_texture("assets/textures/tiles.jpg");
		auto groundTexture 	= load_and_push_texture("assets/textures/ground.png");
		auto lavaTexture 	= load_and_push_texture("assets/textures/lava.jpg");
		auto faceTexture 	= load_and_push_texture("assets/textures/texture.jpg");

		materials =
		{
			.character 		= push_material(GRAPHICS_PIPELINE_ANIMATED, lavaTexture, neutralBumpTexture, blackTexture),
			// .environment 	= push_material(defaultPipeline, tilesTexture, neutralBumpTexture, blackTexture),
			.environment 	= push_material(GRAPHICS_PIPELINE_NORMAL, tilesTexture, neutralBumpTexture, blackTexture),
			.ground 		= push_material(GRAPHICS_PIPELINE_NORMAL, groundTexture, neutralBumpTexture, blackTexture),
		};
		
		// Note(Leo): internet told us vulkan(or glsl) cubemap face order is as follows:
		// (+X,-X,+Y,-Y,+Z,-Z).
		// ... seems to wrock
		StaticArray<TextureAsset,6> skyboxTextureAssets =
		{
			load_texture_asset("assets/textures/miramar_rt.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_lf.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_ft.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_bk.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_up.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_dn.png", global_transientMemory),
		};
		auto skyboxTexture 	= platformApi->push_cubemap(platformGraphics, &skyboxTextureAssets);
		materials.sky 		= platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_SKYBOX, 1, &skyboxTexture);
	}

	auto push_model = [](MeshHandle mesh, MaterialHandle material) -> ModelHandle
	{
		auto handle = platformApi->push_model(platformGraphics, mesh, material);
		return handle;
	};

	// Skybox
	{
		auto meshAsset 	= create_skybox_mesh(global_transientMemory);
		auto meshHandle = platformApi->push_mesh(platformGraphics, &meshAsset);
		scene->skybox 	= push_model(meshHandle, materials.sky);
	}



	// Characters
	{
		scene->playerCarryState = CARRY_NONE;

		char const * filename 	= "assets/models/cube_head_v3.glb";
		auto const gltfFile 	= read_gltf_file(*global_transientMemory, filename);

		auto girlMeshAsset 	= load_mesh_glb(*global_transientMemory, gltfFile, "cube_head");
		auto girlMesh 		= platformApi->push_mesh(platformGraphics, &girlMeshAsset);

		// --------------------------------------------------------------------

		Transform3D * playerTransform = scene->transforms.push_return_pointer({10, 10, 5});
		scene->playerCharacterTransform = playerTransform;

		s32 motorIndex = scene->characterMotors.count();
		scene->playerInputState.inputArrayIndex = motorIndex;
		auto * motor = scene->characterMotors.push_return_pointer();
		motor->transform = playerTransform;

		{
			using namespace CharacterAnimations;			

			auto startTime = platformApi->current_time();

			scene->characterAnimations[WALK] 	= load_animation_glb(persistentMemory, gltfFile, "Walk");
			scene->characterAnimations[RUN] 	= load_animation_glb(persistentMemory, gltfFile, "Run");
			scene->characterAnimations[IDLE] 	= load_animation_glb(persistentMemory, gltfFile, "Idle");
			scene->characterAnimations[JUMP]	= load_animation_glb(persistentMemory, gltfFile, "JumpUp");
			scene->characterAnimations[FALL]	= load_animation_glb(persistentMemory, gltfFile, "JumpDown");
			scene->characterAnimations[CROUCH] 	= load_animation_glb(persistentMemory, gltfFile, "Crouch");

			logSystem(1) << "Loading all 6 animations took: " << platformApi->elapsed_seconds(startTime, platformApi->current_time()) << " s";

			motor->animations[WALK] 	= &scene->characterAnimations[WALK];
			motor->animations[RUN] 		= &scene->characterAnimations[RUN];
			motor->animations[IDLE] 	= &scene->characterAnimations[IDLE];
			motor->animations[JUMP]		= &scene->characterAnimations[JUMP];
			motor->animations[FALL]		= &scene->characterAnimations[FALL];
			motor->animations[CROUCH] 	= &scene->characterAnimations[CROUCH];
		}

		auto startTime = platformApi->current_time();

		logSystem(1) << "Loading skeleton took: " << platformApi->elapsed_seconds(startTime, platformApi->current_time()) << " s";

		scene->skeletonAnimators.push({
			.skeleton 		= load_skeleton_glb(persistentMemory, gltfFile, "cube_head"),
			.animations 	= motor->animations,
			.weights 		= motor->animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		});

		// Note(Leo): take the reference here, so we can easily copy this down below.
		auto & cubeHeadSkeleton = scene->skeletonAnimators.last().skeleton;

		auto model = push_model(girlMesh, materials.character);
		scene->animatedRenderers.push(make_animated_renderer(
				playerTransform,
				&cubeHeadSkeleton,
				model));

		// --------------------------------------------------------------------

		for (s32 randomWalkerIndex = 0; randomWalkerIndex < array_count(scene->randomWalkers); ++randomWalkerIndex)
		{
			v3 position = {RandomRange(-99, 99), RandomRange(-99, 99), 0};
			v3 scale 	= make_uniform_v3(RandomRange(0.8f, 1.5f));

			auto * transform 	= scene->transforms.push_return_pointer({.position = position, .scale = scale});
			s32 motorIndex 		= scene->characterMotors.count();
			auto * motor 		= scene->characterMotors.push_return_pointer();
			motor->transform 	= transform;

			scene->randomWalkers[randomWalkerIndex] = {transform, motorIndex, {RandomRange(-99, 99), RandomRange(-99, 99)}};

			{
				using namespace CharacterAnimations;			

				motor->animations[WALK] 	= &scene->characterAnimations[WALK];
				motor->animations[RUN] 		= &scene->characterAnimations[RUN];
				motor->animations[IDLE] 	= &scene->characterAnimations[IDLE];
				motor->animations[JUMP]		= &scene->characterAnimations[JUMP];
				motor->animations[FALL]		= &scene->characterAnimations[FALL];
				motor->animations[CROUCH] 	= &scene->characterAnimations[CROUCH];
			}

			scene->skeletonAnimators.push({
					.skeleton 		= { .bones = copy_array(persistentMemory, cubeHeadSkeleton.bones) },
					.animations 	= motor->animations,
					.weights 		= motor->animationWeights,
					.animationCount = CharacterAnimations::ANIMATION_COUNT
				});

			auto model = push_model(girlMesh, materials.character); 
			scene->animatedRenderers.push(make_animated_renderer(transform,	&scene->skeletonAnimators.last().skeleton, model));
		}



		Transform3D * targetTransform = nullptr;
		s32 followersPerWalker = array_count(scene->followers) / array_count(scene->randomWalkers); 

		for (s32 followerIndex = 0; followerIndex < array_count(scene->followers); ++followerIndex)
		{
			v3 position 		= {RandomRange(-99, 99), RandomRange(-99, 99), 0};
			v3 scale 			= make_uniform_v3(RandomRange(0.8f, 1.5f));
			auto * transform 	= scene->transforms.push_return_pointer({.position = position, .scale = scale});

			s32 motorIndex 		= scene->characterMotors.count();
			auto * motor 		= scene->characterMotors.push_return_pointer();
			motor->transform 	= transform;


			if ((followerIndex % followersPerWalker) == 0)
			{
				s32 targetIndex = followerIndex / followersPerWalker;
				targetTransform = scene->randomWalkers[targetIndex].transform;
			}


			scene->followers[followerIndex] = make_follower_controller(transform, targetTransform, motorIndex);
			targetTransform = transform;

			{
				using namespace CharacterAnimations;			

				motor->animations[WALK] 	= &scene->characterAnimations[WALK];
				motor->animations[RUN] 		= &scene->characterAnimations[RUN];
				motor->animations[IDLE] 	= &scene->characterAnimations[IDLE];
				motor->animations[JUMP]		= &scene->characterAnimations[JUMP];
				motor->animations[FALL]		= &scene->characterAnimations[FALL];
				motor->animations[CROUCH] 	= &scene->characterAnimations[CROUCH];
			}

			scene->skeletonAnimators.push({
					.skeleton 		= { .bones = copy_array(persistentMemory, cubeHeadSkeleton.bones) },
					.animations 	= motor->animations,
					.weights 		= motor->animationWeights,
					.animationCount = CharacterAnimations::ANIMATION_COUNT
				});

			auto model = push_model(girlMesh, materials.character); 
			scene->animatedRenderers.push(make_animated_renderer(transform,	&scene->skeletonAnimators.last().skeleton, model));
		}
	}

	scene->worldCamera = make_camera(50, 0.1f, 1000.0f);
	scene->playerCamera = {};
	scene->playerCamera.baseOffset = {0, 0, 1.5f};
	scene->playerCamera.distance = 5;

	// Environment
	{
		// constexpr f32 depth = 500;
		// constexpr f32 width = 500;
		f32 mapSize 		= 1200;
		f32 terrainHeight 	= 20;
		{
			// Note(Leo): this is maximum size we support with u16 mesh vertex indices
			s32 terrainMeshResolution = 256;

			auto heightmapTexture 	= load_texture_asset("assets/textures/heightmap6.jpg", global_transientMemory);
			auto heightmap 			= make_heightmap(global_transientMemory, &heightmapTexture, terrainMeshResolution, mapSize, -terrainHeight / 2, terrainHeight / 2);
			auto groundMeshAsset 	= generate_terrain(global_transientMemory, 32, &heightmap);

			auto groundMesh 		= platformApi->push_mesh(platformGraphics, &groundMeshAsset);
			auto model 				= push_model(groundMesh, materials.ground);
			auto transform 			= scene->transforms.push_return_pointer({{-mapSize / 2, -mapSize / 2, 0}});

			scene->renderers.push({transform, model});

			scene->collisionSystem.terrainCollider = std::move(heightmap);
			scene->collisionSystem.terrainTransform = transform;
		}

		{
			auto totemMesh 			= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, "assets/models/totem.glb"), "totem");
			auto totemMeshHandle 	= platformApi->push_mesh(platformGraphics, &totemMesh);
			auto model = push_model(totemMeshHandle, materials.environment);

			Transform3D * transform = scene->transforms.push_return_pointer({});
			transform->position.z = get_terrain_height(&scene->collisionSystem, transform->position.xy) - 0.5f;
			scene->renderers.push({transform, model});
			push_box_collider(	scene->collisionSystem,
								v3 {1.0, 1.0, 5.0},
								v3 {0,0,2},
								transform);

			transform = scene->transforms.push_return_pointer({.position = {0, 5, 0} , .scale =  {0.5, 0.5, 0.5}});
			transform->position.z = get_terrain_height(&scene->collisionSystem, transform->position.xy) - 0.25f;
			scene->renderers.push({transform, model});
			push_box_collider(	scene->collisionSystem,
								v3{0.5, 0.5, 2.5},
								v3{0,0,1},
								transform);
		}

		{
			// test collider
			Transform3D * transform = scene->transforms.push_return_pointer({});
			transform->position = {21, 15};
			transform->position.z = get_terrain_height(&scene->collisionSystem, {20, 20});

			push_box_collider( 	scene->collisionSystem,
								v3{2.0f, 0.05f,5.0f},
								v3{0,0, 2.0f},
								transform);
		}


		{
			f32 pillarRange 			= 600;
			s32 pillarCountPerDirection = 20;
			s32 pillarCount 			= pillarCountPerDirection * pillarCountPerDirection;


			auto pillarMesh 		= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, "assets/models/big_pillar.glb"), "big_pillar");
			auto pillarMeshHandle 	= platformApi->push_mesh(platformGraphics, &pillarMesh);

			// pillarCount = 1;

			for (s32 pillarIndex = 0; pillarIndex < pillarCount; ++pillarIndex)
			{
				// mapSize = 30;
				s32 gridX = (pillarIndex % pillarCountPerDirection);
				s32 gridY = (pillarIndex / pillarCountPerDirection);

				if ((gridY % 4) > 0)
				{
					continue;
				}

				if ((gridX % 7) < 2)
				{
					continue;
				}


				f32 x = ((pillarIndex % pillarCountPerDirection) / (f32)pillarCountPerDirection) * pillarRange - pillarRange / 2;
				f32 y = ((pillarIndex / pillarCountPerDirection) / (f32)pillarCountPerDirection) * pillarRange - pillarRange / 2;

				f32 z = get_terrain_height(&scene->collisionSystem, v2{x, y});

				v3 position = {x, y, z - 1};
				auto transform = scene->transforms.push_return_pointer({position});
				transform->rotation = axis_angle_quaternion(up_v3, RandomRange(-tau / 8, tau / 8));
				
				auto model = push_model(pillarMeshHandle, materials.environment);
				scene->renderers.push({transform, model});

				push_box_collider(scene->collisionSystem, v3{2,2,50}, v3{0,0,25}, transform);
			}
		}

		// Test robot
		{
			v3 position 			= {21, 10, 0};
			position.z 				= get_terrain_height(&scene->collisionSystem, position.xy);
			auto * transform 		= scene->transforms.push_return_pointer({.position = position});

			char const * filename 	= "assets/models/Robot53.glb";
			auto meshAsset 			= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, filename), "model_rigged");	
			auto mesh 				= platformApi->push_mesh(platformGraphics, &meshAsset);

			auto albedo 			= load_and_push_texture("assets/textures/Robot_53_albedo_4k.png");
			auto material 			= push_material(GRAPHICS_PIPELINE_NORMAL, albedo, neutralBumpTexture, blackTexture);

			auto model 				= push_model(mesh, material);
			scene->renderers.push({transform, model});
		}

		{
			auto sceneryFile 		= read_gltf_file(*global_transientMemory, "assets/models/scenery.glb");
			
			auto smallPotMeshAsset 	= load_mesh_glb(*global_transientMemory, sceneryFile, "small_pot");
			auto smallPotMesh 		= platformApi->push_mesh(platformGraphics, &smallPotMeshAsset);

			scene->potModel			= push_model(smallPotMesh, materials.environment);
			for(s32 potIndex = 0; potIndex < scene->potCapacity; ++potIndex)
			{

				v3 position 					= {15, potIndex * 5.0f, 0};
				position.z 						= get_terrain_height(&scene->collisionSystem, position.xy);
				scene->potTransforms[potIndex]	= { .position = position };

				push_cylinder_collider(scene->collisionSystem, 0.3, 0.5, v3{0,0,0.25}, &scene->potTransforms[potIndex]);
			}

			scene->totalPotsCount = scene->potCapacity;

			// ----------------------------------------------------------------	

			auto bigPotMeshAsset 	= load_mesh_glb(*global_transientMemory, sceneryFile, "big_pot");
			auto bigPotMesh 		= platformApi->push_mesh(platformGraphics, &bigPotMeshAsset);

			s32 bigPotCount = 5;
			for (s32 i = 0; i < bigPotCount; ++i)
			{
				ModelHandle model 		= platformApi->push_model(platformGraphics, bigPotMesh, materials.environment);
				Transform3D * transform = scene->transforms.push_return_pointer({13, 2.0f + i * 4.0f, 0});

				transform->position.z 	= get_terrain_height(&scene->collisionSystem, transform->position.xy);

				scene->renderers.push({transform, model});
				push_cylinder_collider(scene->collisionSystem, 0.6, 1, v3{0,0,0.5}, transform);
			}

			// ----------------------------------------------------------------	

			auto treeTextureAsset 	= load_texture_asset("assets/textures/tree.png", global_transientMemory);
			auto treeTexture 		= platformApi->push_texture(platformGraphics, &treeTextureAsset);			
			auto treeMaterial 		= push_material(GRAPHICS_PIPELINE_NORMAL, treeTexture, neutralBumpTexture, blackTexture);
			auto treeMeshAsset 		= load_mesh_glb(*global_transientMemory, sceneryFile, "tree");
			auto treeMesh 			= platformApi->push_mesh(platformGraphics, &treeMeshAsset);

			scene->treeModel 		= push_model(treeMesh, treeMaterial);
			
			scene->treesInPotCount = 0;
			scene->treesOnGroundCount = 0;

			// scene->trees.growths = allocate(persistentMemory, sizeof(f32) * scene->seedCapacity);

			// ----------------------------------------------------------------	

			MeshAsset dropMeshAsset = load_mesh_glb(*global_transientMemory, sceneryFile, "water_drop");
			scene->dropMesh 		= platformApi->push_mesh(platformGraphics, &dropMeshAsset);

			TextureHandle waterTextures [] = {waterTextureHandle, neutralBumpTexture, blackTexture};
			scene->waterDropMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, waterTextures);

			scene->waterCount = scene->waterCapacity;
			for (s32 i = 0; i < scene->waterCount; ++i)
			{
				v3 position = {RandomRange(-50, 50), RandomRange(-50, 50)};
				position.z 	= get_terrain_height(&scene->collisionSystem, position.xy);

				scene->waterTransforms[i] = { .position = position };
			}

			TextureHandle seedTextures [] = {seedTextureHandle, neutralBumpTexture, blackTexture};
			scene->seedMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, seedTextures);

			scene->currentSeedCount = scene->seedCapacity;
			for(s32 i = 0; i < scene->seedCapacity; ++i)
			{
				scene->seedTransforms[i] = identity_transform;

				v3 position = {RandomRange(-50, 50), RandomRange(-50, 50), 0};
				position.z = get_terrain_height(&scene->collisionSystem, position.xy);
				scene->seedTransforms[i].position = position;
			}
		}

		{
			auto file 			= read_gltf_file(*global_transientMemory, "assets/models/stonewalls.glb");

			{
				auto meshAsset 		= load_mesh_glb(*global_transientMemory, file, "StoneWall.001");
				auto mesh 			= platformApi->push_mesh(platformGraphics, &meshAsset);
				auto textureAsset 	= load_texture_asset("assets/textures/stone_wall.jpg", global_transientMemory);
				auto texture 		= platformApi->push_texture(platformGraphics, &textureAsset);
				auto material   	= push_material(GRAPHICS_PIPELINE_NORMAL, texture, neutralBumpTexture, blackTexture);

				Array<BoxCollider> colliders = {};

				auto transformsArray = load_all_transforms_glb(persistentMemory, file, "StoneWall", &colliders);

				scene->stoneWallCount 		= transformsArray.count();
				scene->stoneWallTransforms 	= push_memory<m44>(persistentMemory, scene->stoneWallCount, ALLOC_NO_CLEAR);
				for (s32 i = 0; i < scene->stoneWallCount; ++i)
				{
					scene->stoneWallTransforms[i] = transform_matrix(transformsArray[i]);
				}
				scene->stoneWallMesh		= mesh;
				scene->stoneWallMaterial 	= material;

				for (auto & collider : colliders)
				{
					scene->collisionSystem.staticBoxColliders.push({compute_collider_transform(collider),
																	compute_inverse_collider_transform(collider)});
				}
			}

			{
				auto meshAsset 					= load_mesh_glb(*global_transientMemory, file, "Building.001");
				auto mesh 						= platformApi->push_mesh(platformGraphics, &meshAsset);

				Array<BoxCollider> colliders 	= {};
				auto transformsArray 			= load_all_transforms_glb(*global_transientMemory, file, "Building", &colliders);

				scene->buildingCount 			= transformsArray.count();
 				scene->buildingTransforms 		= push_memory<m44>(persistentMemory, scene->buildingCount, ALLOC_NO_CLEAR);
				for (s32 i = 0; i < scene->buildingCount; ++i)
				{
					scene->buildingTransforms[i] = transform_matrix(transformsArray[i]);
				}

				scene->buildingMesh = mesh;
				scene->buildingMaterial = materials.environment;

				for (auto & collider : colliders)
				{
					scene->collisionSystem.staticBoxColliders.push({compute_collider_transform(collider),
																	compute_inverse_collider_transform(collider)});
				}
			}

			{
				auto meshAsset 	= load_mesh_glb(*global_transientMemory, file, "Gate.001");
				auto meshHandle = platformApi->push_mesh(platformGraphics, &meshAsset);

				Array<BoxCollider> colliders = {};
				auto transformsArray = load_all_transforms_glb(*global_transientMemory, file, "Gate", &colliders);
				scene->gateTransforms = push_memory<m44>(persistentMemory, transformsArray.count(), ALLOC_NO_CLEAR);
				for (s32 i = 0; i < transformsArray.count(); ++i)
				{
					scene->gateTransforms[i] = transform_matrix(transformsArray[i]);
				}

				scene->gateCount 	= transformsArray.count();
				scene->gateMesh 	= meshHandle;
				scene->gateMaterial = materials.environment;

				for (auto collider : colliders)
				{
					scene->collisionSystem.staticBoxColliders.push({compute_collider_transform(collider),
																	compute_inverse_collider_transform(collider)});
				}
			}
		}

		logSystem(0) << "Scene3d loaded, " << used_percent(*global_transientMemory) * 100 << "% of transient memory used.";
		logSystem(0) << "Scene3d loaded, " << used_percent(persistentMemory) * 100 << "% of persistent memory used.";
	}


	return scenePtr;
}
