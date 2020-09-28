/*
Leo Tamminen
shophorn @ internet

Scene description for 3d development game

Todo(Leo):
	crystal trees 
*/

// Todo(Leo): Maybe try to get rid of this forward declaration
struct Game;
internal void game_spawn_tree(Game & game, v3 position, s32 treeTypeIndex);


#include "CharacterMotor.cpp"
#include "PlayerController3rdPerson.cpp"
#include "FollowerController.cpp"

#include "scene3d_monuments.cpp"
#include "scene3d_trees.cpp"

#include "metaballs.cpp"
#include "experimental.cpp"
#include "dynamic_mesh.cpp"
#include "Trees3.cpp"
#include "settings.cpp" // Todo(Leo): this is sky, name and reorganize properly
#include "game_assets.cpp"
#include "game_settings.cpp"

enum CameraMode : s32
{ 
	CAMERA_MODE_PLAYER, 
	CAMERA_MODE_ELEVATOR,
	CAMERA_MODE_MOUSE_AND_KEYBOARD,

	CAMERA_MODE_COUNT
};

enum GameObjectType : s32
{ 	
	GO_NONE,
	GO_POT,
	GO_WATER,
	GO_RACCOON,
	GO_TREE_3,
};

enum TrainState : s32
{
	TRAIN_MOVE,
	TRAIN_WAIT,
};

enum NoblePersonMode : s32
{
	NOBLE_WANDER_AROUND,
	NOBLE_WAIT_FOR_TRAIN,
	NOBLE_AWAY,
	NOBLE_ARRIVING_IN_TRAIN,
};

enum RaccoonMode : s32
{
	RACCOON_IDLE,
	RACCOON_FLEE,
	RACCOON_CARRIED,
};

struct PhysicsObject
{
	GameObjectType 	type;
	s32 			index;
	f32 			velocity;
};

enum MenuView : s32
{
	MENU_OFF,
	MENU_MAIN,
	MENU_CONFIRM_EXIT,
	MENU_CONFIRM_TELEPORT,
	MENU_EDIT_SKY,
	MENU_EDIT_MESH_GENERATION,
	MENU_EDIT_TREE,
	MENU_SAVE_COMPLETE,

	MENU_SPAWN,
};

internal m44 * compute_transform_matrices(MemoryArena & allocator, s32 count, Transform3D * transforms)
{
	m44 * result = push_memory<m44>(allocator, count, ALLOC_GARBAGE);

	for (s32 i = 0; i < count; ++i)
	{
		result[i] = transform_matrix(transforms[i]);
	}

	return result;
}

struct Game
{
	MemoryArena * 	persistentMemory;
	GameAssets 		assets;

	SkySettings skySettings;

	// ---------------------------------------

	CollisionSystem3D 			collisionSystem;

	// ---------------------------------------

	Transform3D 		playerCharacterTransform;
	CharacterMotor 		playerCharacterMotor;
	SkeletonAnimator 	playerSkeletonAnimator;
	AnimatedRenderer 	playerAnimatedRenderer;

	PlayerInputState	playerInputState;

	Camera 						worldCamera;
	PlayerCameraController 		playerCamera;
	FreeCameraController		freeCamera;
	MouseCameraController		mouseCamera;

	// ---------------------------------------

	Transform3D 		noblePersonTransform;
	CharacterMotor 		noblePersonCharacterMotor;
	SkeletonAnimator 	noblePersonSkeletonAnimator;
	AnimatedRenderer 	noblePersonAnimatedRenderer;

	s32 	noblePersonMode;

	v3 		nobleWanderTargetPosition;
	f32 	nobleWanderWaitTimer;
	bool32 	nobleWanderIsWaiting;

	// ---------------------------------------

	static constexpr f32 fullWaterLevel = 1;
	Waters 			waters;
	MeshHandle 		waterMesh;
	MaterialHandle 	waterMaterial;

	/// POTS -----------------------------------
		s32 			potCapacity;
		s32 			potCount;
		Transform3D * 	potTransforms;
		f32 * 			potWaterLevels;

		MeshHandle 		potMesh;
		MaterialHandle 	potMaterial;

		MeshHandle 			bigPotMesh;
		MaterialHandle		bigPotMaterial;
		Array2<Transform3D> bigPotTransforms;

	// ------------------------------------------------------

	Monuments monuments;

	MeshHandle 		totemMesh;
	MaterialHandle 	totemMaterial;
	Transform3D 	totemTransforms [2];

	// ------------------------------------------------------

	s32 				raccoonCount;
	s32 *				raccoonModes;
	Transform3D * 		raccoonTransforms;
	v3 *				raccoonTargetPositions;
	CharacterMotor * 	raccoonCharacterMotors;

	MeshHandle 		raccoonMesh;
	MaterialHandle 	raccoonMaterial;

	Transform3D 	robotTransform;
	MeshHandle 		robotMesh;
	MaterialHandle 	robotMaterial;

	// ------------------------------------------------------

	s32 playerCarryState;
	s32 carriedItemIndex;

	Array2<PhysicsObject> physicsObjects;

	// ------------------------------------------------------

	Transform3D 	trainTransform;
	MeshHandle 		trainMesh;
	MaterialHandle 	trainMaterial;

	v3 trainStopPosition;
	v3 trainFarPositionA;
	v3 trainFarPositionB;

	s32 trainMoveState;
	s32 trainTargetReachedMoveState;

	s32 trainWayPointIndex;

	f32 trainFullSpeed;
	f32 trainStationMinSpeed;
	f32 trainAcceleration;
	f32 trainWaitTimeOnStop;
	f32 trainBrakeBeforeStationDistance;

	f32 trainCurrentWaitTime;
	f32 trainCurrentSpeed;

	v3 trainCurrentTargetPosition;
	v3 trainCurrentDirection;

	// ------------------------------------------------------

	// Note(Leo): There are multiple mesh handles here
	s32 			terrainCount;
	m44 * 			terrainTransforms;
	MeshHandle * 	terrainMeshes;
	MaterialHandle 	terrainMaterial;

	m44 			seaTransform;
	MeshHandle 		seaMesh;
	MaterialHandle 	seaMaterial;

	// ----------------------------------------------------------
	
	bool32 		drawMCStuff;
	
	f32 			metaballGridScale;
	MaterialHandle 	metaballMaterial;

	m44 		metaballTransform;

	u32 		metaballVertexCapacity;
	u32 		metaballVertexCount;
	Vertex * 	metaballVertices;

	u32 		metaballIndexCapacity;
	u32 		metaballIndexCount;
	u16 * 		metaballIndices;


	m44 metaballTransform2;
	
	u32 		metaballVertexCapacity2;
	u32 		metaballVertexCount2;
	Vertex * 	metaballVertices2;

	u32 		metaballIndexCapacity2;
	u32 		metaballIndexCount2;
	u16 *		metaballIndices2;


	// ----------------------------------------------------------

	// Sky
	ModelHandle 	skybox;

	// Random
	bool32 		getSkyColorFromTreeDistance;

	Gui 		gui;
	CameraMode 	cameraMode;
	bool32		drawDebugShadowTexture;


	MenuView menuView;

	GuiTextureHandle guiPanelImage;
	v4 guiPanelColour;

	// Todo(Leo): this is kinda too hacky
	constexpr static s32 	timeScaleCount = 3;
	s32 					timeScaleIndex;

	Array2<Tree3> 	trees;
	Tree3Settings 	treeSettings [2];

	s32 			inspectedTreeIndex;
};

internal void game_spawn_water(Game & game, s32 count)
{
	Waters & waters = game.waters;

	v2 center = game.playerCharacterTransform.position.xy;

	count = min_f32(count, waters.capacity - waters.count);

	for (s32 i = 0; i < count; ++i)
	{
		f32 distance 	= random_range(1, 5);
		f32 angle 		= random_range(0, 2 * π);

		f32 x = cosine(angle) * distance;
		f32 y = sine(angle) * distance;

		v3 position = { x + center.x, y + center.y, 0 };
		position.z = get_terrain_height(game.collisionSystem, position.xy);

		waters.transforms[waters.count].position = position;
		waters.transforms[waters.count].rotation = identity_quaternion;
		waters.transforms[waters.count].scale = {1,1,1};

		waters.levels[waters.count] = game.fullWaterLevel;

		waters.count += 1;
	}
}

