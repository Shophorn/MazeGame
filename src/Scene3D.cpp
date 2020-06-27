/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for 3d development scene
=============================================================================*/

#include "CharacterMotor.cpp"
#include "PlayerController3rdPerson.cpp"
#include "FollowerController.cpp"

#include "scene3d_monuments.cpp"
#include "scene3d_trees.cpp"

enum CameraMode : s32
{ 
	CAMERA_MODE_PLAYER, 
	CAMERA_MODE_ELEVATOR,

	CAMERA_MODE_COUNT
};

enum CarryMode : s32
{ 	
	CARRY_NONE,
	CARRY_POT_EMPTY,
	CARRY_POT_TREE,
	CARRY_POT_SEED,
	CARRY_WATER,
	CARRY_SEED,
};

struct Scene3d
{
	// Todo(Leo): Remove these
	Array<Transform3D> 			transforms;

	Array<SkeletonAnimator> 	skeletonAnimators;	
	Array<CharacterMotor>		characterMotors;
	Array<CharacterInput>		characterInputs;
	Array<Renderer> 			renderers;
	Array<AnimatedRenderer> 	animatedRenderers;

	CollisionSystem3D 			collisionSystem;

	// Player
	Camera 						worldCamera;
	PlayerCameraController 		playerCamera;
	FreeCameraController		freeCamera;

	PlayerInputState 			playerInputState;
	Transform3D * 				playerCharacterTransform;

	// ---------------------------------------
	// POTS

	s32 potCapacity;

	s32 potEmptyCount;
	s32 potTreeCount;
	s32 potSeedCount;

	Transform3D * potEmptyTransforms;
	Transform3D * potTreeTransforms;
	Transform3D * potSeedTransforms;

	f32 * potEmptyWaterLevels;
	f32 * potTreeWaterLevels;
	f32 * potSeedWaterLevels;

	f32 * seedsInPotWaterLevels;

	MeshHandle 		potMesh;
	MaterialHandle 	potMaterial;

	// ---------------------------------------

	Waters waters;
	static constexpr f32 fullWaterLevel = 1;

	s32 			seedCapacity;
	s32 			seedCount;
	Transform3D * 	seedTransforms;
	f32 * 			seedWaterLevels;

	Monuments monuments;

	MeshHandle 		dropMesh;
	MaterialHandle 	waterDropMaterial;
	MaterialHandle 	seedMaterial;

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

	s32 			cubePyramidCount;
	m44 *			cubePyramidTransforms;
	MeshHandle 		cubePyramidMesh;
	MaterialHandle 	cubePyramidMaterial;

	// Note(Leo): There are multiple mesh handles here
	s32 			terrainCount;
	m44 * 			terrainTransforms;
	MeshHandle * 	terrainMeshes;
	MaterialHandle 	terrainMaterial;

	m44 			seaTransform;
	MeshHandle 		seaMesh;
	MaterialHandle 	seaMaterial;

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
		MENU_CONFIRM_TELEPORT,
	};

	MenuView menuView;

	GuiTextureHandle guiPanelImage;
	v4 guiPanelColour;

	// Todo(Leo): this is kinda too hacky
	constexpr static s32 	timeScaleCount = 3;
	s32 					timeScaleIndex;

	s32 			lSystemCapacity;
	s32 			lSystemCount;
	TimedLSystem * 	lSystems;
	Leaves * 		lSystemLeavess;

	s32 			lSystemsInPotCount;
	TimedLSystem * 	lSystemsInPot;
	Leaves * 		lSystemsInPotLeavess;

	MaterialHandle 	lSystemTreeMaterial;
};

