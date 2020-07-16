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
	CARRY_POT,
	CARRY_WATER,
	CARRY_TREE,
};

struct Scene3d
{
	// Todo(Leo): Remove these "component" arrays, they are stupidly generic solution, that hide away actual data location, at least the way they are now used
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

	static constexpr f32 fullWaterLevel = 1;
	Waters 			waters;
	MeshHandle 		waterMesh;
	MaterialHandle 	waterMaterial;

	/// POTS -----------------------------------
		s32 			potCapacity;
		s32 			potEmptyCount;
		Transform3D * 	potEmptyTransforms;
		f32 * 			potEmptyWaterLevels;

		MeshHandle 		potMesh;
		MaterialHandle 	potMaterial;

	/// TREES ------------------------------------
		s32 			lSystemCapacity;
		s32 			lSystemCount;
		TimedLSystem * 	lSystems;
		Leaves * 		lSystemLeavess;
		s32 * 			lSystemsPotIndices;

		MaterialHandle 	lSystemTreeMaterial;
	// ------------------------------------------------------

	Monuments monuments;

	// ------------------------------------------------------


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
};

struct Scene3dSaveLoad
{
	s32 playerLocation;
	s32 watersLocation;
	s32 treesLocation;
};

internal void write_save_file(Scene3d * scene)
{
	auto file = platformApi->open_file("save_game.fssave", FILE_MODE_WRITE);

	// Note(Leo): as we go, set values to this struct, and write this to file last.
	Scene3dSaveLoad save;
	platformApi->set_file_position(file, sizeof(save));

	save.playerLocation = platformApi->get_file_position(file);
	platformApi->write_file(file, sizeof(Transform3D), scene->playerCharacterTransform);

	save.watersLocation = platformApi->get_file_position(file);
	platformApi->write_file(file, sizeof(scene->waters.capacity),				&scene->waters.capacity);
	platformApi->write_file(file, sizeof(scene->waters.count), 					&scene->waters.count);
	platformApi->write_file(file, scene->waters.count * sizeof(Transform3D), 	scene->waters.transforms);
	platformApi->write_file(file, scene->waters.count * sizeof(f32),			scene->waters.levels);

	save.treesLocation = platformApi->get_file_position(file);
	platformApi->write_file(file, sizeof(scene->lSystemCapacity), &scene->lSystemCapacity);
	platformApi->write_file(file, sizeof(scene->lSystemCount), &scene->lSystemCount);
	for (s32 i = 0; i < scene->lSystemCount; ++i)
	{
		platformApi->write_file(file, sizeof(scene->lSystems[i].wordCapacity), &scene->lSystems[i].wordCapacity);
		platformApi->write_file(file, sizeof(scene->lSystems[i].wordCount), &scene->lSystems[i].wordCount);
		platformApi->write_file(file, sizeof(Module) * scene->lSystems[i].wordCount, scene->lSystems[i].aWord);
		platformApi->write_file(file, sizeof(scene->lSystems[i].timeSinceLastUpdate), &scene->lSystems[i].timeSinceLastUpdate);
		platformApi->write_file(file, sizeof(scene->lSystems[i].position), &scene->lSystems[i].position);
		platformApi->write_file(file, sizeof(scene->lSystems[i].rotation), &scene->lSystems[i].rotation);
		platformApi->write_file(file, sizeof(scene->lSystems[i].type), &scene->lSystems[i].type);
		platformApi->write_file(file, sizeof(scene->lSystems[i].totalAge), &scene->lSystems[i].totalAge);
		platformApi->write_file(file, sizeof(scene->lSystems[i].maxAge), &scene->lSystems[i].maxAge);
	}

	platformApi->set_file_position(file, 0);
	platformApi->write_file(file, sizeof(save), &save);

	platformApi->close_file(file);
}