internal void push_physics_object(Array2<PhysicsObject> & array, GameObjectType type, s32 index)
{
	Assert(array.count < array.capacity);
	array[array.count++] = {type, index};
}

internal void game_spawn_tree(Game & game, v3 position, s32 treeTypeIndex)
{
	// Assert(game.trees.count < game.trees.capacity);
	if (game.trees.count >= game.trees.capacity)
	{
		// Todo(Leo): do something more interesting
		log_debug(FILE_ADDRESS, "Trying to spawn tree, but out of tree capacity");
		return;
	}

	Tree3 & tree = game.trees[game.trees.count++];

	initialize_test_tree_3(*game.persistentMemory, tree, position, &game.treeSettings[treeTypeIndex]);

	tree.typeIndex 	= treeTypeIndex;
	tree.game 		= &game;

	MeshAssetId seedMesh = treeTypeIndex == 0 ? MESH_ASSET_SEED : MESH_ASSET_WATER_DROP;

	tree.leaves.material 	= assets_get_material(game.assets, MATERIAL_ASSET_LEAVES);
	tree.seedMesh 			= assets_get_mesh(game.assets, seedMesh);
	tree.seedMaterial		= assets_get_material(game.assets, MATERIAL_ASSET_SEED);

	push_physics_object(game.physicsObjects, GO_TREE_3, game.trees.count -1);
}

internal void game_spawn_tree_on_player(Game & game)
{
	v2 center = game.playerCharacterTransform.position.xy;

	f32 distance 	= random_range(1, 5);
	f32 angle 		= random_range(0, 2 * π);

	f32 x = cosine(angle) * distance;
	f32 y = sine(angle) * distance;

	v3 position = { x + center.x, y + center.y, 0 };
	position.z = get_terrain_height(game.collisionSystem, position.xy);

	local_persist s32 treeTypeIndex = 0;

	game_spawn_tree(game, position, treeTypeIndex);

	treeTypeIndex += 1;
	treeTypeIndex %= 2;
}


#include "game_gui.cpp"