internal bool32 update_scene_3d(void * scenePtr, PlatformInput const & input, PlatformTime const & time)
{
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);
	
	/// ****************************************************************************
	/// DEFINE UPDATE FUNCTIONS

	auto get_water = [	carryState 			= scene->playerCarryState,
						carriedItemIndex 	= scene->carriedItemIndex]
					(Waters & waters, v3 position, f32 distanceThreshold, f32 requestedAmount) -> f32
	{	
		s32 closestIndex;
		f32 closestDistance = highest_f32;

		for (s32 i = 0; i < waters.count; ++i)
		{
			if (carriedItemIndex == i && carryState == CARRY_WATER)
			{
				continue;
			}

			f32 distance = magnitude_v3(waters.transforms[i].position - position);
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closestIndex 	= i;
			}
		}

		float amount = 0;

		if (closestDistance < distanceThreshold)
		{
			waters.levels[closestIndex] -= requestedAmount;
			if (waters.levels[closestIndex] < 0)
			{
				waters.count -= 1;
				waters.transforms[closestIndex] 	= waters.transforms[waters.count];
				waters.levels[closestIndex] 		= waters.levels[waters.count];
			}

			amount = requestedAmount;
		}

		return amount;
	};

	auto get_water_from_pot = [](f32 * potWaterLevels, s32 index, f32 amount) -> f32
	{
		amount = min_f32(amount, potWaterLevels[index]);
		potWaterLevels[index] -= amount;
		return amount;
	};

	auto add_l_system_tree = [	&lSystemCount 	= scene->lSystemCount,
								lSystems 		= scene->lSystems]
							(v3 position)
	{
		lSystems[lSystemCount].position 	= position;
		lSystems[lSystemCount].rotation 	= identity_quaternion;
		lSystemCount 						+= 1;
	};

	auto add_l_system_tree_in_pot = [	&lSystemCount 	= scene->lSystemsInPotCount,
										lSystems 		= scene->lSystemsInPot]
										(v3 position)
	{
		lSystems[lSystemCount].position 	= position;
		lSystems[lSystemCount].rotation 	= identity_quaternion;
		lSystemCount 						+= 1;
	};


	/// ****************************************************************************
	/// TIME

	f32 const timeScales [scene->timeScaleCount] { 1.0f, 0.1f, 0.5f }; 
	f32 scaledTime 		= time.elapsedTime * timeScales[scene->timeScaleIndex];
	f32 unscaledTime 	= time.elapsedTime;

	/// ****************************************************************************

	gui_start(scene->gui, input);

	/* Sadly, we need to draw skybox before game logic, because otherwise
	debug lines would be hidden. This can be fixed though, just make debug pipeline similar to shadows. */ 
	platformApi->draw_model(platformGraphics, scene->skybox, identity_m44, false, nullptr, 0);

	// Game Logic section
	if(scene->cameraMode == CAMERA_MODE_PLAYER)
	{
		if (scene->menuView == Scene3d::MENU_OFF)
		{
			// Todo(Leo): Maybe do submit motor thing, so we can also disable falling etc here
			update_player_input(scene->playerInputState, scene->characterInputs, scene->worldCamera, input);
		}
		else
		{
			scene->characterInputs[scene->playerInputState.inputArrayIndex] = {};
		}

		update_camera_controller(&scene->playerCamera, scene->playerCharacterTransform->position, input, scaledTime);

		scene->worldCamera.position = scene->playerCamera.position;
		scene->worldCamera.direction = scene->playerCamera.direction;

		scene->worldCamera.farClipPlane = 1000;
	}
	else
	{
		scene->characterInputs[scene->playerInputState.inputArrayIndex] = {};
		m44 cameraMatrix = update_free_camera(scene->freeCamera, input, unscaledTime);

		scene->worldCamera.position = scene->freeCamera.position;
		scene->worldCamera.direction = scene->freeCamera.direction;

		/// Document(Leo): Teleport player
		if (scene->menuView == Scene3d::MENU_OFF && is_clicked(input.A))
		{
			scene->menuView = Scene3d::MENU_CONFIRM_TELEPORT;
			gui_ignore_input();
			gui_reset_selection();
		}

		scene->worldCamera.farClipPlane = 2000;
	}	

	/// *******************************************************************************************

	update_camera_system(&scene->worldCamera, platformGraphics, platformWindow);

	Light light = { .direction 	= normalize_v3({-1, 1.2, -8}), 
					.color 		= v3{0.95, 0.95, 0.9}};
	v3 ambient 	= {0.05, 0.05, 0.3};

	constexpr f32 minPlayerDistance = 350;
	constexpr f32 maxPlayerDistance = 355;
	f32 playerDistance 				= magnitude_v2(scene->playerCharacterTransform->position.xy);
	light.skyColorSelection 		= clamp_f32((playerDistance - minPlayerDistance) / (maxPlayerDistance - minPlayerDistance), 0, 1);

	platformApi->update_lighting(platformGraphics, &light, &scene->worldCamera, ambient);

	/// *******************************************************************************************
	
	if ((scene->cameraMode == CAMERA_MODE_PLAYER) && is_clicked(input.Y))
	{
		Waters & waters = scene->waters;

		v2 center = scene->playerCharacterTransform->position.xy;

		s32 spawnCount = 10;
		spawnCount = min_f32(spawnCount, waters.capacity - waters.count);

		for (s32 i = 0; i < spawnCount; ++i)
		{
			f32 distance 	= RandomRange(1, 5);
			f32 angle 		= RandomRange(0, 2 * π);

			f32 x = cosine(angle) * distance;
			f32 y = sine(angle) * distance;

			v3 position = { x + center.x, y + center.y, 0 };
			position.z = get_terrain_height(&scene->collisionSystem, position.xy);

			waters.transforms[waters.count].position = position;
			waters.transforms[waters.count].rotation = identity_quaternion;
			waters.transforms[waters.count].scale = {1,1,1};

			waters.levels[waters.count] = scene->fullWaterLevel;

			waters.count += 1;
		}
	}

	if ((scene->cameraMode == CAMERA_MODE_PLAYER) && is_clicked(input.A))
	{
		v3 playerPosition = scene->playerCharacterTransform->position;
		f32 grabDistance = 1.0f;

		switch(scene->playerCarryState)
		{
			case CARRY_NONE: {
				/* Todo(Leo): Do this properly, taking into account player facing direction and distances etc. */
				auto check_pickup = [	playerPosition 		= scene->playerCharacterTransform->position,
										&playerCarryState 	= scene->playerCarryState,
										&carriedItemIndex 	= scene->carriedItemIndex,
										grabDistance]
									(s32 count, Transform3D const * transforms, s32 carryState)
				{
					for (s32 i = 0; i < count; ++i)
					{
						if (magnitude_v3(playerPosition - transforms[i].position) < grabDistance)
						{
							playerCarryState = carryState;
							carriedItemIndex = i;
						}
					}
				};

				check_pickup(scene->potEmptyCount, scene->potEmptyTransforms, CARRY_POT_EMPTY);
				check_pickup(scene->potTreeCount, scene->potTreeTransforms, CARRY_POT_TREE);
				check_pickup(scene->potSeedCount, scene->potSeedTransforms, CARRY_POT_SEED);
				check_pickup(scene->waters.count, scene->waters.transforms, CARRY_WATER);
				check_pickup(scene->seedCount, scene->seedTransforms, CARRY_SEED);

			} break;

			case CARRY_POT_EMPTY:
				scene->playerCarryState = CARRY_NONE;
				scene->potEmptyTransforms[scene->carriedItemIndex].position.z
					= get_terrain_height(&scene->collisionSystem, scene->potEmptyTransforms[scene->carriedItemIndex].position.xy);
				break;
	
			case CARRY_POT_TREE:
				scene->playerCarryState = CARRY_NONE;
				scene->potTreeTransforms[scene->carriedItemIndex].position.z
					= get_terrain_height(&scene->collisionSystem, scene->potTreeTransforms[scene->carriedItemIndex].position.xy);
				break;
	
			case CARRY_POT_SEED:
				scene->playerCarryState = CARRY_NONE;
				scene->potSeedTransforms[scene->carriedItemIndex].position.z
					= get_terrain_height(&scene->collisionSystem, scene->potSeedTransforms[scene->carriedItemIndex].position.xy);
				break;
	
			case CARRY_WATER:
			{
				Transform3D & waterTransform = scene->waters.transforms[scene->carriedItemIndex];

				scene->playerCarryState = CARRY_NONE;
				waterTransform.position.z = get_terrain_height(&scene->collisionSystem, waterTransform.position.xy);

				constexpr f32 waterSnapDistance 	= 0.5f;
				constexpr f32 smallPotMaxWaterLevel = 1.0f;

				f32 droppedWaterLevel = scene->waters.levels[scene->carriedItemIndex];

				for (s32 i = 0; i < scene->potSeedCount; ++i)
				{
					f32 distance = magnitude_v3(scene->potSeedTransforms[i].position - waterTransform.position);
					if (distance < waterSnapDistance)
					{
						scene->potSeedWaterLevels[i] += droppedWaterLevel;
						scene->potSeedWaterLevels[i] = min_f32(scene->potSeedWaterLevels[i] + droppedWaterLevel, smallPotMaxWaterLevel);

						scene->waters.count -= 1;
						scene->waters.transforms[scene->carriedItemIndex] 	= scene->waters.transforms[scene->waters.count];
						scene->waters.levels[scene->carriedItemIndex] 		= scene->waters.levels[scene->waters.count];

						break;
					}
				}

				for (s32 i = 0; i < scene->potTreeCount; ++i)
				{
					f32 distance = magnitude_v3(scene->potTreeTransforms[i].position - waterTransform.position);
					if (distance < waterSnapDistance)
					{
						scene->potTreeWaterLevels[i] += droppedWaterLevel;
						scene->potTreeWaterLevels[i] = min_f32(scene->potTreeWaterLevels[i] + droppedWaterLevel, smallPotMaxWaterLevel);

						scene->waters.count -= 1;
						scene->waters.transforms[scene->carriedItemIndex] 	= scene->waters.transforms[scene->waters.count];
						scene->waters.levels[scene->carriedItemIndex] 		= scene->waters.levels[scene->waters.count];

						break;	
					}
				}


			} break;

			case CARRY_SEED:
				scene->playerCarryState = CARRY_NONE;
				scene->seedTransforms[scene->carriedItemIndex].position.z = get_terrain_height(&scene->collisionSystem, scene->seedTransforms[scene->carriedItemIndex].position.xy);

				// s32 firstEmptyPot = scene->potsWithTreeCount + scene->potsWithSeedCount;
				for (s32 i = 0; i < scene->potEmptyCount; ++i)
				{
					f32 distance = magnitude_v3(scene->potEmptyTransforms[i].position - scene->seedTransforms[scene->carriedItemIndex].position);
					if(distance < 0.5f)
					{
						/* Note(Leo): copy empty pot's transform into tree pots' array. Then pop pot from
						empty list and seed from seed list */
						scene->potSeedTransforms[scene->potSeedCount] = scene->potEmptyTransforms[i];
						scene->potSeedCount += 1;

						scene->potEmptyCount -=1;
						scene->potEmptyTransforms[i] = scene->potEmptyTransforms[scene->potEmptyCount];

						scene->seedCount -= 1;
						scene->seedTransforms[scene->carriedItemIndex] = scene->seedTransforms[scene->seedCount];

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
		case CARRY_POT_EMPTY:
			scene->potEmptyTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->potEmptyTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_POT_TREE:
			scene->potTreeTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->potTreeTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_POT_SEED:
			scene->potSeedTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->potSeedTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_WATER:
			scene->waters.transforms[scene->carriedItemIndex].position = carriedPosition;
			scene->waters.transforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_SEED:
			scene->seedTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->seedTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		default:
			break;
	}

	/// UPDATE TREES IN POTS POSITIONS
	for (s32 i = 0; i < scene->lSystemsInPotCount; ++i)
	{
		scene->lSystemsInPot[i].position = multiply_point(transform_matrix(scene->potTreeTransforms[i]), v3{0, 0, 0.25});
		scene->lSystemsInPot[i].rotation = scene->potTreeTransforms[i].rotation;
	}
	
	/// UPDATE SEEDS
	{
		constexpr f32 seedWaterDistanceThreshold 	= 0.4f;
		constexpr f32 seedWaterDrainSpeed 			= 0.5;
		constexpr f32 seedFullWaterLevel 			= 0.5f;
	
		for (s32 i = 0; i < scene->seedCount; ++i)
		{
			f32 waterAmount = get_water(scene->waters,
										scene->seedTransforms[i].position,
										seedWaterDistanceThreshold,
										scaledTime * seedWaterDrainSpeed);
			scene->seedWaterLevels[i] += waterAmount;

			if (scene->seedWaterLevels[i] > seedFullWaterLevel)
			{
				add_l_system_tree(scene->seedTransforms[i].position);

				scene->seedCount -= 1;
				scene->seedTransforms[i] = scene->seedTransforms[scene->seedCount];
				scene->seedWaterLevels[i] = scene->seedWaterLevels[scene->seedCount];

				debug_draw_circle_xy(scene->seedTransforms[i].position + v3{0,0,0.1f}, seedWaterDistanceThreshold, colour_bright_green, DEBUG_NPC);
			}
			else
			{
				debug_draw_circle_xy(scene->seedTransforms[i].position + v3{0,0,0.1f}, seedWaterDistanceThreshold, colour_bright_red, DEBUG_NPC);
			}

		}

		for (s32 i = 0; i < scene->potSeedCount; ++i)
		{
			f32 requestedWaterAmount 		= seedWaterDrainSpeed * scaledTime;
			f32 acquiredWaterAmount 		= min_f32(scene->potSeedWaterLevels[i], requestedWaterAmount);
			scene->potSeedWaterLevels[i] 	-= acquiredWaterAmount;
			scene->seedsInPotWaterLevels[i] += acquiredWaterAmount;

			if (scene->seedsInPotWaterLevels[i] > seedFullWaterLevel)
			{
				// Todo(Leo): separate trees in pot and on ground
				add_l_system_tree_in_pot(scene->potSeedTransforms[i].position);

				scene->potTreeTransforms[scene->potTreeCount] = scene->potSeedTransforms[i];
				scene->potTreeCount += 1;

				// Todo(Leo): There is duplicate information here...
				scene->potSeedCount -= 1;
				scene->potSeedTransforms[i] 	= scene->potSeedTransforms[scene->potSeedCount];
				scene->potSeedWaterLevels[i] 	= scene->potSeedWaterLevels[scene->potSeedCount];
				scene->seedsInPotWaterLevels[i] = scene->seedsInPotWaterLevels[scene->potSeedCount];
			}

		}

	}

	// -----------------------------------------------------------------------------------------------------------

	// Todo(Leo): Rather use something like submit_collider() with what every collider can decide themselves, if they want to
	// contribute to collisions this frame or not.
	precompute_colliders(scene->collisionSystem);

	/// SUBMIT COLLIDERS
	{
		// Todo(Leo): Maybe make something like that these would have predetermined range, that is only updated when
		// tree has grown a certain amount or somthing. These are kinda semi-permanent by nature.

		auto submit_cylinder_colliders = [&collisionSystem = scene->collisionSystem](f32 radius, f32 halfHeight, s32 count, Transform3D * transforms)
		{
			for (s32 i = 0; i < count; ++i)
			{
				submit_cylinder_collider(collisionSystem, radius, halfHeight, transforms[i].position + v3{0,0,halfHeight});
			}
		};

		f32 smallPotColliderRadius = 0.3;
		f32 smallPotColliderHeight = 0.58;
		f32 smallPotHalfHeight = smallPotColliderHeight / 2;
		submit_cylinder_colliders(smallPotColliderRadius, smallPotHalfHeight, scene->potEmptyCount, scene->potEmptyTransforms);
		submit_cylinder_colliders(smallPotColliderRadius, smallPotHalfHeight, scene->potSeedCount, scene->potSeedTransforms);
		submit_cylinder_colliders(smallPotColliderRadius, smallPotHalfHeight, scene->potTreeCount, scene->potTreeTransforms);

		constexpr f32 baseRadius = 0.12;
		constexpr f32 baseHeight = 2;

	}

	for (auto & randomWalker : scene->randomWalkers)
	{
		update_random_walker_input(randomWalker, scene->characterInputs, scaledTime);
	}

	for (auto & follower : scene->followers)
	{
		update_follower_input(follower, scene->characterInputs, scaledTime);
	}
	
	for (int i = 0; i < 1; ++i)
	// for (int i = 0; i < scene->characterMotors.count(); ++i)
	{
		update_character_motor(	scene->characterMotors[i],
								scene->characterInputs[i],
								scene->collisionSystem,
								scaledTime,
								i == scene->playerInputState.inputArrayIndex ? DEBUG_PLAYER : DEBUG_NPC);
	}


	for (s32 i = 0; i < 1; ++i)
	// for (s32 i = 0; i < scene->skeletonAnimators.count; ++i)
	{
		update_skeleton_animator(scene->skeletonAnimators[i], scaledTime);
	}

	// Rebellious(Leo): stop using systems, and instead just do the stuff you need to >:)
	/// DRAW UNUSED WATER
	if (scene->waters.count > 0)
	{
		m44 * waterTransforms = push_memory<m44>(*global_transientMemory, scene->waters.count, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < scene->waters.count; ++i)
		{
			waterTransforms[i] = transform_matrix(	scene->waters.transforms[i].position,
													scene->waters.transforms[i].rotation,
													make_uniform_v3(scene->waters.levels[i] / scene->fullWaterLevel));
		}
		platformApi->draw_meshes(platformGraphics, scene->waters.count, waterTransforms, scene->dropMesh, scene->waterDropMaterial);
	}
	
	/// DRAW UNUSED SEEDS
	if(scene->seedCount)
	{
		m44 * seedTransforms = push_memory<m44>(*global_transientMemory, scene->seedCount, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < scene->seedCount; ++i)
		{
			seedTransforms[i] = transform_matrix(scene->seedTransforms[i]);
		}
		platformApi->draw_meshes(platformGraphics, scene->seedCount, seedTransforms, scene->dropMesh, scene->seedMaterial);
	}

	/// DRAW POTS
	{
		auto append_matrices = [](s32 count, Transform3D const * transforms, m44 * targetMatrices)
		{
			for (s32 i = 0; i < count; ++i)
			{
				targetMatrices[i] = transform_matrix(transforms[i]);
			}
		};

		s32 totalPotCount = scene->potEmptyCount + scene->potTreeCount + scene->potSeedCount;
		m44 * potTransformMatrices = push_memory<m44>(*global_transientMemory, totalPotCount, ALLOC_NO_CLEAR);

		append_matrices(scene->potEmptyCount, scene->potEmptyTransforms, potTransformMatrices);
		append_matrices(scene->potTreeCount, scene->potTreeTransforms, potTransformMatrices + scene->potEmptyCount);
		append_matrices(scene->potSeedCount, scene->potSeedTransforms, potTransformMatrices + scene->potEmptyCount + scene->potTreeCount);

		platformApi->draw_meshes(platformGraphics, totalPotCount, potTransformMatrices, scene->potMesh, scene->potMaterial);
	}

	/// DRAW SEEDS INSIDE POTS
	if (scene->potSeedCount > 0)
	{
		m44 * seedsInPotTransforms = push_memory<m44>(*global_transientMemory, scene->potSeedCount, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < scene->potSeedCount; ++i)
		{
			seedsInPotTransforms[i] = transform_matrix(scene->potSeedTransforms[i]);
		}
		platformApi->draw_meshes(platformGraphics, scene->potSeedCount, seedsInPotTransforms, scene->dropMesh, scene->seedMaterial);
	}

	/// DRAW STATIC SCENERY
	{
		platformApi->draw_meshes(platformGraphics, scene->stoneWallCount, scene->stoneWallTransforms, scene->stoneWallMesh, scene->stoneWallMaterial);
		platformApi->draw_meshes(platformGraphics, scene->buildingCount, scene->buildingTransforms, scene->buildingMesh, scene->buildingMaterial);
		platformApi->draw_meshes(platformGraphics, scene->gateCount, scene->gateTransforms, scene->gateMesh, scene->gateMaterial);
		platformApi->draw_meshes(platformGraphics, scene->cubePyramidCount, scene->cubePyramidTransforms, scene->cubePyramidMesh, scene->cubePyramidMaterial);

		for(s32 i = 0; i < scene->terrainCount; ++i)
		{
			platformApi->draw_meshes(platformGraphics, 1, scene->terrainTransforms + i, scene->terrainMeshes[i], scene->terrainMaterial);
		}

		platformApi->draw_meshes(platformGraphics, 1, &scene->seaTransform, scene->seaMesh, scene->seaMaterial);

		scene3d_draw_monuments(scene->monuments);
	}


	/// DEBUG DRAW COLLIDERS
	{
		for (auto const & collider : scene->collisionSystem.precomputedBoxColliders)
		{
			debug_draw_box(collider.transform, colour_muted_green, DEBUG_BACKGROUND);
		}

		for (auto const & collider : scene->collisionSystem.staticBoxColliders)
		{
			debug_draw_box(collider.transform, colour_dark_green, DEBUG_BACKGROUND);
		}

		for (auto const & collider : scene->collisionSystem.precomputedCylinderColliders)
		{
			debug_draw_circle_xy(collider.center - v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green, DEBUG_BACKGROUND);
			debug_draw_circle_xy(collider.center + v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green, DEBUG_BACKGROUND);
		}

		for (auto const & collider : scene->collisionSystem.submittedCylinderColliders)
		{
			debug_draw_circle_xy(collider.center - v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green, DEBUG_BACKGROUND);
			debug_draw_circle_xy(collider.center + v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green, DEBUG_BACKGROUND);
		}

		debug_draw_circle_xy(scene->playerCharacterTransform->position + v3{0,0,0.7}, 0.25f, colour_bright_green, DEBUG_PLAYER);
	}

	  //////////////////////////////
	 /// 	RENDERING 			///
	//////////////////////////////
	/*
		TODO(LEO): THIS COMMENT PIECE IS NOT ACCURATE ANYMORE

		1. Update camera and lights
		2. Render normal models
		3. Render animated models
	*/

	for (auto & renderer : scene->renderers)
	{
		platformApi->draw_model(platformGraphics, renderer.model,
								transform_matrix(*renderer.transform),
								renderer.castShadows,
								nullptr, 0);
	}


	// for (auto & renderer : scene->animatedRenderers)
	{
		auto & renderer = scene->animatedRenderers[0];

		m44 boneTransformMatrices [32];

		update_animated_renderer(boneTransformMatrices, renderer.skeleton->bones);

		platformApi->draw_model(platformGraphics, renderer.model, transform_matrix(*renderer.transform),
								renderer.castShadows, boneTransformMatrices, array_count(boneTransformMatrices));
	}

	/// UPDATE LSYSTEM TREES
	{
		// TODO(Leo): This is stupid, the two use cases are too different, and we have to capture too much stuff :(

		local_persist s32 persist_updatedLSystemIndex = -1;
		persist_updatedLSystemIndex += 1;
		persist_updatedLSystemIndex %= 10;
		s32 updatedLSystemIndex = persist_updatedLSystemIndex;

		auto update_lsystem = [	&waters 			= scene->waters,
								potTreeWaterLevels 	= scene->potTreeWaterLevels,
								get_water, 
								get_water_from_pot,
								updatedLSystemIndex,
								scaledTime]
							(s32 count, TimedLSystem * lSystems, Leaves * leavess, bool isInPot)
		{
			for (int i = 0; i < count; ++i)
			{
				leavess[i].position = lSystems[i].position;
				leavess[i].rotation = lSystems[i].rotation;


				f32 waterDistanceThreshold = 1;
				debug_draw_circle_xy(lSystems[i].position + v3{0,0,1}, waterDistanceThreshold, colour_aqua_blue, DEBUG_NPC);

				if (lSystems[i].totalAge > lSystems[i].maxAge)
				{
					continue;
				}

				lSystems[i].timeSinceLastUpdate += scaledTime;
				if ((i % 10) == updatedLSystemIndex)
				{
					float timePassed 			= lSystems[i].timeSinceLastUpdate;
					lSystems[i].timeSinceLastUpdate = 0;

					constexpr f32 treeWaterDrainSpeed = 0.5f;

					f32 waterLevel = 	isInPot ?
										get_water_from_pot(potTreeWaterLevels, i, treeWaterDrainSpeed * scaledTime) : 
										get_water (waters, lSystems[i].position, waterDistanceThreshold, treeWaterDrainSpeed * scaledTime);

					if (waterLevel > 0)
					{
						advance_lsystem_time(lSystems[i], waters, timePassed);
						update_lsystem_mesh(lSystems[i], leavess[i]);
					}
				}

			}			
		};

		update_lsystem(scene->lSystemCount, scene->lSystems, scene->lSystemLeavess, false);
		update_lsystem(scene->lSystemsInPotCount, scene->lSystemsInPot, scene->lSystemsInPotLeavess, true);

		auto draw_l_system_trees = [material = scene->lSystemTreeMaterial](s32 count, TimedLSystem * lSystems)
		{
			for (s32 i = 0; i < count; ++i)
			{
				if (lSystems[i].vertices.count > 0 && lSystems[i].indices.count > 0)
				{
					mesh_generate_tangents(	lSystems[i].vertices.count, lSystems[i].vertices.data,
											lSystems[i].indices.count, lSystems[i].indices.data);

					platformApi->draw_procedural_mesh( 	platformGraphics,
														lSystems[i].vertices.count, lSystems[i].vertices.data,
														lSystems[i].indices.count, lSystems[i].indices.data,
														transform_matrix(lSystems[i].position, lSystems[i].rotation, make_uniform_v3(1)),
														material);
				}
			}
		};

		draw_l_system_trees(scene->lSystemCount, scene->lSystems);		
		draw_l_system_trees(scene->lSystemsInPotCount, scene->lSystemsInPot);		



		if (scene->lSystemCount > 0)
		{
			auto & lSystem = scene->lSystems[0];
			s32 vertexCount = lSystem.vertices.count;

			v3 * points = push_memory<v3>(*global_transientMemory, vertexCount * 2, ALLOC_NO_CLEAR);

			for (s32 i = 0; i < vertexCount; ++i)
			{
				points [i * 2] = lSystem.vertices.data[i].position + lSystem.position;
				points [i * 2 + 1] = lSystem.vertices.data[i].position + lSystem.position + lSystem.vertices.data[i].normal * 0.4f;
			}


			if (vertexCount > 0)
			{
				debug_draw_lines(vertexCount * 2, points, colour_bright_green, DEBUG_PLAYER);
			}
		}
	}

	/// DRAW LEAVES FOR BOTH LSYSTEM ARRAYS IN THE END BECAUSE OF LEAF SHDADOW INCONVENIENCE
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this unconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this unconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this unconvenience!
	// Todo(Leo): This works ok currently, but do fix this, maybe do a proper command buffer for these
	{
		auto draw_l_system_tree_leaves = [scaledTime](s32 count, Leaves * leavess)
		{
			for (s32 i = 0; i < count; ++i)
			{
				if (leavess[i].count > 0)
				{
					draw_leaves(leavess[i], scaledTime);
				}
			}
		};

		draw_l_system_tree_leaves(scene->lSystemCount, scene->lSystemLeavess);
		draw_l_system_tree_leaves(scene->lSystemsInPotCount, scene->lSystemsInPotLeavess);
	}

	// ------------------------------------------------------------------------

	if (is_clicked(input.start))
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
		{
			gui_position({900, 300});
			gui_background_image(scene->guiPanelImage, 3, scene->guiPanelColour);

			gui_text("Exit to Main Menu?");

			if (gui_button("Yes"))
			{
				keepScene = false;
			}

			if (gui_button("No"))
			{
				scene->menuView = Scene3d::MENU_MAIN;
				gui_reset_selection();
			}
		} break;

		case Scene3d::MENU_MAIN:
		{
			gui_position({900, 300});	
			gui_background_image(scene->guiPanelImage, 7, scene->guiPanelColour);

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

			if (gui_button("Exit Scene"))
			{
				scene->menuView = Scene3d::MENU_CONFIRM_EXIT;
				gui_reset_selection();
			}
		} break;
	
		case Scene3d::MENU_CONFIRM_TELEPORT:
		{
			gui_position({900, 300});
			gui_background_image(scene->guiPanelImage, 3, scene->guiPanelColour);

			gui_text("Teleport Player Here?");

			if (gui_button("Yes"))
			{
				scene->playerCharacterTransform->position = scene->freeCamera.position;
				scene->menuView = Scene3d::MENU_OFF;
				scene->cameraMode = CAMERA_MODE_PLAYER;
			}

			if (gui_button("No"))
			{
				scene->menuView = Scene3d::MENU_OFF;
			}
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

	gui_position({0, 0});

	{
		s32 elapsedMilliseconds 	= scaledTime * 1000;
		char elapsedTimeCString [] 	= "XXX";
		elapsedTimeCString[0]		= '0' + elapsedMilliseconds / 100;
		elapsedTimeCString[1] 		= '0' + ((elapsedMilliseconds / 10) % 10);
		elapsedTimeCString[2] 		= '0' + elapsedMilliseconds % 10;

		gui_text(elapsedTimeCString);
	}

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
	scene->gui.textColor 			= colour_white;
	scene->gui.selectedTextColor 	= colour_muted_red;
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
		TextureAsset testGuiAsset 	= load_texture_asset( "assets/textures/tiles.png", global_transientMemory);
		scene->guiPanelImage 		= platformApi->push_gui_texture(platformGraphics, &testGuiAsset);
		scene->guiPanelColour 		= colour_rgb_alpha(colour_aqua_blue.rgb, 0.5);
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
	TextureAsset neutralBumpTextureAsset 	= make_texture_asset(allocate_array<u32>(*global_transientMemory, {color_rgba_32(colour_bump)}), 1, 1, 4);
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
		auto tilesAlbedo 	= load_and_push_texture("assets/textures/tiles.jpg");
		auto tilesNormal 	= load_and_push_texture("assets/textures/tiles_normal.png");
		auto groundAlbedo 	= load_and_push_texture("assets/textures/ground.png");
		auto groundNormal 	= load_and_push_texture("assets/textures/ground_normal.png");
		auto redTilesAlbedo	= load_and_push_texture("assets/textures/tiles_red.png");
		auto faceTexture 	= load_and_push_texture("assets/textures/texture.jpg");


		materials =
		{
			.character 		= push_material(GRAPHICS_PIPELINE_ANIMATED, redTilesAlbedo, tilesNormal, blackTexture),
			.environment 	= push_material(GRAPHICS_PIPELINE_NORMAL, tilesAlbedo, tilesNormal, blackTexture),
			.ground 		= push_material(GRAPHICS_PIPELINE_NORMAL, groundAlbedo, groundNormal, blackTexture),
		};
		
		// Note(Leo): internet told us vulkan(or glsl) cubemap face order is as follows:
		// (+X,-X,+Y,-Y,+Z,-Z).
		// ... seems to wrock
		// StaticArray<TextureAsset,6> skyboxTextureAssets =
		// {
		// 	load_texture_asset("assets/textures/miramar_rt.png", global_transientMemory),
		// 	load_texture_asset("assets/textures/miramar_lf.png", global_transientMemory),
		// 	load_texture_asset("assets/textures/miramar_ft.png", global_transientMemory),
		// 	load_texture_asset("assets/textures/miramar_bk.png", global_transientMemory),
		// 	load_texture_asset("assets/textures/miramar_up.png", global_transientMemory),
		// 	load_texture_asset("assets/textures/miramar_dn.png", global_transientMemory),
		// };
		// auto skyboxTexture 	= platformApi->push_cubemap(platformGraphics, &skyboxTextureAssets);
		materials.sky 		= platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_SKYBOX, 0, nullptr);
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

		auto const gltfFile 		= read_gltf_file(*global_transientMemory, "assets/models/cube_head_v4.glb");

		// Exporting animations from blender is not easy, this file has working animations
		auto const animationFile 	= read_gltf_file(*global_transientMemory, "assets/models/cube_head_v3.glb");

		auto girlMeshAsset 	= load_mesh_glb(*global_transientMemory, gltfFile, "cube_head");
		auto girlMesh 		= platformApi->push_mesh(platformGraphics, &girlMeshAsset);

		// --------------------------------------------------------------------

		Transform3D * playerTransform 	= scene->transforms.push_return_pointer({10, 0, 5});
		scene->playerCharacterTransform = playerTransform;

		s32 motorIndex = scene->characterMotors.count();
		scene->playerInputState.inputArrayIndex = motorIndex;
		auto * motor = scene->characterMotors.push_return_pointer();
		motor->transform = playerTransform;

		{
			using namespace CharacterAnimations;			

			auto startTime = platformApi->current_time();

			scene->characterAnimations[WALK] 	= load_animation_glb(persistentMemory, animationFile, "Walk");
			scene->characterAnimations[RUN] 	= load_animation_glb(persistentMemory, animationFile, "Run");
			scene->characterAnimations[IDLE] 	= load_animation_glb(persistentMemory, animationFile, "Idle");
			scene->characterAnimations[JUMP]	= load_animation_glb(persistentMemory, animationFile, "JumpUp");
			scene->characterAnimations[FALL]	= load_animation_glb(persistentMemory, animationFile, "JumpDown");
			scene->characterAnimations[CROUCH] 	= load_animation_glb(persistentMemory, animationFile, "Crouch");

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

	scene->worldCamera 				= make_camera(60, 0.1f, 1000.0f);
	scene->playerCamera 			= {};
	scene->playerCamera.baseOffset 	= {0, 0, 2};
	scene->playerCamera.distance 	= 5;

	// Environment
	{
		{
			constexpr f32 mapSize 		= 1200;
			constexpr f32 minTerrainElevation = -5;
			constexpr f32 maxTerrainElevation = 50;

			// Note(Leo): this is maximum size we support with u16 mesh vertex indices
			s32 chunkResolution			= 128;
			
			s32 chunkCountPerDirection 	= 10;
			f32 chunkSize 				= 1.0f / chunkCountPerDirection;
			f32 chunkWorldSize 			= chunkSize * mapSize;

			push_memory_checkpoint(*global_transientMemory);

			s32 heightmapResolution = 1024;
			auto heightmapTexture 	= load_texture_asset("assets/textures/heightmap_island.png", global_transientMemory);
			auto heightmap 			= make_heightmap(&persistentMemory, &heightmapTexture, heightmapResolution, mapSize, minTerrainElevation, maxTerrainElevation);

			pop_memory_checkpoint(*global_transientMemory);

			s32 terrainCount 			= chunkCountPerDirection * chunkCountPerDirection;
			scene->terrainCount 		= terrainCount;
			scene->terrainTransforms 	= push_memory<m44>(persistentMemory, terrainCount, ALLOC_NO_CLEAR);
			scene->terrainMeshes 		= push_memory<MeshHandle>(persistentMemory, terrainCount, ALLOC_NO_CLEAR);
			scene->terrainMaterial 		= materials.ground;

			/// GENERATE GROUND MESH
			{
				push_memory_checkpoint(*global_transientMemory);
			
				for (s32 i = 0; i < terrainCount; ++i)
				{
					s32 x = i % chunkCountPerDirection;
					s32 y = i / chunkCountPerDirection;

					v2 position = { x * chunkSize, y * chunkSize };
					v2 size 	= { chunkSize, chunkSize };


					auto groundMeshAsset 	= generate_terrain(*global_transientMemory, heightmap, position, size, chunkResolution, 10);
					scene->terrainMeshes[i] = platformApi->push_mesh(platformGraphics, &groundMeshAsset);

				}
			
				pop_memory_checkpoint(*global_transientMemory);
			}

			f32 halfMapSize = mapSize / 2;
			v3 terrainOrigin = {-halfMapSize, -halfMapSize, 0};

			for (s32 i = 0; i < terrainCount; ++i)
			{
				s32 x = i % chunkCountPerDirection;
				s32 y = i / chunkCountPerDirection;

				v3 leftBackCornerPosition = {x * chunkWorldSize - halfMapSize, y * chunkWorldSize - halfMapSize, 0};
				scene->terrainTransforms[i] = translation_matrix(leftBackCornerPosition);
			}

			auto transform = scene->transforms.push_return_pointer({{-mapSize / 2, -mapSize / 2, 0}});

			scene->collisionSystem.terrainCollider 	= std::move(heightmap);
			scene->collisionSystem.terrainTransform = transform;


			MeshAsset seaMeshAsset = {};
			{
				seaMeshAsset.vertices = allocate_array<Vertex>(*global_transientMemory, 
				{
					{-0.5, -0.5, 0, 0, 0, 1, 1,1,1, 0, 0},
					{ 0.5, -0.5, 0, 0, 0, 1, 1,1,1, 1, 0},
					{-0.5,  0.5, 0, 0, 0, 1, 1,1,1, 0, 1},
					{ 0.5,  0.5, 0, 0, 0, 1, 1,1,1, 1, 1},
				});
				seaMeshAsset.indices = allocate_array<u16>(*global_transientMemory,
				{
					0, 1, 2, 2, 1, 3
				});
				seaMeshAsset.indexType = IndexType::UInt16;
			}
			mesh_generate_tangents(seaMeshAsset);
			scene->seaMesh 			= platformApi->push_mesh(platformGraphics, &seaMeshAsset);
			scene->seaMaterial 		= push_material(GRAPHICS_PIPELINE_NORMAL, waterTextureHandle, neutralBumpTexture, blackTexture);
			scene->seaTransform 	= transform_matrix({0,0,0}, identity_quaternion, {mapSize, mapSize, 1});
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


		scene->monuments = scene3d_load_monuments(persistentMemory, materials.environment, scene->collisionSystem);

		// Test robot
		{
			v3 position 			= {21, 10, 0};
			position.z 				= get_terrain_height(&scene->collisionSystem, position.xy);
			auto * transform 		= scene->transforms.push_return_pointer({.position = position});

			char const * filename 	= "assets/models/Robot53.glb";
			auto meshAsset 			= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, filename), "model_rigged");	
			auto mesh 				= platformApi->push_mesh(platformGraphics, &meshAsset);

			auto albedo 			= load_and_push_texture("assets/textures/Robot_53_albedo_4k.png");
			auto normal 			= load_and_push_texture("assets/textures/Robot_53_normal_4k.png");
			auto material 			= push_material(GRAPHICS_PIPELINE_NORMAL, albedo, normal, blackTexture);

			auto model 				= push_model(mesh, material);
			scene->renderers.push({transform, model});
		}

		{
			auto sceneryFile 		= read_gltf_file(*global_transientMemory, "assets/models/scenery.glb");
			
			auto smallPotMeshAsset 	= load_mesh_glb(*global_transientMemory, sceneryFile, "small_pot");
			scene->potMesh 			= platformApi->push_mesh(platformGraphics, &smallPotMeshAsset);
			scene->potMaterial 		= materials.environment;

			scene->potCapacity = 10;

			scene->potEmptyTransforms 	= push_memory<Transform3D>(persistentMemory, scene->potCapacity, ALLOC_NO_CLEAR);
			scene->potSeedTransforms 	= push_memory<Transform3D>(persistentMemory, scene->potCapacity, ALLOC_NO_CLEAR);
			scene->potTreeTransforms 	= push_memory<Transform3D>(persistentMemory, scene->potCapacity, ALLOC_NO_CLEAR);

			scene->potEmptyWaterLevels 	= push_memory<f32>(persistentMemory, scene->potCapacity, 0);
			scene->potTreeWaterLevels 	= push_memory<f32>(persistentMemory, scene->potCapacity, 0);
			scene->potSeedWaterLevels 	= push_memory<f32>(persistentMemory, scene->potCapacity, 0);

			scene->seedsInPotWaterLevels = push_memory<f32>(persistentMemory, scene->potCapacity, 0);





			scene->potEmptyCount = scene->potCapacity;
			scene->potTreeCount = 0;
			scene->potSeedCount = 0;

			for(s32 i = 0; i < scene->potCapacity; ++i)
			{
				v3 position 					= {15, i * 5.0f, 0};
				position.z 						= get_terrain_height(&scene->collisionSystem, position.xy);
				scene->potEmptyTransforms[i]	= { .position = position };
			}

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
				f32 colliderHeight = 1.16;
				push_cylinder_collider(scene->collisionSystem, 0.6, colliderHeight, v3{0,0,colliderHeight / 2}, transform);
			}


			// ----------------------------------------------------------------	

			MeshAsset dropMeshAsset = load_mesh_glb(*global_transientMemory, sceneryFile, "water_drop");
			scene->dropMesh 		= platformApi->push_mesh(platformGraphics, &dropMeshAsset);

			TextureHandle waterTextures [] 	= {waterTextureHandle, neutralBumpTexture, blackTexture};
			scene->waterDropMaterial 		= platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, waterTextures);

			scene->waters.capacity 		= 200;
			scene->waters.count 		= 20;
			scene->waters.transforms 	= push_memory<Transform3D>(persistentMemory, scene->waters.capacity, 0);
			scene->waters.levels 		= push_memory<f32>(persistentMemory, scene->waters.capacity, 0);

			for (s32 i = 0; i < scene->waters.count; ++i)
			{
				v3 position = {RandomRange(-50, 50), RandomRange(-50, 50)};
				position.z 	= get_terrain_height(&scene->collisionSystem, position.xy);

				scene->waters.transforms[i] 	= { .position = position };
				scene->waters.levels[i] 		= scene->fullWaterLevel;
			}

			TextureHandle seedTextures [] = {seedTextureHandle, neutralBumpTexture, blackTexture};
			scene->seedMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, seedTextures);


			scene->seedCapacity 	= 50;
			scene->seedCount 		= 20;
			scene->seedTransforms 	= push_memory<Transform3D>(persistentMemory, scene->seedCapacity, ALLOC_NO_CLEAR);
			scene->seedWaterLevels 	= push_memory<f32>(persistentMemory, scene->seedCapacity, ALLOC_NO_CLEAR);
			for(s32 i = 0; i < scene->seedCount; ++i)
			{
				scene->seedTransforms[i] = identity_transform;

				v3 position = {RandomRange(-50, 50), RandomRange(-50, 50), 0};
				position.z = get_terrain_height(&scene->collisionSystem, position.xy);
				scene->seedTransforms[i].position = position;

				scene->seedWaterLevels[i] = 0;
			}
		}

		{
			auto file = read_gltf_file(*global_transientMemory, "assets/models/stonewalls.glb");

			// Stone Walls
			{
				auto meshAsset 	= load_mesh_glb(*global_transientMemory, file, "StoneWall.001");
				auto mesh 		= platformApi->push_mesh(platformGraphics, &meshAsset);

				auto albedo 	= load_and_push_texture("assets/textures/stone_wall.jpg");
				auto normal 	= load_and_push_texture("assets/textures/stone_wall_normal.png");
				auto material   = push_material(GRAPHICS_PIPELINE_NORMAL, albedo, normal, blackTexture);

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

			// Buildings
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

			// Gates
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

			// Pyramid thing
			{
				auto meshAsset = load_mesh_glb(*global_transientMemory, file, "CubePyramid");
				auto meshHandle = platformApi->push_mesh(platformGraphics, &meshAsset);

				Array<BoxCollider> colliders = {};
				auto transformsArray = load_all_transforms_glb(*global_transientMemory, file, "CubePyramid", &colliders);
				scene->cubePyramidCount = transformsArray.count();

				scene->cubePyramidTransforms = push_memory<m44>(persistentMemory, scene->cubePyramidCount, ALLOC_NO_CLEAR);
				for (s32 i = 0; i < scene->cubePyramidCount; ++i)
				{
					scene->cubePyramidTransforms[i] = transform_matrix(transformsArray[i]);
				}

				scene->cubePyramidMesh = meshHandle;

				auto textureAsset = load_texture_asset("assets/textures/concrete_XX.png", global_transientMemory);
				auto texture = platformApi->push_texture(platformGraphics, &textureAsset);


				TextureHandle textures [] = {texture, neutralBumpTexture, blackTexture};
				scene->cubePyramidMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, textures);

				for (auto collider : colliders)
				{
					scene->collisionSystem.staticBoxColliders.push({compute_collider_transform(collider),
																	compute_inverse_collider_transform(collider)});
				}
			}
		}
	}

	// ----------------------------------------------------------------------------------
	
	scene->lSystemCapacity 	= 60;
	scene->lSystemCount 	= 0;
	scene->lSystems 		= push_memory<TimedLSystem>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);
	scene->lSystemLeavess 	= push_memory<Leaves>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);

	scene->lSystemsInPotCount 	= 0;
	scene->lSystemsInPot 		= push_memory<TimedLSystem>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);
	scene->lSystemsInPotLeavess = push_memory<Leaves>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);

	scene->lSystemTreeMaterial = scene->seedMaterial;

	for (int i = 0; i < scene->lSystemCapacity; ++i)
	{
		scene->lSystems[i]			= make_lsystem(persistentMemory);
		scene->lSystemLeavess[i] 	= make_leaves(persistentMemory, 4000);
	
		scene->lSystemsInPot[i]			= make_lsystem(persistentMemory);
		scene->lSystemsInPotLeavess[i] 	= make_leaves(persistentMemory, 4000);
	}

	Assert(scene->lSystemCapacity >= scene->seedCapacity && "Not enough space for trees for all seeds.");

	// ----------------------------------------------------------------------------------


	logSystem(0) << "Scene3d loaded, " << used_percent(*global_transientMemory) * 100 << "% of transient memory used.";
	logSystem(0) << "Scene3d loaded, " << used_percent(persistentMemory) * 100 << "% of persistent memory used.";

	return scenePtr;
}