internal bool32 update_scene_3d(void * scenePtr, PlatformInput const & input, PlatformTime const & time)
{
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);
	
	/// ****************************************************************************
	/// DEFINE UPDATE FUNCTIONS
	// Todo(Leo): Get water functions are used in one place only, inline them
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

	/// ****************************************************************************
	/// TIME

	f32 const timeScales [scene->timeScaleCount] { 1.0f, 0.1f, 0.5f }; 
	f32 scaledTime 		= time.elapsedTime * timeScales[scene->timeScaleIndex];
	f32 unscaledTime 	= time.elapsedTime;

	/// ****************************************************************************

	gui_start_frame(scene->gui, input);

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
	

	/// SPAWN WATER
	if ((scene->cameraMode == CAMERA_MODE_PLAYER) && is_clicked(input.Y))
	{
		Waters & waters = scene->waters;

		v2 center = scene->playerCharacterTransform->position.xy;

		s32 spawnCount = 10;
		spawnCount = min_f32(spawnCount, waters.capacity - waters.count);

		for (s32 i = 0; i < spawnCount; ++i)
		{
			f32 distance 	= random_range(1, 5);
			f32 angle 		= random_range(0, 2 * π);

			f32 x = cosine(angle) * distance;
			f32 y = sine(angle) * distance;

			v3 position = { x + center.x, y + center.y, 0 };
			position.z = get_terrain_height(scene->collisionSystem, position.xy);

			waters.transforms[waters.count].position = position;
			waters.transforms[waters.count].rotation = identity_quaternion;
			waters.transforms[waters.count].scale = {1,1,1};

			waters.levels[waters.count] = scene->fullWaterLevel;

			waters.count += 1;
		}
	}

	/// PICKUP OR DROP
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

				check_pickup(scene->potEmptyCount, scene->potEmptyTransforms, CARRY_POT);
				check_pickup(scene->waters.count, scene->waters.transforms, CARRY_WATER);

				for (s32 i = 0; i < scene->lSystemCount; ++i)
				{
					auto & lSystem = scene->lSystems[i]; 
					if (lSystem.totalAge > 1)
					{
						continue;
					}

					if (scene->lSystemsPotIndices[i] >= 0)
					{
						continue;
					}

					f32 distanceToLSystemTree = magnitude_v3(lSystem.position - scene->playerCharacterTransform->position);
					if(distanceToLSystemTree < grabDistance)
					{
						scene->playerCarryState = CARRY_TREE;
						scene->carriedItemIndex = i;
					}
				}

			} break;

			case CARRY_POT:
				scene->playerCarryState = CARRY_NONE;
				scene->potEmptyTransforms[scene->carriedItemIndex].position.z
					= get_terrain_height(scene->collisionSystem, scene->potEmptyTransforms[scene->carriedItemIndex].position.xy);
				break;
	
			case CARRY_WATER:
			{
				Transform3D & waterTransform = scene->waters.transforms[scene->carriedItemIndex];

				scene->playerCarryState = CARRY_NONE;
				waterTransform.position.z = get_terrain_height(scene->collisionSystem, waterTransform.position.xy);

				constexpr f32 waterSnapDistance 	= 0.5f;
				constexpr f32 smallPotMaxWaterLevel = 1.0f;

				f32 droppedWaterLevel = scene->waters.levels[scene->carriedItemIndex];

				for (s32 i = 0; i < scene->potEmptyCount; ++i)
				{
					f32 distance = magnitude_v3(scene->potEmptyTransforms[i].position - waterTransform.position);
					if (distance < waterSnapDistance)
					{
						scene->potEmptyWaterLevels[i] += droppedWaterLevel;
						scene->potEmptyWaterLevels[i] = min_f32(scene->potEmptyWaterLevels[i] + droppedWaterLevel, smallPotMaxWaterLevel);

						scene->waters.count -= 1;
						scene->waters.transforms[scene->carriedItemIndex] 	= scene->waters.transforms[scene->waters.count];
						scene->waters.levels[scene->carriedItemIndex] 		= scene->waters.levels[scene->waters.count];

						break;	
					}
				}


			} break;

			case CARRY_TREE:
			{
				scene->playerCarryState = CARRY_NONE;
				scene->lSystems[scene->carriedItemIndex].position.z = get_terrain_height(scene->collisionSystem, scene->lSystems[scene->carriedItemIndex].position.xy);

				constexpr f32 treeSnapDistance = 0.5f;

				for (s32 potIndex = 0; potIndex < scene->potEmptyCount; ++potIndex)
				{
					f32 distance = magnitude_v3(scene->potEmptyTransforms[potIndex].position - scene->lSystems[scene->carriedItemIndex].position);
					if (distance < treeSnapDistance)
					{
						scene->lSystemsPotIndices[scene->carriedItemIndex] = potIndex;


						// scene->lSystemsInPot[scene->lSystemsInPotCount] 			= scene->lSystems[scene->carriedItemIndex];
						// scene->lSystemsInPotLeavess[scene->lSystemsInPotCount]		= scene->lSystemLeavess[scene->carriedItemIndex];
						// scene->lSystemsInPotPotIndices[scene->lSystemsInPotCount]	= potIndex;
						// scene->lSystemsInPotCount 									+= 1;

						// scene->lSystemCount 							-= 1;
						// scene->lSystems[scene->carriedItemIndex] 		= scene->lSystems[scene->lSystemCount];
						// scene->lSystemLeavess[scene->carriedItemIndex] 	= scene->lSystemLeavess[scene->lSystemCount];

					}
				}

			} break;
		}
	} // endif input


	v3 carriedPosition = multiply_point(transform_matrix(*scene->playerCharacterTransform), {0, 0.7, 0.7});
	quaternion carriedRotation = scene->playerCharacterTransform->rotation;
	switch(scene->playerCarryState)
	{
		case CARRY_POT:
			scene->potEmptyTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->potEmptyTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_WATER:
			scene->waters.transforms[scene->carriedItemIndex].position = carriedPosition;
			scene->waters.transforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_TREE:
			scene->lSystems[scene->carriedItemIndex].position = carriedPosition;
			scene->lSystems[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		default:
			break;
	}

	/// UPDATE TREES IN POTS POSITIONS
	for (s32 i = 0; i < scene->lSystemCount; ++i)
	{
		if (scene->lSystemsPotIndices[i] > 0)
		{
			s32 potIndex = scene->lSystemsPotIndices[i];
			scene->lSystems[i].position = multiply_point(transform_matrix(scene->potEmptyTransforms[potIndex]), v3{0,0,0.25});
			scene->lSystems[i].rotation = scene->potEmptyTransforms[potIndex].rotation;
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
	
	/// DRAW POTS
	{
		m44 * potTransformMatrices = push_memory<m44>(*global_transientMemory, scene->potEmptyCount, ALLOC_NO_CLEAR);
		for(s32 i = 0; i < scene->potEmptyCount; ++i)
		{
			potTransformMatrices[i] = transform_matrix(scene->potEmptyTransforms[i]);
		}
		platformApi->draw_meshes(platformGraphics, scene->potEmptyCount, potTransformMatrices, scene->potMesh, scene->potMaterial);
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

		auto update_lsystem = [	&waters 				= scene->waters,
								potEmptyWaterLevels 	= scene->potEmptyWaterLevels,
								lSystemsPotIndices 		= scene->lSystemsPotIndices,
								// lSystemsInPotPotIndices = scene->lSystemsInPotPotIndices,
								get_water, 
								get_water_from_pot,
								updatedLSystemIndex,
								scaledTime]
							(s32 count, TimedLSystem * lSystems, Leaves * leavess)// bool isInPot)
		{

			for (int i = 0; i < count; ++i)
			{
				bool isInPot = lSystemsPotIndices[i] > 0;

				leavess[i].position = lSystems[i].position;
				leavess[i].rotation = lSystems[i].rotation;

				{
					v3 position = leavess[i].position;
					if (position.x < 0)
					{
						if (position.y < 0)
						{
							leavess[i].colourIndex = 0;
						}
						else
						{
							leavess[i].colourIndex = 1;
						}
					}
					else
					{
						if (position.y < 0)
						{
							leavess[i].colourIndex = 2;
						}
						else
						{
							leavess[i].colourIndex = 3;
						}
					}
				}

				f32 waterDistanceThreshold = 1;
				debug_draw_circle_xy(lSystems[i].position + v3{0,0,1}, waterDistanceThreshold, colour_aqua_blue, DEBUG_NPC);

				if (lSystems[i].totalAge > lSystems[i].maxAge)
				{
					continue;
				}

				lSystems[i].timeSinceLastUpdate += scaledTime;
				if ((i % 10) == updatedLSystemIndex)
				{
					float timePassed 				= lSystems[i].timeSinceLastUpdate;
					lSystems[i].timeSinceLastUpdate = 0;

					constexpr f32 treeWaterDrainSpeed = 0.5f;

					f32 waterLevel = 	isInPot ?
										get_water_from_pot(potEmptyWaterLevels, lSystemsPotIndices[i], treeWaterDrainSpeed * timePassed) : 
										get_water (waters, lSystems[i].position, waterDistanceThreshold, treeWaterDrainSpeed * timePassed);

					if (waterLevel > 0)
					{
						if (lSystems[i].type == TREE_1)
						{
							advance_lsystem_time(lSystems[i], timePassed);
							update_lsystem_mesh(lSystems[i], leavess[i]);
						}
						else if(lSystems[i].type == TREE_2)
						{
							advance_lsystem_time_tree2(lSystems[i], timePassed);
							update_lsystem_mesh_tree2(lSystems[i], leavess[i]);
						}
					}
				}

			}			
		};

		update_lsystem(scene->lSystemCount, scene->lSystems, scene->lSystemLeavess);

		auto draw_l_system_trees = [material = scene->lSystemTreeMaterial] (s32 count, TimedLSystem * lSystems)
		{
			for (s32 i = 0; i < count; ++i)
			{
				m44 transform = transform_matrix(lSystems[i].position, lSystems[i].rotation, make_uniform_v3(1));

				if (lSystems[i].vertices.count > 0 && lSystems[i].indices.count > 0)
				{
					mesh_generate_tangents(	lSystems[i].vertices.count, lSystems[i].vertices.data,
											lSystems[i].indices.count, lSystems[i].indices.data);

					platformApi->draw_procedural_mesh( 	platformGraphics,
														lSystems[i].vertices.count, lSystems[i].vertices.data,
														lSystems[i].indices.count, lSystems[i].indices.data,
														transform,
														material);
				}

				// Todo(Leo): this is not maybe so nice anymore
				constexpr s32 randomTreeAgeToStopDrawingSeed = 1;
				if (lSystems[i].totalAge < randomTreeAgeToStopDrawingSeed)
				{
					platformApi->draw_meshes(platformGraphics, 1, &transform, lSystems[i].seedMesh, lSystems[i].seedMaterial);
				}
			}
		};

		draw_l_system_trees(scene->lSystemCount, scene->lSystems);		

		for (s32 i = 0; i < scene->lSystemCount; ++i)
		{
			m44 transform = transform_matrix(scene->lSystems[i].position, scene->lSystems[i].rotation, make_uniform_v3(1));
			constexpr s32 randomTreeAgeToStopDrawingSeed = 1;
			if (scene->lSystems[i].totalAge < randomTreeAgeToStopDrawingSeed)
			{
				platformApi->draw_meshes(platformGraphics, 1, &transform, scene->lSystems[i].seedMesh, scene->lSystems[i].seedMaterial);
			}
		}

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
		platformApi->draw_meshes(platformGraphics, scene->waters.count, waterTransforms, scene->waterMesh, scene->waterMaterial);
	}

	/// DRAW LEAVES FOR BOTH LSYSTEM ARRAYS IN THE END BECAUSE OF LEAF SHDADOW INCONVENIENCE
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this unconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this unconvenience!
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
	
	v4 menuColor = colour_rgb_alpha(colour_raw_umber.rgb, 0.5);
	switch(scene->menuView)
	{	
		case Scene3d::MENU_OFF:
			// Nothing to do
			break;

		case Scene3d::MENU_CONFIRM_EXIT:
		{
			gui_position({800, 300});

			gui_start_panel("Exit to Main Menu?", menuColor);

			if (gui_button("Yes"))
			{
				keepScene = false;
			}

			if (gui_button("No"))
			{
				scene->menuView = Scene3d::MENU_MAIN;
				gui_reset_selection();
			}

			gui_end_panel();
		} break;

		case Scene3d::MENU_MAIN:
		{
			gui_position({800, 300});	

			gui_start_panel("Menu", menuColor);


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

			if (gui_button("Save Game"))
			{
				write_save_file(scene);
			}

			if (gui_button("Exit Scene"))
			{
				scene->menuView = Scene3d::MENU_CONFIRM_EXIT;
				gui_reset_selection();
			}

			gui_end_panel();

		} break;
	
		case Scene3d::MENU_CONFIRM_TELEPORT:
		{
			gui_position({800, 300});

			gui_start_panel("Teleport Player Here?", menuColor);

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

void * load_scene_3d(MemoryArena & persistentMemory, PlatformFileHandle saveFile)
{
	Scene3dSaveLoad save;
	if (saveFile != nullptr)
	{
		logDebug(0) << "Loading saved game";

		platformApi->set_file_position(saveFile, 0);
		platformApi->read_file(saveFile, sizeof(save), &save);
	}

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

	TextureAsset guiTextureAsset 	= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xffffffff}), 1, 1, 4);
	scene->gui.panelTexture			= platformApi->push_gui_texture(platformGraphics, &guiTextureAsset);

	// ----------------------------------------------------------------------------------

	// TODO(Leo): these should probably all go away
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
		TextureAsset testGuiAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/tiles.png");
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
		auto asset = load_texture_asset(*global_transientMemory, path);
		auto result = platformApi->push_texture(platformGraphics, &asset);
		return result;
	};

	TextureAsset whiteTextureAsset 			= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xffffffff}), 1, 1, 4);
	TextureAsset blackTextureAsset	 		= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff000000}), 1, 1, 4);
	TextureAsset neutralBumpTextureAsset 	= make_texture_asset(allocate_array<u32>(*global_transientMemory, {color_rgba_32(colour_bump)}), 1, 1, 4);
	TextureAsset waterBlueTextureAsset 		= make_texture_asset(allocate_array<u32>(*global_transientMemory, {colour_rgb_alpha_32(colour_aqua_blue.rgb,0.7)}), 1, 1, 4);
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

		if (saveFile != nullptr)
		{
			logDebug(0) << "player position = " << playerTransform->position;

			platformApi->set_file_position(saveFile, save.playerLocation);
			platformApi->read_file(saveFile, sizeof(Transform3D), playerTransform);
			
			logDebug(0) << "player position = " << playerTransform->position;
		}

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
			v3 position = {random_range(-99, 99), random_range(-99, 99), 0};
			v3 scale 	= make_uniform_v3(random_range(0.8f, 1.5f));

			auto * transform 	= scene->transforms.push_return_pointer({.position = position, .scale = scale});
			s32 motorIndex 		= scene->characterMotors.count();
			auto * motor 		= scene->characterMotors.push_return_pointer();
			motor->transform 	= transform;

			scene->randomWalkers[randomWalkerIndex] = {transform, motorIndex, {random_range(-99, 99), random_range(-99, 99)}};

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
			v3 position 		= {random_range(-99, 99), random_range(-99, 99), 0};
			v3 scale 			= make_uniform_v3(random_range(0.8f, 1.5f));
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

	auto sceneryFile 		= read_gltf_file(*global_transientMemory, "assets/models/scenery.glb");

	// Environment
	{
		/// GROUND AND TERRAIN
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

			auto heightmapTexture 	= load_texture_asset(*global_transientMemory, "assets/textures/heightmap_island.png");
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

		/// TOTEMS
		{
			auto totemMesh 			= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, "assets/models/totem.glb"), "totem");
			auto totemMeshHandle 	= platformApi->push_mesh(platformGraphics, &totemMesh);
			auto model = push_model(totemMeshHandle, materials.environment);

			Transform3D * transform = scene->transforms.push_return_pointer({});
			transform->position.z = get_terrain_height(scene->collisionSystem, transform->position.xy) - 0.5f;
			scene->renderers.push({transform, model});
			push_box_collider(	scene->collisionSystem,
								v3 {1.0, 1.0, 5.0},
								v3 {0,0,2},
								transform);

			transform = scene->transforms.push_return_pointer({.position = {0, 5, 0} , .scale =  {0.5, 0.5, 0.5}});
			transform->position.z = get_terrain_height(scene->collisionSystem, transform->position.xy) - 0.25f;
			scene->renderers.push({transform, model});
			push_box_collider(	scene->collisionSystem,
								v3{0.5, 0.5, 2.5},
								v3{0,0,1},
								transform);
		}

		/// INVISIBLE TEST COLLIDER
		{
			Transform3D * transform = scene->transforms.push_return_pointer({});
			transform->position = {21, 15};
			transform->position.z = get_terrain_height(scene->collisionSystem, {20, 20});

			push_box_collider( 	scene->collisionSystem,
								v3{2.0f, 0.05f,5.0f},
								v3{0,0, 2.0f},
								transform);
		}

		scene->monuments = scene3d_load_monuments(persistentMemory, materials.environment, scene->collisionSystem);

		// TEST ROBOT
		{
			v3 position 			= {21, 10, 0};
			position.z 				= get_terrain_height(scene->collisionSystem, position.xy);
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

		/// SMALL SCENERY OBJECTS
		{			
			auto smallPotMeshAsset 	= load_mesh_glb(*global_transientMemory, sceneryFile, "small_pot");
			scene->potMesh 			= platformApi->push_mesh(platformGraphics, &smallPotMeshAsset);
			scene->potMaterial 		= materials.environment;

			scene->potCapacity 			= 10;
			scene->potEmptyCount 		= scene->potCapacity;
			scene->potEmptyTransforms 	= push_memory<Transform3D>(persistentMemory, scene->potCapacity, ALLOC_NO_CLEAR);
			scene->potEmptyWaterLevels 	= push_memory<f32>(persistentMemory, scene->potCapacity, 0);

			for(s32 i = 0; i < scene->potCapacity; ++i)
			{
				v3 position 					= {15, i * 5.0f, 0};
				position.z 						= get_terrain_height(scene->collisionSystem, position.xy);
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

				transform->position.z 	= get_terrain_height(scene->collisionSystem, transform->position.xy);

				scene->renderers.push({transform, model});
				f32 colliderHeight = 1.16;
				push_cylinder_collider(scene->collisionSystem, 0.6, colliderHeight, v3{0,0,colliderHeight / 2}, transform);
			}


			// ----------------------------------------------------------------	

			/// WATERS
			{
				MeshAsset dropMeshAsset = load_mesh_glb(*global_transientMemory, sceneryFile, "water_drop");
				scene->waterMesh 		= platformApi->push_mesh(platformGraphics, &dropMeshAsset);

				TextureHandle waterTextures [] 	= {waterTextureHandle, neutralBumpTexture, blackTexture};
				scene->waterMaterial 		= platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_WATER, 3, waterTextures);

				if (saveFile != nullptr)
				{
					platformApi->set_file_position(saveFile, save.watersLocation);
					platformApi->read_file(saveFile, sizeof(scene->waters.capacity), &scene->waters.capacity);
					platformApi->read_file(saveFile, sizeof(scene->waters.count), &scene->waters.count);

					scene->waters.transforms 	= push_memory<Transform3D>(persistentMemory, scene->waters.capacity, 0);
					scene->waters.levels 		= push_memory<f32>(persistentMemory, scene->waters.capacity, 0);

					platformApi->read_file(saveFile, sizeof(scene->waters.transforms[0]) * scene->waters.count, scene->waters.transforms);					
					platformApi->read_file(saveFile, sizeof(scene->waters.levels[0]) * scene->waters.count, scene->waters.levels);					
				}
				else
				{				
					scene->waters.capacity 		= 200;
					scene->waters.count 		= 20;
					scene->waters.transforms 	= push_memory<Transform3D>(persistentMemory, scene->waters.capacity, 0);
					scene->waters.levels 		= push_memory<f32>(persistentMemory, scene->waters.capacity, 0);

					for (s32 i = 0; i < scene->waters.count; ++i)
					{
						v3 position = {random_range(-50, 50), random_range(-50, 50)};
						position.z 	= get_terrain_height(scene->collisionSystem, position.xy);

						scene->waters.transforms[i] 	= { .position = position };
						scene->waters.levels[i] 		= scene->fullWaterLevel;
					}
				}
			}


		}

		/// BIG SCENERY THINGS
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

				auto textureAsset = load_texture_asset(*global_transientMemory, "assets/textures/concrete_XX.png");
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

	/// LSYSTEM TREES	
	{
		// Todo(Leo): I forgot this. what does this comment mean?
		// TODO(Leo): fukin häx
		MeshHandle seedMesh1;
		MeshHandle seedMesh2;

		MaterialHandle seedMaterial1;
		MaterialHandle seedMaterial2;

		/// SEEDS
		{
			MeshAsset seedMeshAsset 		= load_mesh_glb(*global_transientMemory, sceneryFile, "acorn");
			seedMesh1 						= platformApi->push_mesh(platformGraphics, &seedMeshAsset);
			seedMesh2 						= scene->waterMesh;

			TextureAsset seedAlbedoAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/Acorn_albedo.png");
			TextureHandle seedAlbedo 		= platformApi->push_texture(platformGraphics, &seedAlbedoAsset);
			TextureHandle seedTextures [] 	= {seedAlbedo, neutralBumpTexture, blackTexture};

			seedMaterial1 					= platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, seedTextures);
			seedMaterial2 					= seedMaterial1;
		}

		// Todo(Leo): textures are copied too many times: from file to stb, from stb to TextureAsset, from TextureAsset to graphics.
		TextureAsset albedo = load_texture_asset(*global_transientMemory, "assets/textures/Acorn_albedo.png");
		TextureAsset normal = load_texture_asset(*global_transientMemory, "assets/textures/ground_normal.png");

		TextureHandle treeTextures [] =
		{	
			platformApi->push_texture(platformGraphics, &albedo),
			platformApi->push_texture(platformGraphics, &normal),
			blackTexture
		};
		scene->lSystemTreeMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, treeTextures);

		MeshHandle tree1SeedMesh = seedMesh1;
		MeshHandle tree2SeedMesh = seedMesh2;

		MaterialHandle tree1SeedMaterial = seedMaterial1;
		MaterialHandle tree2SeedMaterial = seedMaterial2;


		if (saveFile != nullptr)
		{
			platformApi->set_file_position(saveFile, save.treesLocation);
			
			platformApi->read_file(saveFile, sizeof(scene->lSystemCapacity), &scene->lSystemCapacity);
			platformApi->read_file(saveFile, sizeof(scene->lSystemCount), &scene->lSystemCount);

			scene->lSystems 			= push_memory<TimedLSystem>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);
			scene->lSystemLeavess 		= push_memory<Leaves>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);
			scene->lSystemsPotIndices 	= push_memory<s32>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);

			for (s32 i = 0; i < scene->lSystemCapacity; ++i)
			{
				platformApi->read_file(saveFile, sizeof(scene->lSystems[i].wordCapacity), &scene->lSystems[i].wordCapacity);
				platformApi->read_file(saveFile, sizeof(scene->lSystems[i].wordCount), &scene->lSystems[i].wordCount);

				scene->lSystems[i].aWord = push_memory<Module>(persistentMemory, scene->lSystems[i].wordCapacity, ALLOC_NO_CLEAR);
				scene->lSystems[i].bWord = push_memory<Module>(persistentMemory, scene->lSystems[i].wordCapacity, ALLOC_NO_CLEAR);
				platformApi->read_file(saveFile, sizeof(Module) * scene->lSystems[i].wordCount, scene->lSystems[i].aWord);

				platformApi->read_file(saveFile, sizeof(scene->lSystems[i].timeSinceLastUpdate), &scene->lSystems[i].timeSinceLastUpdate);
				platformApi->read_file(saveFile, sizeof(scene->lSystems[i].position), &scene->lSystems[i].position);
				platformApi->read_file(saveFile, sizeof(scene->lSystems[i].rotation), &scene->lSystems[i].rotation);
				platformApi->read_file(saveFile, sizeof(scene->lSystems[i].type), &scene->lSystems[i].type);
				platformApi->read_file(saveFile, sizeof(scene->lSystems[i].totalAge), &scene->lSystems[i].totalAge);
				platformApi->read_file(saveFile, sizeof(scene->lSystems[i].maxAge), &scene->lSystems[i].maxAge);

				scene->lSystems[i].vertices = push_memory_view<Vertex>(persistentMemory, 15'000);
				scene->lSystems[i].indices = push_memory_view<u16>(persistentMemory, 45'000);

				scene->lSystemsPotIndices[i] = -1;
				scene->lSystemLeavess[i] 	= make_leaves(persistentMemory, 4000);

				if (scene->lSystems[i].type == TREE_1)
				{
					scene->lSystems[i].seedMesh 	= tree1SeedMesh;
					scene->lSystems[i].seedMaterial = tree1SeedMaterial;

					update_lsystem_mesh(scene->lSystems[i], scene->lSystemLeavess[i]);
				}
				else if (scene->lSystems[i].type == TREE_2)
				{
					scene->lSystems[i].seedMesh 	= tree2SeedMesh;
					scene->lSystems[i].seedMaterial = tree1SeedMaterial;
					
					update_lsystem_mesh_tree2(scene->lSystems[i], scene->lSystemLeavess[i]);
				}
			}	

		}
		else
		{
			scene->lSystemCapacity 		= 20;
			scene->lSystemCount 		= 20;
			scene->lSystems 			= push_memory<TimedLSystem>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);
			scene->lSystemLeavess 		= push_memory<Leaves>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);
			scene->lSystemsPotIndices 	= push_memory<s32>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);

			for (int i = 0; i < scene->lSystemCapacity; ++i)
			{
				scene->lSystems[i]			= make_lsystem(persistentMemory);
				scene->lSystemLeavess[i] 	= make_leaves(persistentMemory, 4000);
		
				if (i < scene->lSystemCapacity / 2)
				{
					scene->lSystems[i].type 		= TREE_1;
					scene->lSystems[i].seedMesh 	= tree1SeedMesh;
					scene->lSystems[i].seedMaterial = tree1SeedMaterial;
				}
				else
				{
					scene->lSystems[i].type 		= TREE_2;
					scene->lSystems[i].maxAge		= 50;
					scene->lSystems[i].aWord[0]		= {'A', 0, 0};

					scene->lSystems[i].seedMesh 	= tree2SeedMesh;
					scene->lSystems[i].seedMaterial = tree1SeedMaterial;
				}

				scene->lSystemsPotIndices[i] = -1;
			}

			for (s32 i = 0; i < scene->lSystemCount; ++i)
			{
				v3 position 				= random_inside_unit_square() * 20 - v3{10, 10, 0};
				position.z 					= get_terrain_height(scene->collisionSystem, position.xy);
				scene->lSystems[i].position = position;
				scene->lSystems[i].rotation = identity_quaternion;


				logDebug(0) << position;
			}
		}
	}

	// ----------------------------------------------------------------------------------

	logSystem(0) << "Scene3d loaded, " << used_percent(*global_transientMemory) * 100 << "% of transient memory used.";
	logSystem(0) << "Scene3d loaded, " << used_percent(persistentMemory) * 100 << "% of persistent memory used.";

	return scenePtr;
}