// Todo(Leo): add this to syntax higlight, so that 'GAME UPDATE' is different color
/// ------------------ GAME UPDATE -------------------------
internal bool32 update_scene_3d(void * scenePtr, PlatformInput const & input, PlatformTime const & time)
{
	Game * game = reinterpret_cast<Game*>(scenePtr);
	
	/// ****************************************************************************

	struct SnapOnGround
	{
		CollisionSystem3D & collisionSystem;

		v3 operator()(v2 position)
		{
			v3 result = {position.x, position.y, get_terrain_height(collisionSystem, position)};
			return result;
		}

		v3 operator()(v3 position)
		{
			v3 result = {position.x, position.y, get_terrain_height(collisionSystem, position.xy)};
			return result;
		}
	};

	SnapOnGround snap_on_ground = {game->collisionSystem};

	/// ****************************************************************************
	/// TIME

	f32 const timeScales [game->timeScaleCount] { 1.0f, 0.1f, 0.5f }; 
	f32 scaledTime 		= time.elapsedTime * timeScales[game->timeScaleIndex];
	f32 unscaledTime 	= time.elapsedTime;

	/// ****************************************************************************

	gui_start_frame(game->gui, input, unscaledTime);

	/* Sadly, we need to draw skybox before game logic, because otherwise
	debug lines would be hidden. This can be fixed though, just make debug pipeline similar to shadows. */ 
	graphics_draw_model(platformGraphics, game->skybox, identity_m44, false, nullptr, 0);

	// Game Logic section
	switch (game->cameraMode)
	{
		case CAMERA_MODE_PLAYER:
		{
			CharacterInput playerCharacterMotorInput = {};

			if (game->menuView == MENU_OFF)
			{
				if (is_clicked(input.down))
				{
					game->menuView = MENU_SPAWN;
				}

				if (is_clicked(input.Y))
				{
					game->nobleWanderTargetPosition 	= game->playerCharacterTransform.position;
					game->nobleWanderWaitTimer 		= 0;
					game->nobleWanderIsWaiting 		= false;
				}

				playerCharacterMotorInput = update_player_input(game->playerInputState, game->worldCamera, input);
			}

			update_character_motor(game->playerCharacterMotor, playerCharacterMotorInput, game->collisionSystem, scaledTime, DEBUG_LEVEL_PLAYER);

			update_camera_controller(&game->playerCamera, game->playerCharacterTransform.position, input, scaledTime);

			game->worldCamera.position = game->playerCamera.position;
			game->worldCamera.direction = game->playerCamera.direction;

			game->worldCamera.farClipPlane = 1000;

			/// ---------------------------------------------------------------------------------------------------

			FS_DEBUG_NPC(debug_draw_circle_xy(game->nobleWanderTargetPosition + v3{0,0,0.5}, 1.0, colour_bright_green));
			FS_DEBUG_NPC(debug_draw_circle_xy(game->nobleWanderTargetPosition + v3{0,0,0.5}, 0.9, colour_bright_green));
		} break;

		case CAMERA_MODE_ELEVATOR:
		{
			m44 cameraMatrix = update_free_camera(game->freeCamera, input, unscaledTime);

			game->worldCamera.position = game->freeCamera.position;
			game->worldCamera.direction = game->freeCamera.direction;

			/// Document(Leo): Teleport player
			if (game->menuView == MENU_OFF && is_clicked(input.A))
			{
				game->menuView = MENU_CONFIRM_TELEPORT;
				gui_ignore_input();
				gui_reset_selection();
			}

			game->worldCamera.farClipPlane = 2000;
		} break;

		case CAMERA_MODE_MOUSE_AND_KEYBOARD:
		{
			update_mouse_camera(game->mouseCamera, input, unscaledTime);

			game->worldCamera.position = game->mouseCamera.position;
			game->worldCamera.direction = game->mouseCamera.direction;

			/// Document(Leo): Teleport player
			if (game->menuView == MENU_OFF && is_clicked(input.A))
			{
				game->menuView = MENU_CONFIRM_TELEPORT;
				gui_ignore_input();
				gui_reset_selection();
			}

			game->worldCamera.farClipPlane = 2000;
		} break;

		case CAMERA_MODE_COUNT:
			Assert(false && "Bad execution path");
			break;
	}


	/// *******************************************************************************************

	update_camera_system(&game->worldCamera, platformGraphics, platformWindow);

	{
		v3 lightDirection = {0,0,1};
		lightDirection = rotate_v3(axis_angle_quaternion(right_v3, game->skySettings.sunHeightAngle * π), lightDirection);
		lightDirection = rotate_v3(axis_angle_quaternion(up_v3, game->skySettings.sunOrbitAngle * 2 * π), lightDirection);
		lightDirection = normalize_v3(-lightDirection);

		Light light = { .direction 	= lightDirection, //normalize_v3({-1, 1.2, -8}), 
						.color 		= v3{0.95, 0.95, 0.9}};
		v3 ambient 	= {0.05, 0.05, 0.3};

		light.skyColorSelection = game->skySettings.skyColourSelection;

		light.skyGroundColor.rgb 	= game->skySettings.skyGradientGround;
		light.skyBottomColor.rgb 	= game->skySettings.skyGradientBottom;
		light.skyTopColor.rgb 		= game->skySettings.skyGradientTop;

		light.horizonHaloColorAndFalloff.rgb 	= game->skySettings.horizonHaloColour;
		light.horizonHaloColorAndFalloff.a 		= game->skySettings.horizonHaloFalloff;

		light.sunHaloColorAndFalloff.rgb 	= game->skySettings.sunHaloColour;
		light.sunHaloColorAndFalloff.a 		= game->skySettings.sunHaloFalloff;

		light.sunDiscColor.rgb 			= game->skySettings.sunDiscColour;
		light.sunDiscSizeAndFalloff.xy 	= {	game->skySettings.sunDiscSize,
											game->skySettings.sunDiscFalloff };

		graphics_drawing_update_lighting(platformGraphics, &light, &game->worldCamera, ambient);
		HdrSettings hdrSettings = 
		{
			game->skySettings.hdrExposure,
			game->skySettings.hdrContrast,
		};
		graphics_drawing_update_hdr_settings(platformGraphics, &hdrSettings);
	}

	/// *******************************************************************************************
	

	/// SPAWN WATER
	bool playerInputAvailable = game->cameraMode == CAMERA_MODE_PLAYER
								&& game->menuView == MENU_OFF;

	/// PICKUP OR DROP
	if (playerInputAvailable && is_clicked(input.A))
	{
		v3 playerPosition = game->playerCharacterTransform.position;
		f32 grabDistance = 1.0f;

		switch(game->playerCarryState)
		{
			case GO_NONE: {
				/* Todo(Leo): Do this properly, taking into account player facing direction and distances etc. */
				auto check_pickup = [	playerPosition 		= game->playerCharacterTransform.position,
										&playerCarryState 	= game->playerCarryState,
										&carriedItemIndex 	= game->carriedItemIndex,
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

				check_pickup(game->potCount, game->potTransforms, GO_POT);
				check_pickup(game->waters.count, game->waters.transforms, GO_WATER);
				check_pickup(game->raccoonCount, game->raccoonTransforms, GO_RACCOON);

				{
					for (auto & tree : game->trees)
					{
						if (magnitude_v3(playerPosition - tree.position) < grabDistance)
						{
							// f32 maxGrabbableLength 	= 1;
							// f32 maxGrabbableRadius 	= maxGrabbableLength / tree.settings->maxHeightToWidthRatio;
							// bool isGrabbable 		= tree.nodes[tree.branches[0].startNodeIndex].radius > maxGrabbableRadius;

							// if (isGrabbable)
							{
								game->playerCarryState = GO_TREE_3;
								game->carriedItemIndex = array_2_get_index_of(game->trees, tree);
							}
						}
					}
				}

			} break;

			case GO_POT:
				push_physics_object(game->physicsObjects, GO_POT, game->carriedItemIndex);
				game->carriedItemIndex = -1;
				game->playerCarryState = GO_NONE;

				break;
	
			case GO_WATER:
			{
				Transform3D & waterTransform = game->waters.transforms[game->carriedItemIndex];

				push_physics_object(game->physicsObjects, GO_WATER, game->carriedItemIndex);
				game->carriedItemIndex = -1;
				game->playerCarryState = GO_NONE;

				constexpr f32 waterSnapDistance 	= 0.5f;
				constexpr f32 smallPotMaxWaterLevel = 1.0f;

				f32 droppedWaterLevel = game->waters.levels[game->carriedItemIndex];

				for (s32 i = 0; i < game->potCount; ++i)
				{
					f32 distance = magnitude_v3(game->potTransforms[i].position - waterTransform.position);
					if (distance < waterSnapDistance)
					{
						game->potWaterLevels[i] += droppedWaterLevel;
						game->potWaterLevels[i] = min_f32(game->potWaterLevels[i] + droppedWaterLevel, smallPotMaxWaterLevel);

						game->waters.count -= 1;
						game->waters.transforms[game->carriedItemIndex] 	= game->waters.transforms[game->waters.count];
						game->waters.levels[game->carriedItemIndex] 		= game->waters.levels[game->waters.count];

						break;	
					}
				}


			} break;

			case GO_RACCOON:
			{
				game->raccoonTransforms[game->carriedItemIndex].rotation = identity_quaternion;
				push_physics_object(game->physicsObjects, GO_RACCOON, game->carriedItemIndex);
				game->carriedItemIndex = -1;
				game->playerCarryState = GO_NONE;

			} break;

			case GO_TREE_3:
			{	
				push_physics_object(game->physicsObjects, GO_TREE_3, game->carriedItemIndex);
				game->carriedItemIndex = -1;
				game->playerCarryState = GO_NONE;
			} break;
		}
	} // endif input


	v3 carriedPosition 			= multiply_point(transform_matrix(game->playerCharacterTransform), {0, 0.7, 0.7});
	quaternion carriedRotation 	= game->playerCharacterTransform.rotation;
	switch(game->playerCarryState)
	{
		case GO_POT:
			game->potTransforms[game->carriedItemIndex].position = carriedPosition;
			game->potTransforms[game->carriedItemIndex].rotation = carriedRotation;
			break;

		case GO_WATER:
			game->waters.transforms[game->carriedItemIndex].position = carriedPosition;
			game->waters.transforms[game->carriedItemIndex].rotation = carriedRotation;
			break;

		case GO_RACCOON:
		{
			game->raccoonTransforms[game->carriedItemIndex].position 	= carriedPosition + v3{0,0,0.2};

			v3 right = rotate_v3(carriedRotation, right_v3);
			game->raccoonTransforms[game->carriedItemIndex].rotation 	= carriedRotation * axis_angle_quaternion(right, 1.4f);


		} break;

		case GO_TREE_3:
			game->trees[game->carriedItemIndex].position = carriedPosition;
			break;

		case GO_NONE:
			break;


		default:
			Assert(false && "That cannot be carried!");
			break;
	}

	/// UPDATE TREES IN POTS POSITIONS
	// Todo(Leo):...

	// UPDATE physics objects
	{
		for (s32 i = 0; i < game->physicsObjects.count; ++i)
		{
			s32 index 		= game->physicsObjects[i].index;
			bool cancelFall = index == game->carriedItemIndex;

			f32 & velocity = game->physicsObjects[i].velocity;
			v3 * position;

			switch (game->physicsObjects[i].type)
			{
				case GO_RACCOON:	position = &game->raccoonTransforms[index].position; 	break;
				case GO_WATER: 		position = &game->waters.transforms[index].position; 	break;
				case GO_POT:		position = &game->potTransforms[index].position;		break;
				case GO_TREE_3: 	position = &game->trees[index].position; 				break;

				default:
					Assert(false && "That item has not physics properties");
			}
		
			f32 groundHeight = get_terrain_height(game->collisionSystem, position->xy);

			if (cancelFall)
			{
				array_2_unordered_remove(game->physicsObjects, i);
				i -= 1;

				continue;
			}

			// Todo(Leo): this is in reality dependent on frame rate, this works for development capped 30 ms frame time
			// but is lower when frametime is lower
			constexpr f32 physicsSleepThreshold = 0.2;

			if (velocity < 0 && position->z < groundHeight)
			{
				position->z = groundHeight;

				f32 velocityBefore = velocity;

				f32 bounceFactor 	= 0.5;
				velocity 			= -velocity * bounceFactor;

				if (velocity < physicsSleepThreshold)
				{
					array_2_unordered_remove(game->physicsObjects, i);
					i -= 1;
				}

			}
			else
			{
				velocity 		+= scaledTime * physics_gravity_acceleration;
				position->z 	+= scaledTime * velocity;
			}
		}
	}
	
	// -----------------------------------------------------------------------------------------------------------
	/// TRAIN
	{
		v3 trainWayPoints [] = 
		{
			game->trainStopPosition, 
			game->trainFarPositionA,
			game->trainStopPosition, 
			game->trainFarPositionB,
		};

		if (game->trainMoveState == TRAIN_MOVE)
		{

			v3 trainMoveVector 	= game->trainCurrentTargetPosition - game->trainTransform.position;
			f32 directionDot 	= dot_v3(trainMoveVector, game->trainCurrentDirection);
			f32 moveDistance	= magnitude_v3(trainMoveVector);

			if (moveDistance > game->trainBrakeBeforeStationDistance)
			{
				game->trainCurrentSpeed += scaledTime * game->trainAcceleration;
				game->trainCurrentSpeed = min_f32(game->trainCurrentSpeed, game->trainFullSpeed);
			}
			else
			{
				game->trainCurrentSpeed -= scaledTime * game->trainAcceleration;
				game->trainCurrentSpeed = max_f32(game->trainCurrentSpeed, game->trainStationMinSpeed);
			}

			if (directionDot > 0)
			{
				game->trainTransform.position += game->trainCurrentDirection * game->trainCurrentSpeed * scaledTime;
			}
			else
			{
				game->trainMoveState 		= TRAIN_WAIT;
				game->trainCurrentWaitTime = 0;
			}

		}
		else
		{
			game->trainCurrentWaitTime += scaledTime;
			if (game->trainCurrentWaitTime > game->trainWaitTimeOnStop)
			{
				game->trainMoveState 		= TRAIN_MOVE;
				game->trainCurrentSpeed 	= 0;

				v3 start 	= trainWayPoints[game->trainWayPointIndex];
				v3 end 		= trainWayPoints[(game->trainWayPointIndex + 1) % array_count(trainWayPoints)];

				game->trainWayPointIndex += 1;
				game->trainWayPointIndex %= array_count(trainWayPoints);

				game->trainCurrentTargetPosition 	= end;
				game->trainCurrentDirection 		= normalize_v3(end - start);
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------
	/// NOBLE PERSON CHARACTER
	{
		CharacterInput nobleCharacterMotorInput = {};

		switch(game->noblePersonMode)
		{
			case NOBLE_WANDER_AROUND:
			{
				if (game->nobleWanderIsWaiting)
				{
					game->nobleWanderWaitTimer -= scaledTime;
					if (game->nobleWanderWaitTimer < 0)
					{
						game->nobleWanderTargetPosition = { random_range(-99, 99), random_range(-99, 99)};
						game->nobleWanderIsWaiting = false;
					}


					m44 gizmoTransform = make_transform_matrix(	game->noblePersonTransform.position + up_v3 * game->noblePersonTransform.scale.z * 2.0f, 
																game->noblePersonTransform.rotation,
																game->nobleWanderWaitTimer);
					FS_DEBUG_NPC(debug_draw_diamond_xz(gizmoTransform, colour_muted_red));
				}
				
				f32 distance = magnitude_v2(game->noblePersonTransform.position.xy - game->nobleWanderTargetPosition.xy);
				if (distance < 1.0f && game->nobleWanderIsWaiting == false)
				{
					game->nobleWanderWaitTimer = 10;
					game->nobleWanderIsWaiting = true;
				}


				v3 input 			= {};
				input.xy	 		= game->nobleWanderTargetPosition.xy - game->noblePersonTransform.position.xy;
				f32 inputMagnitude 	= magnitude_v3(input);
				input 				= input / inputMagnitude;
				inputMagnitude 		= clamp_f32(inputMagnitude, 0.0f, 1.0f);
				input 				= input * inputMagnitude;

				nobleCharacterMotorInput = {input, false, false};

			} break;
		}
	
		update_character_motor(	game->noblePersonCharacterMotor,
								nobleCharacterMotorInput,
								game->collisionSystem,
								scaledTime,
								DEBUG_LEVEL_NPC);
	}

	// -----------------------------------------------------------------------------------------------------------
	/// Update RACCOONS
	{

		for(s32 i = 0; i < game->raccoonCount; ++i)
		{
			CharacterInput raccoonCharacterMotorInput = {};
	
			v3 toTarget 			= game->raccoonTargetPositions[i] - game->raccoonTransforms[i].position;
			f32 distanceToTarget 	= magnitude_v3(toTarget);

			v3 input = {};

			if (distanceToTarget < 1.0f)
			{
				game->raccoonTargetPositions[i] 	= snap_on_ground(random_inside_unit_square() * 100 - v3{50,50,0});
				toTarget 							= game->raccoonTargetPositions[i] - game->raccoonTransforms[i].position;
			}
			else
			{
				input.xy	 		= game->raccoonTargetPositions[i].xy - game->raccoonTransforms[i].position.xy;
				f32 inputMagnitude 	= magnitude_v3(input);
				input 				= input / inputMagnitude;
				inputMagnitude 		= clamp_f32(inputMagnitude, 0.0f, 1.0f);
				input 				= input * inputMagnitude;
			}

			raccoonCharacterMotorInput = {input, false, false};

			bool isCarried = (game->playerCarryState == GO_RACCOON) && (game->carriedItemIndex == i);

			if (isCarried == false)
			{
				update_character_motor( game->raccoonCharacterMotors[i],
										raccoonCharacterMotorInput,
										game->collisionSystem,
										scaledTime,
										DEBUG_LEVEL_NPC);
			}

			// debug_draw_circle_xy(snap_on_ground(game->raccoonTargetPositions[i].xy) + v3{0,0,1}, 1, colour_bright_red, DEBUG_LEVEL_ALWAYS);
		}


	}

	// -----------------------------------------------------------------------------------------------------------

	// Todo(Leo): Rather use something like submit_collider() with what every collider can decide themselves, if they want to
	// contribute to collisions this frame or not.
	precompute_colliders(game->collisionSystem);

	/// SUBMIT COLLIDERS
	{
		// Todo(Leo): Maybe make something like that these would have predetermined range, that is only updated when
		// tree has grown a certain amount or somthing. These are kinda semi-permanent by nature.

		auto submit_cylinder_colliders = [&collisionSystem = game->collisionSystem](f32 radius, f32 halfHeight, s32 count, Transform3D * transforms)
		{
			for (s32 i = 0; i < count; ++i)
			{
				submit_cylinder_collider(collisionSystem, radius, halfHeight, transforms[i].position + v3{0,0,halfHeight});
			}
		};

		f32 smallPotColliderRadius = 0.3;
		f32 smallPotColliderHeight = 0.58;
		f32 smallPotHalfHeight = smallPotColliderHeight / 2;
		submit_cylinder_colliders(smallPotColliderRadius, smallPotHalfHeight, game->potCount, game->potTransforms);

		constexpr f32 baseRadius = 0.12;
		constexpr f32 baseHeight = 2;

		// NEW TREES 3
		for (auto tree : game->trees)
		{
			constexpr f32 colliderHalfHeight = 1.0f;
			constexpr v3 colliderOffset = {0, 0, colliderHalfHeight};
			submit_cylinder_collider(	game->collisionSystem,
										tree.nodes[tree.branches[0].startNodeIndex].radius,
										colliderHalfHeight,
										tree.position + colliderOffset);
		}
	}


	update_skeleton_animator(game->playerSkeletonAnimator, scaledTime);
	update_skeleton_animator(game->noblePersonSkeletonAnimator, scaledTime);
	
	
	/// DRAW POTS
	{
		// Todo(Leo): store these as matrices, we can easily retrieve position (that is needed somwhere) from that too.
		m44 * potTransformMatrices = push_memory<m44>(*global_transientMemory, game->potCount, ALLOC_GARBAGE);
		for(s32 i = 0; i < game->potCount; ++i)
		{
			potTransformMatrices[i] = transform_matrix(game->potTransforms[i]);
		}
		graphics_draw_meshes(platformGraphics, game->potCount, potTransformMatrices, game->potMesh, game->potMaterial);
	}

	/// DRAW STATIC SCENERY
	{
		for(s32 i = 0; i < game->terrainCount; ++i)
		{
			graphics_draw_meshes(platformGraphics, 1, game->terrainTransforms + i, game->terrainMeshes[i], game->terrainMaterial);
		}

		graphics_draw_meshes(platformGraphics, 1, &game->seaTransform, game->seaMesh, game->seaMaterial);

		m44 * totemTransforms = compute_transform_matrices(*global_transientMemory, array_count(game->totemTransforms), game->totemTransforms);
		graphics_draw_meshes(platformGraphics, array_count(game->totemTransforms), totemTransforms, game->totemMesh, game->totemMaterial);		

		m44 * bigPotTransforms = compute_transform_matrices(*global_transientMemory, game->bigPotTransforms.count, game->bigPotTransforms.memory);
		graphics_draw_meshes(platformGraphics, game->bigPotTransforms.count, bigPotTransforms, game->bigPotMesh, game->bigPotMaterial);		

		m44 * robotTransforms = compute_transform_matrices(*global_transientMemory, 1, &game->robotTransform);
		graphics_draw_meshes(platformGraphics, 1, robotTransforms, game->robotMesh, game->robotMaterial);

		game_draw_monuments(game->monuments);
	}


	/// DEBUG DRAW COLLIDERS
	{
		for (auto const & collider : game->collisionSystem.precomputedBoxColliders)
		{
			FS_DEBUG_BACKGROUND(debug_draw_box(collider.transform, colour_muted_green));
		}

		for (auto const & collider : game->collisionSystem.staticBoxColliders)
		{
			FS_DEBUG_BACKGROUND(debug_draw_box(collider.transform, colour_dark_green));
		}

		for (auto const & collider : game->collisionSystem.precomputedCylinderColliders)
		{
			FS_DEBUG_BACKGROUND(debug_draw_circle_xy(collider.center - v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green));
			FS_DEBUG_BACKGROUND(debug_draw_circle_xy(collider.center + v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green));
		}

		for (auto const & collider : game->collisionSystem.submittedCylinderColliders)
		{
			FS_DEBUG_BACKGROUND(debug_draw_circle_xy(collider.center - v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green));
			FS_DEBUG_BACKGROUND(debug_draw_circle_xy(collider.center + v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green));
		}

		FS_DEBUG_PLAYER(debug_draw_circle_xy(game->playerCharacterTransform.position + v3{0,0,0.7}, 0.25f, colour_bright_green));
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

	if (game->drawMCStuff)
	{
		v3 position = multiply_point(game->metaballTransform, {0,0,0});
		// debug_draw_circle_xy(position, 4, {1,0,1,1}, DEBUG_LEVEL_ALWAYS);
		// graphics_draw_meshes(platformGraphics, 1, &game->metaballTransform, game->metaballMesh, game->metaballMaterial);

		local_persist f32 testX = 0;
		local_persist f32 testY = 0;
		local_persist f32 testZ = 0;
		local_persist f32 testW = 0;

		testX += scaledTime;
		testY += scaledTime * 1.2;
		testZ += scaledTime * 0.9;
		testW += scaledTime * 1.7;

		v4 positions[] =
		{
			{mathfun_pingpong_f32(testX, 5),2,2,										2},
			{1,mathfun_pingpong_f32(testY, 3),1,										1},
			{4,3,mathfun_pingpong_f32(testZ, 3) + 1,									1.5},
			{mathfun_pingpong_f32(testW, 4) + 1, mathfun_pingpong_f32(testY, 3), 3,		1.2},
		};

		generate_mesh_marching_cubes(	game->metaballVertexCapacity, game->metaballVertices, &game->metaballVertexCount,
										game->metaballIndexCapacity, game->metaballIndices, &game->metaballIndexCount,
										sample_four_sdf_balls, positions, game->metaballGridScale);

		graphics_draw_procedural_mesh(	platformGraphics,
											game->metaballVertexCount, game->metaballVertices,
											game->metaballIndexCount, game->metaballIndices,
											game->metaballTransform,
											game->metaballMaterial);

		auto sample_sdf_2 = [](v3 position, void const * data)
		{
			v3 a = {2,2,2};
			v3 b = {5,3,5};

			f32 rA = 1;
			f32 rB = 1;

			// f32 d = min_f32(1, max_f32(0, dot_v3()))

			f32 t = min_f32(1, max_f32(0, dot_v3(position - a, b - a) / square_magnitude_v3(b-a)));
			f32 d = magnitude_v3(position - a  - t * (b -a));

			return d - lerp_f32(0.5,0.1,t);

			// return min_f32(magnitude_v3(a - position) - rA, magnitude_v3(b - position) - rB);
		};

		f32 fieldMemory [] =
		{
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			
			5,5,5,5,5,
			5,-2,-2,5,5,
			5,-1,-1,-1,5,
			5,5,5,5,5,
			
			5,5,5,5,5,
			5,5,5,5,5,
			5,5,5,5,5,
			5,5,5,5,5,
		};

		// v3 fieldSize = {5,4,4};

		VoxelField field = {5, 4, 4, fieldMemory};

		generate_mesh_marching_cubes(	game->metaballVertexCapacity2, game->metaballVertices2, &game->metaballVertexCount2,
										game->metaballIndexCapacity2, game->metaballIndices2, &game->metaballIndexCount2,
										sample_heightmap_for_mc, &game->collisionSystem.terrainCollider, game->metaballGridScale);

		FS_DEBUG_ALWAYS(debug_draw_circle_xy(multiply_point(game->metaballTransform2, game->metaballVertices2[0].position), 5.0f, colour_bright_green));
		// log_debug(0) << multiply_point(game->metaballTransform2, game->metaballVertices2[0].position);

		if (game->metaballVertexCount2 > 0 && game->metaballIndexCount2 > 0)
		{
			graphics_draw_procedural_mesh(	platformGraphics,
												game->metaballVertexCount2, game->metaballVertices2,
												game->metaballIndexCount2, game->metaballIndices2,
												game->metaballTransform2,
												game->metaballMaterial);
		}
	}

	{
		m44 trainTransformMatrix = transform_matrix(game->trainTransform);
		graphics_draw_meshes(platformGraphics, 1, &trainTransformMatrix, game->trainMesh, game->trainMaterial);
	}

	/// DRAW RACCOONS
	{
		m44 * raccoonTransformMatrices = push_memory<m44>(*global_transientMemory, game->raccoonCount, ALLOC_GARBAGE);
		for (s32 i = 0; i < game->raccoonCount; ++i)
		{
			raccoonTransformMatrices[i] = transform_matrix(game->raccoonTransforms[i]);
		}
		graphics_draw_meshes(platformGraphics, game->raccoonCount, raccoonTransformMatrices, game->raccoonMesh, game->raccoonMaterial);
	}


	/// PLAYER
	/// CHARACTER 2
	{

		m44 boneTransformMatrices [32];
		
		// -------------------------------------------------------------------------------

		update_animated_renderer(boneTransformMatrices, game->playerSkeletonAnimator);

		graphics_draw_model(platformGraphics, 	game->playerAnimatedRenderer.model,
												transform_matrix(game->playerCharacterTransform),
												true,
												boneTransformMatrices, array_count(boneTransformMatrices));

		// -------------------------------------------------------------------------------

		update_animated_renderer(boneTransformMatrices, game->noblePersonSkeletonAnimator);

		graphics_draw_model(platformGraphics, 	game->playerAnimatedRenderer.model,
												transform_matrix(game->noblePersonTransform),
												true,
												boneTransformMatrices, array_count(boneTransformMatrices));
	}


	/// DRAW UNUSED WATER
	if (game->waters.count > 0)
	{
		m44 * waterTransforms = push_memory<m44>(*global_transientMemory, game->waters.count, ALLOC_GARBAGE);
		for (s32 i = 0; i < game->waters.count; ++i)
		{
			waterTransforms[i] = transform_matrix(	game->waters.transforms[i].position,
													game->waters.transforms[i].rotation,
													make_uniform_v3(game->waters.levels[i] / game->fullWaterLevel));
		}
		graphics_draw_meshes(platformGraphics, game->waters.count, waterTransforms, game->waterMesh, game->waterMaterial);
	}

	for (auto & tree : game->trees)
	{
		GetWaterFunc get_water = {game->waters, game->carriedItemIndex, game->playerCarryState == GO_WATER };
		update_tree_3(tree, scaledTime, get_water);
		
		tree.leaves.position = tree.position;
		tree.leaves.rotation = identity_quaternion;


		m44 transform = translation_matrix(tree.position);
		// Todo(Leo): make some kind of SLOPPY_ASSERT macro thing
		if (tree.mesh.vertices.count > 0 || tree.mesh.indices.count > 0)
		{
			graphics_draw_procedural_mesh(	platformGraphics,
											tree.mesh.vertices.count, tree.mesh.vertices.memory,
											tree.mesh.indices.count, tree.mesh.indices.memory,
											transform, assets_get_material(game->assets, MATERIAL_ASSET_TREE));
		}

		if (tree.drawSeed)
		{
			graphics_draw_meshes(platformGraphics, 1, &transform, tree.seedMesh, tree.seedMaterial);
		}

		if (tree.hasFruit)
		{
			v3 fruitPosition = tree.position + tree.fruitPosition;

			f32 fruitSize = tree.fruitAge / tree.fruitMaturationTime;
			v3 fruitScale = {fruitSize, fruitSize, fruitSize};

			m44 fruitTransform = transform_matrix(fruitPosition, identity_quaternion, fruitScale);

			FS_DEBUG_ALWAYS(debug_draw_circle_xy(fruitPosition, 0.5, colour_bright_red));

			graphics_draw_meshes(platformGraphics, 1, &fruitTransform, tree.seedMesh, tree.seedMaterial);
		}

		FS_DEBUG_BACKGROUND(debug_draw_circle_xy(tree.position, 2, colour_bright_red));
	}

	/// DRAW LEAVES FOR BOTH LSYSTEM ARRAYS IN THE END BECAUSE OF LEAF SHDADOW INCONVENIENCE
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): This works ok currently, but do fix this, maybe do a proper command buffer for these
	{
		for (auto & tree : game->trees)
		{
			draw_leaves(tree.leaves, scaledTime, tree.settings->leafSize, tree.settings->leafColour);
		}
	}

	// ------------------------------------------------------------------------

	auto gui_result = do_gui(game, input);

	// if (game->settings.dirty)
	// {
	// 	write_settings_file(game->settings, game);
	// 	game->settings.dirty = false;
	// }

	return gui_result;
}

internal void * load_scene_3d(MemoryArena & persistentMemory, PlatformFileHandle saveFile)
{
	Game * game = push_memory<Game>(persistentMemory, 1, ALLOC_ZERO_MEMORY);
	*game = {};
	game->persistentMemory = &persistentMemory;

	// Note(Leo): We currently only have statically allocated stuff (or rather allocated with game),
	// this can be read here, at the top. If we need to allocate some stuff, we need to reconsider.
	read_settings_file(game->skySettings, game->treeSettings[0], game->treeSettings[1]);

	// ----------------------------------------------------------------------------------

	scene_3d_initialize_gui(game);

	// ----------------------------------------------------------------------------------

	// Todo(Leo): Think more about implications of storing pointer to persistent memory here
	// Note(Leo): We have not previously used persistent allocation elsewhere than in this load function
	game->assets = init_game_assets(&persistentMemory);

	game->collisionSystem.boxColliders 			= push_array_2<BoxCollider>(persistentMemory, 600, ALLOC_GARBAGE);
	game->collisionSystem.cylinderColliders 	= push_array_2<CylinderCollider>(persistentMemory, 600, ALLOC_GARBAGE);
	game->collisionSystem.staticBoxColliders 	= push_array_2<StaticBoxCollider>(persistentMemory, 2000, ALLOC_GARBAGE);

	game->physicsObjects = push_array_2<PhysicsObject>(persistentMemory, 100, ALLOC_ZERO_MEMORY);

	log_application(1, "Allocations succesful! :)");

	// ----------------------------------------------------------------------------------

	{
		// Todo(Leo): gui assets to systemtic asset things also
		TextureAssetData testGuiAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/tiles.png");
		game->guiPanelImage 		= graphics_memory_push_gui_texture(platformGraphics, &testGuiAsset);
		game->guiPanelColour 		= colour_rgb_alpha(colour_aqua_blue.rgb, 0.5);
	}


	// Skysphere
	game->skybox = graphics_memory_push_model(	platformGraphics,
												assets_get_mesh(game->assets, MESH_ASSET_SKYSPHERE),
												assets_get_material(game->assets, MATERIAL_ASSET_SKY));

	// Characters
	{
		game->playerCarryState 		= GO_NONE;
		game->playerCharacterTransform = {.position = {10, 0, 5}};

		auto & motor 	= game->playerCharacterMotor;
		motor.transform = &game->playerCharacterTransform;

		{
			using namespace CharacterAnimations;			

			motor.animations[WALK] 		= assets_get_animation(game->assets, AAID_CHARACTER_WALK);
			motor.animations[RUN] 		= assets_get_animation(game->assets, AAID_CHARACTER_RUN);
			motor.animations[IDLE] 		= assets_get_animation(game->assets, AAID_CHARACTER_IDLE);
			motor.animations[JUMP]		= assets_get_animation(game->assets, AAID_CHARACTER_JUMP);
			motor.animations[FALL]		= assets_get_animation(game->assets, AAID_CHARACTER_FALL);
			motor.animations[CROUCH] 	= assets_get_animation(game->assets, AAID_CHARACTER_CROUCH);
		}

		game->playerSkeletonAnimator = 
		{
			.skeleton 		= assets_get_skeleton(game->assets, SAID_CHARACTER),
			.animations 	= motor.animations,
			.weights 		= motor.animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		};

		game->playerSkeletonAnimator.boneBoneSpaceTransforms = push_array_2<Transform3D>(	persistentMemory,
																							game->playerSkeletonAnimator.skeleton->bones.count,
																							ALLOC_ZERO_MEMORY);
		array_2_fill_with_value(game->playerSkeletonAnimator.boneBoneSpaceTransforms, identity_transform);

		auto model = graphics_memory_push_model(platformGraphics,
												assets_get_mesh(game->assets, MESH_ASSET_CHARACTER),
												assets_get_material(game->assets, MATERIAL_ASSET_CHARACTER));
		game->playerAnimatedRenderer = make_animated_renderer(&game->playerCharacterTransform, game->playerSkeletonAnimator.skeleton, model);

	}

	// NOBLE PERSON GUY
	{
		v3 position = {random_range(-99, 99), random_range(-99, 99), 0};
		v3 scale 	= make_uniform_v3(random_range(0.8f, 1.5f));

		game->noblePersonTransform.position 	= position;
		game->noblePersonTransform.scale 		= scale;

		game->noblePersonCharacterMotor = {};
		game->noblePersonCharacterMotor.transform = &game->noblePersonTransform;

		{
			using namespace CharacterAnimations;
			
			game->noblePersonCharacterMotor.animations[WALK] 	= assets_get_animation(game->assets, AAID_CHARACTER_WALK);
			game->noblePersonCharacterMotor.animations[RUN] 	= assets_get_animation(game->assets, AAID_CHARACTER_RUN);
			game->noblePersonCharacterMotor.animations[IDLE]	= assets_get_animation(game->assets, AAID_CHARACTER_IDLE);
			game->noblePersonCharacterMotor.animations[JUMP] 	= assets_get_animation(game->assets, AAID_CHARACTER_JUMP);
			game->noblePersonCharacterMotor.animations[FALL] 	= assets_get_animation(game->assets, AAID_CHARACTER_FALL);
			game->noblePersonCharacterMotor.animations[CROUCH] = assets_get_animation(game->assets, AAID_CHARACTER_CROUCH);
		}

		game->noblePersonSkeletonAnimator = 
		{
			.skeleton 		= assets_get_skeleton(game->assets, SAID_CHARACTER),
			.animations 	= game->noblePersonCharacterMotor.animations,
			.weights 		= game->noblePersonCharacterMotor.animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		};
		game->noblePersonSkeletonAnimator.boneBoneSpaceTransforms = push_array_2<Transform3D>(	persistentMemory,
																								game->noblePersonSkeletonAnimator.skeleton->bones.count,
																								ALLOC_ZERO_MEMORY);
		array_2_fill_with_value(game->noblePersonSkeletonAnimator.boneBoneSpaceTransforms, identity_transform);

		auto model = graphics_memory_push_model(platformGraphics,
												assets_get_mesh(game->assets, MESH_ASSET_CHARACTER), 
												assets_get_material(game->assets, MATERIAL_ASSET_CHARACTER)); 

		game->noblePersonAnimatedRenderer = make_animated_renderer(&game->noblePersonTransform, game->noblePersonSkeletonAnimator.skeleton, model);

	}

	game->worldCamera 				= make_camera(60, 0.1f, 1000.0f);
	game->playerCamera 			= {};
	game->playerCamera.baseOffset 	= {0, 0, 2};
	game->playerCamera.distance 	= 5;

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

			HeightMap heightmap;
			{
				auto checkpoint = memory_push_checkpoint(*global_transientMemory);
				
				s32 heightmapResolution = 1024;

				// todo(Leo): put to asset system thing
				auto heightmapTexture 	= load_texture_asset(*global_transientMemory, "assets/textures/heightmap_island.png");
				heightmap 				= make_heightmap(&persistentMemory, &heightmapTexture, heightmapResolution, mapSize, minTerrainElevation, maxTerrainElevation);

				memory_pop_checkpoint(*global_transientMemory, checkpoint);
			}

			s32 terrainCount 			= chunkCountPerDirection * chunkCountPerDirection;
			game->terrainCount 		= terrainCount;
			game->terrainTransforms 	= push_memory<m44>(persistentMemory, terrainCount, ALLOC_GARBAGE);
			game->terrainMeshes 		= push_memory<MeshHandle>(persistentMemory, terrainCount, ALLOC_GARBAGE);
			game->terrainMaterial 		= assets_get_material(game->assets, MATERIAL_ASSET_GROUND);

			/// GENERATE GROUND MESH
			{
				auto checkpoint = memory_push_checkpoint(*global_transientMemory);
				// push_memory_checkpoint(*global_transientMemory);
			
				for (s32 i = 0; i < terrainCount; ++i)
				{
					s32 x = i % chunkCountPerDirection;
					s32 y = i / chunkCountPerDirection;

					v2 position = { x * chunkSize, y * chunkSize };
					v2 size 	= { chunkSize, chunkSize };

					auto groundMeshAsset 	= generate_terrain(*global_transientMemory, heightmap, position, size, chunkResolution, 20);
					game->terrainMeshes[i] = graphics_memory_push_mesh(platformGraphics, &groundMeshAsset);
				}
			
				memory_pop_checkpoint(*global_transientMemory, checkpoint);
				// pop_memory_checkpoint(*global_transientMemory);
			}

			f32 halfMapSize = mapSize / 2;
			v3 terrainOrigin = {-halfMapSize, -halfMapSize, 0};

			for (s32 i = 0; i < terrainCount; ++i)
			{
				s32 x = i % chunkCountPerDirection;
				s32 y = i / chunkCountPerDirection;

				v3 leftBackCornerPosition = {x * chunkWorldSize - halfMapSize, y * chunkWorldSize - halfMapSize, 0};
				game->terrainTransforms[i] = translation_matrix(leftBackCornerPosition);
			}

			// Todo(Leo): use better(dumber but smarter) thing to get rid of std move
			game->collisionSystem.terrainCollider 	= std::move(heightmap);
			game->collisionSystem.terrainOffset = {{-mapSize / 2, -mapSize / 2, 0}};

			MeshAssetData seaMeshAsset = {};
			{
				Vertex vertices []
				{
					{-0.5, -0.5, 0, 0, 0, 1, 1,1,1, 0, 0},
					{ 0.5, -0.5, 0, 0, 0, 1, 1,1,1, 1, 0},
					{-0.5,  0.5, 0, 0, 0, 1, 1,1,1, 0, 1},
					{ 0.5,  0.5, 0, 0, 0, 1, 1,1,1, 1, 1},
				};
				seaMeshAsset.vertices 	= push_and_copy_memory(*global_transientMemory, array_count(vertices), vertices); 

				u16 indices [] 			= {0,1,2,2,1,3};
				seaMeshAsset.indexCount = array_count(indices);
				seaMeshAsset.indices 	= push_and_copy_memory(*global_transientMemory, array_count(indices), indices);

				seaMeshAsset.indexType = IndexType::UInt16;
			}
			mesh_generate_tangents(seaMeshAsset);
			game->seaMesh 		= graphics_memory_push_mesh(platformGraphics, &seaMeshAsset);
			game->seaMaterial 	= assets_get_material(game->assets, MATERIAL_ASSET_SEA);
			game->seaTransform = transform_matrix({0,0,0}, identity_quaternion, {mapSize, mapSize, 1});
		}

		/// TOTEMS
		{
			game->totemMesh 	= assets_get_mesh(game->assets, MESH_ASSET_TOTEM);
			game->totemMaterial = assets_get_material(game->assets, MATERIAL_ASSET_ENVIRONMENT);

			v3 position0 = {0,0,0}; 	position0.z = get_terrain_height(game->collisionSystem, position0.xy);
			v3 position1 = {0,5,0}; 	position1.z = get_terrain_height(game->collisionSystem, position1.xy);

			game->totemTransforms[0] = {position0, {1,1,1}, identity_quaternion};
			game->totemTransforms[1] = {position1, {0.5, 0.5, 0.5}, identity_quaternion};

			push_box_collider(	game->collisionSystem,
								v3 {1.0, 1.0, 5.0},
								v3 {0,0,2},
								&game->totemTransforms[0]);

			push_box_collider(	game->collisionSystem,
								v3{0.5, 0.5, 2.5},
								v3{0,0,1},
								&game->totemTransforms[1]);
		}

		/// RACCOONS
		{
			game->raccoonCount = 4;
			push_multiple_memories(	persistentMemory,
									game->raccoonCount,
									ALLOC_ZERO_MEMORY,
									
									&game->raccoonModes,
									&game->raccoonTransforms,
									&game->raccoonTargetPositions,
									&game->raccoonCharacterMotors);

			for (s32 i = 0; i < game->raccoonCount; ++i)
			{
				game->raccoonModes[i]					= RACCOON_IDLE;

				game->raccoonTransforms[i] 			= identity_transform;
				game->raccoonTransforms[i].position 	= random_inside_unit_square() * 100 - v3{50, 50, 0};
				game->raccoonTransforms[i].position.z  = get_terrain_height(game->collisionSystem, game->raccoonTransforms[i].position.xy);

				game->raccoonTargetPositions[i] 		= random_inside_unit_square() * 100 - v3{50,50,0};
				game->raccoonTargetPositions[i].z  	= get_terrain_height(game->collisionSystem, game->raccoonTargetPositions[i].xy);

				// ------------------------------------------------------------------------------------------------
		
				game->raccoonCharacterMotors[i] = {};
				game->raccoonCharacterMotors[i].transform = &game->raccoonTransforms[i];

				{
					using namespace CharacterAnimations;

					game->raccoonCharacterMotors[i].animations[WALK] 		= assets_get_animation(game->assets, AAID_RACCOON_EMPTY);
					game->raccoonCharacterMotors[i].animations[RUN] 		= assets_get_animation(game->assets, AAID_RACCOON_EMPTY);
					game->raccoonCharacterMotors[i].animations[IDLE]		= assets_get_animation(game->assets, AAID_RACCOON_EMPTY);
					game->raccoonCharacterMotors[i].animations[JUMP] 		= assets_get_animation(game->assets, AAID_RACCOON_EMPTY);
					game->raccoonCharacterMotors[i].animations[FALL] 		= assets_get_animation(game->assets, AAID_RACCOON_EMPTY);
					game->raccoonCharacterMotors[i].animations[CROUCH] 	= assets_get_animation(game->assets, AAID_RACCOON_EMPTY);
				}
			}

			game->raccoonMesh 		= assets_get_mesh(game->assets, MESH_ASSET_RACCOON);
			game->raccoonMaterial	= assets_get_material(game->assets, MATERIAL_ASSET_RACCOON);
		}

		// /// INVISIBLE TEST COLLIDER
		// {
		// 	Transform3D * transform = game->transforms.push_return_pointer({});
		// 	transform->position = {21, 15};
		// 	transform->position.z = get_terrain_height(game->collisionSystem, {20, 20});

		// 	push_box_collider( 	game->collisionSystem,
		// 						v3{2.0f, 0.05f,5.0f},
		// 						v3{0,0, 2.0f},
		// 						transform);
		// }

		game->monuments = Game_load_monuments(persistentMemory, assets_get_material(game->assets, MATERIAL_ASSET_ENVIRONMENT), game->collisionSystem);

		// TEST ROBOT
		{
			v3 position 			= {21, 10, 0};
			position.z 				= get_terrain_height(game->collisionSystem, position.xy);
			game->robotTransform 	= {position};

			game->robotMesh 		= assets_get_mesh(game->assets, MESH_ASSET_ROBOT);
			game->robotMaterial 	= assets_get_material(game->assets, MATERIAL_ASSET_ROBOT);
		}

		/// SMALL SCENERY OBJECTS
		{			
			game->potMesh 		= assets_get_mesh(game->assets, MESH_ASSET_SMALL_POT);
			game->potMaterial 	= assets_get_material(game->assets, MATERIAL_ASSET_ENVIRONMENT);

			{
				game->potCapacity 		= 10;
				game->potCount 			= game->potCapacity;
				game->potTransforms 	= push_memory<Transform3D>(persistentMemory, game->potCapacity, ALLOC_GARBAGE);
				game->potWaterLevels 	= push_memory<f32>(persistentMemory, game->potCapacity, ALLOC_GARBAGE);

				for(s32 i = 0; i < game->potCapacity; ++i)
				{
					v3 position 			= {15, i * 5.0f, 0};
					position.z 				= get_terrain_height(game->collisionSystem, position.xy);
					game->potTransforms[i]	= { .position = position };
				}
			}

			// ----------------------------------------------------------------	

			auto bigPotMesh = assets_get_mesh(game->assets, MESH_ASSET_BIG_POT);

			s32 bigPotCount = 5;

			game->bigPotMesh 		= assets_get_mesh(game->assets, MESH_ASSET_BIG_POT);
			game->bigPotMaterial 	= assets_get_material(game->assets, MATERIAL_ASSET_ENVIRONMENT);
			game->bigPotTransforms 	= push_array_2<Transform3D>(persistentMemory, bigPotCount, ALLOC_GARBAGE);
			game->bigPotTransforms.count = bigPotCount;

			for (s32 i = 0; i < bigPotCount; ++i)
			{
				v3 position = {13, 2.0f + i * 4.0f}; 		position.z = get_terrain_height(game->collisionSystem, position.xy);

				Transform3D * transform = &game->bigPotTransforms[i];
				*transform 				= identity_transform;
				transform->position 	= position;

				f32 colliderHeight = 1.16;
				push_cylinder_collider(game->collisionSystem, 0.6, colliderHeight, v3{0,0,colliderHeight / 2}, transform);
			}


			// ----------------------------------------------------------------	

			/// WATERS
			{
				game->waterMesh 		= assets_get_mesh(game->assets, MESH_ASSET_WATER_DROP);
				game->waterMaterial 	= assets_get_material(game->assets, MATERIAL_ASSET_WATER);

				{				
					game->waters.capacity 		= 200;
					game->waters.count 		= 20;
					game->waters.transforms 	= push_memory<Transform3D>(persistentMemory, game->waters.capacity, ALLOC_GARBAGE);
					game->waters.levels 		= push_memory<f32>(persistentMemory, game->waters.capacity, ALLOC_GARBAGE);

					for (s32 i = 0; i < game->waters.count; ++i)
					{
						v3 position = {random_range(-50, 50), random_range(-50, 50)};
						position.z 	= get_terrain_height(game->collisionSystem, position.xy);

						game->waters.transforms[i] 	= { .position = position };
						game->waters.levels[i] 		= game->fullWaterLevel;
					}
				}
			}


		}

		/// TRAIN
		{
			game->trainMesh 		= assets_get_mesh(game->assets, MESH_ASSET_TRAIN);
			game->trainMaterial 	= assets_get_material(game->assets, MATERIAL_ASSET_ENVIRONMENT);

			// ----------------------------------------------------------------------------------

			game->trainStopPosition 	= {50, 0, 0};
			game->trainStopPosition.z 	= get_terrain_height(game->collisionSystem, game->trainStopPosition.xy);
			game->trainStopPosition.z 	= max_f32(0, game->trainStopPosition.z);

			f32 trainFarDistance 		= 2000;

			game->trainFarPositionA 	= {50, trainFarDistance, 0};
			game->trainFarPositionA.z 	= get_terrain_height(game->collisionSystem, game->trainFarPositionA.xy);
			game->trainFarPositionA.z 	= max_f32(0, game->trainFarPositionA.z);

			game->trainFarPositionB 	= {50, -trainFarDistance, 0};
			game->trainFarPositionB.z 	= get_terrain_height(game->collisionSystem, game->trainFarPositionB.xy);
			game->trainFarPositionB.z 	= max_f32(0, game->trainFarPositionB.z);

			game->trainTransform.position 			= game->trainStopPosition;

			game->trainMoveState 					= TRAIN_WAIT;

			game->trainFullSpeed 					= 200;
			game->trainStationMinSpeed 			= 1;
			game->trainAcceleration 				= 20;
			game->trainWaitTimeOnStop 				= 5;

			/*
			v = v0 + a*t
			-> t = (v - v0) / a
			d = d0 + v0*t + 1/2*a*t^2
			d - d0 = v0*t + 1/2*a*t^2
			*/

			f32 timeToBrakeBeforeStation 			= (game->trainFullSpeed - game->trainStationMinSpeed) / game->trainAcceleration;
			game->trainBrakeBeforeStationDistance 	= game->trainFullSpeed * timeToBrakeBeforeStation
													// Note(Leo): we brake, so acceleration term is negative
													- 0.5f * game->trainAcceleration * timeToBrakeBeforeStation * timeToBrakeBeforeStation;

			game->trainCurrentWaitTime = 0;
			game->trainCurrentSpeed 	= 0;

		}
	}

	// ----------------------------------------------------------------------------------

	/// MARCHING CUBES AND METABALLS TESTING
	{
		v3 position = {-10, -30, 0};
		position.z = get_terrain_height(game->collisionSystem, position.xy) + 3;

		s32 vertexCountPerCube 	= 14;
		s32 indexCountPerCube 	= 36;
		s32 cubeCapacity 		= 100000;
		s32 vertexCapacity 		= vertexCountPerCube * cubeCapacity;
		s32 indexCapacity 		= indexCountPerCube * cubeCapacity;

		Vertex * vertices 	= push_memory<Vertex>(persistentMemory, vertexCapacity, ALLOC_GARBAGE);
		u16 * indices 		= push_memory<u16>(persistentMemory, indexCapacity, ALLOC_GARBAGE);

		u32 vertexCount;
		u32 indexCount;

		game->metaballGridScale = 0.3;

		game->metaballVertexCount 	= vertexCount;
		game->metaballVertices 	= vertices;

		game->metaballIndexCount 	= indexCount;
		game->metaballIndices 		= indices;

		game->metaballVertexCapacity 	= vertexCapacity;
		game->metaballIndexCapacity 	= indexCapacity;

		game->metaballTransform 	= translation_matrix(position);

		game->metaballMaterial = assets_get_material(game->assets, MATERIAL_ASSET_TREE);

		// ----------------------------------------------------------------------------------

		game->metaballVertexCapacity2 	= vertexCapacity;
		game->metaballVertexCount2 	= 0;
		game->metaballVertices2 		= push_memory<Vertex>(persistentMemory, vertexCapacity, ALLOC_GARBAGE);

		game->metaballIndexCapacity2 	= vertexCapacity;
		game->metaballIndexCount2 		= 0;
		game->metaballIndices2 		= push_memory<u16>(persistentMemory, indexCapacity, ALLOC_GARBAGE);

		v3 position2 				= {0, -30, 0};
		position2.z 				= get_terrain_height(game->collisionSystem, position2.xy) + 3;
		game->metaballTransform2 	= translation_matrix(position2);

	}

	// ----------------------------------------------------------------------------------
	
	{	
		s32 treeCapacity 	= 50;	
		game->trees 		= push_array_2<Tree3>(persistentMemory, treeCapacity, ALLOC_ZERO_MEMORY);


		s32 initialTreesCount 	= 10;
		s32 halfCount 			= initialTreesCount / 2;

		for (s32 i = 0; i < halfCount; ++i)
		{
			v3 position = random_on_unit_circle_xy() * random_value() * 50;
			position.z = get_terrain_height(game->collisionSystem, position.xy);

			game_spawn_tree(*game, position, 0);
		}

		// ----------------------------------------------------------------------------------

		for (s32 i = halfCount; i < initialTreesCount; ++i)
		{
			v3 position = random_on_unit_circle_xy() * random_value() * 50;
			position.z = get_terrain_height(game->collisionSystem, position.xy);
			
			game_spawn_tree(*game, position, 1);
		}
	
		game->inspectedTreeIndex 	= 0;
	}


	// ----------------------------------------------------------------------------------

	log_application(0, "Game loaded, ", used_percent(*global_transientMemory) * 100, "% of transient memory used.");
	log_application(0, "Game loaded, ", used_percent(persistentMemory) * 100, "% of persistent memory used.");

	return game;
}
