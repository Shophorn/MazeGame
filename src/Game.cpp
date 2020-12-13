/*
Leo Tamminen
shophorn @ internet

Scene description for 3d development game

Todo(Leo):
	crystal trees 

Todo(leo): audio
	clipping, compressor, dynamic gain, soft knee, hard knee
*/

// Todo(Leo): Maybe try to get rid of this forward declaration
struct Game;
internal s32 game_spawn_tree(Game & game, v3 position, s32 treeTypeIndex, bool32 pushToPhysics = true);


#include "map.cpp"

#include "game_settings.cpp"

#include "CharacterMotor.cpp"
#include "PlayerController3rdPerson.cpp"
#include "FollowerController.cpp"



enum CameraMode : s32
{ 
	CameraMode_player, 
	CameraMode_elevator,
	CameraMode_mouse_and_keyboard,

	CameraModeCount
};

enum EntityType : s32
{ 	
	// Todo(Leo): none should be default value, but really it would be better to have it not
	// contribute to count, basically be below 0 or above count
	EntityType_none,
	
	EntityType_pot,
	EntityType_water,
	EntityType_raccoon,
	EntityType_tree_3,
	EntityType_box,

	EntityTypeCount
};

struct EntityReference
{
	EntityType 	type;
	s32 		index;
};

bool operator == (EntityReference const & a, EntityReference const & b)
{
	bool result = a.type == b.type && a.index == b.index;
	return result;
}

enum TrainState : s32
{
	TrainState_move,
	TrainState_wait,
};

enum NoblePersonMode : s32
{
	NoblePersonMode_wander_around,
	NoblePersonMode_wait_for_train,
	NoblePersonMode_away,
	NoblePersonMode_arriving_in_train,
};

enum RaccoonMode : s32
{
	RaccoonMode_idle,
	RaccoonMode_flee,
	RaccoonMode_carried,
};


// Todo(Leo): Do a stack of these instead of enum.
// struct MenuView
// {
// 	void (*view_func)(Menu & menu, void * data);
// 	void * data;	
// };

enum MenuView : s32
{
	MenuView_off,
	MenuView_main,
	MenuView_confirm_exit,
	MenuView_confirm_teleport,
	MenuView_edit_sky,
	MenuView_edit_mesh_generation,
	MenuView_edit_tree,
	MenuView_save_complete,
	MenuView_edit_monuments,
	MenuView_edit_camera,
	MenuView_edit_clouds,

	MenuView_spawn,
};


struct PhysicsEntity
{
	// EntityType 	type;
	// s32 			index;
	EntityReference entity;
	f32 			velocity;
};


struct PhysicsWorld
{
	Array<PhysicsEntity> entities;
};

internal void initialize_physics_world(PhysicsWorld & physicsWorld, MemoryArena & allocator)
{
	physicsWorld 			= {};
	physicsWorld.entities 	= push_array<PhysicsEntity>(allocator, 1000, ALLOC_GARBAGE);
}

internal void physics_world_push_entity(PhysicsWorld & physicsWorld, EntityReference entity)
{
	Assert(physicsWorld.entities.count < physicsWorld.entities.capacity);
	physicsWorld.entities[physicsWorld.entities.count++] = {entity, 0};
}

#include "metaballs.cpp"
#include "dynamic_mesh.cpp"
#include "sky_settings.cpp"

// Note(Leo): This maybe seems nice?
#include "game_assets.cpp"
#include "game_monuments.cpp"
#include "game_waters.cpp"
#include "game_clouds.cpp"
#include "game_leaves.cpp"
#include "game_trees.cpp"

// Todo(Leo): actually other way aroung, but now scene saving button is on blocks editor
#include "scene_data.cpp"
#include "game_building_blocks.cpp"


struct MenuState
{
	MenuView 	view;
	s32 		selectedIndex;
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

enum BoxState : s32
{
	BoxState_closed,
	BoxState_opening,
	BoxState_open,
	BoxState_closing,
};


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
		s32 				potCapacity;
		s32 				potCount;
		Transform3D * 		potTransforms;
		f32 * 				potWaterLevels;
		EntityReference * 	potCarriedEntities;

		MeshHandle 		potMesh;
		MaterialHandle 	potMaterial;

		MeshHandle 			bigPotMesh;
		MaterialHandle		bigPotMaterial;
		Array<Transform3D> 	bigPotTransforms;

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

	EntityReference playerCarriedEntity;

	PhysicsWorld physicsWorld;

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
	f32 		timeScale = 1;

	Trees trees;

	s32 				boxCount;
	Transform3D * 		boxTransforms;
	Transform3D * 		boxCoverLocalTransforms;
	BoxState * 			boxStates;
	f32 * 				boxOpenPercents;
	EntityReference * 	boxCarriedEntities;

	Clouds clouds;

	// ----------------------------------------------

	Scene scene;

	s64 selectedBuildingBlockIndex;

	// ----------------------------------------------
	
	// AUDIO

	AudioAsset * backgroundAudio;
	AudioAsset * stepSFX;
	AudioAsset * stepSFX2;
	AudioAsset * stepSFX3;

	AudioClip 			backgroundAudioClip;
	Array<AudioClip> 	audioClipsOnPlay;
};


internal void update_physics_world(PhysicsWorld & physics, Game * game, f32 elapsedTime)
{
	for (s32 i = 0; i < physics.entities.count; ++i)
	{
		bool cancelFall = physics.entities[i].entity == game->playerCarriedEntity;

		s32 index 		= physics.entities[i].entity.index;
		// bool cancelFall = index == game->playerCarriedEntity.index;

		f32 & velocity = physics.entities[i].velocity;
		v3 * position;

		switch (physics.entities[i].entity.type)
		{
			case EntityType_raccoon:	position = &game->raccoonTransforms[index].position; 	break;
			case EntityType_water: 		position = &game->waters.positions[index]; 				break;
			case EntityType_pot:		position = &game->potTransforms[index].position;		break;
			case EntityType_tree_3: 	position = &game->trees.array[index].position; 			break;
			case EntityType_box:		position = &game->boxTransforms[index].position;		break;

			default:
				Assert(false && "That item has not physics properties");
		}
	
		f32 groundHeight = get_terrain_height(game->collisionSystem, position->xy);

		if (cancelFall)
		{
			array_unordered_remove(physics.entities, i);
			i -= 1;

			continue;
		}

		// Todo(Leo): this is in reality dependent on frame rate, this works for development capped 30 ms frame time
		// but is lower when frametime is lower
		constexpr f32 physicsSleepThreshold = 0.2;

		if (velocity < 0 && position->z < groundHeight)
		{
			position->z = groundHeight;

			{
				// log_debug(FILE_ADDRESS, velocity);
				f32 maxVelocityForAudio = 10;
				f32 velocityForAudio 	= f32_clamp(-velocity, 0, maxVelocityForAudio);
				f32 volume 				= velocityForAudio / maxVelocityForAudio;

				game->audioClipsOnPlay.push({game->stepSFX, 0, random_range(0.5, 1.5), volume, *position});
			}


			// todo (Leo): move to physics properties per game object type
			f32 bounceFactor 	= 0.5;
			velocity 			= -velocity * bounceFactor;

			if (velocity < physicsSleepThreshold)
			{
				array_unordered_remove(physics.entities, i);
				i -= 1;
			}

		}
		else
		{
			velocity 		+= elapsedTime * physics_gravity_acceleration;
			position->z 	+= elapsedTime * velocity;
		}
	}
}


internal auto game_get_serialized_objects(Game & game)
{
	auto serializedObjects = make_property_list
	(
		serialize_object("sky", game.skySettings),
		serialize_object("tree_0", game.trees.settings[0]),
		serialize_object("tree_1", game.trees.settings[1]),
		serialize_object("player_camera", game.playerCamera)
	);

	return serializedObjects;
}

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

		waters_instantiate(game.waters, game.physicsWorld, position, game.fullWaterLevel);
	}
}

internal s32 game_spawn_tree(Game & game, v3 position, s32 treeTypeIndex, bool32 pushToPhysics)
{
	// Assert(game.trees.count < game.trees.capacity);
	if (game.trees.array.count >= game.trees.array.capacity)
	{
		// Todo(Leo): do something more interesting
		log_debug(FILE_ADDRESS, "Trying to spawn tree, but out of tree capacity");
		return -1;
	}

	s32 index = game.trees.array.count++;
	Tree & tree = game.trees.array[index];

	reset_tree_3(tree, &game.trees.settings[treeTypeIndex], position);

	tree.typeIndex 	= treeTypeIndex;
	tree.game 		= &game;

	MeshAssetId seedMesh = treeTypeIndex == 0 ? MeshAssetId_seed : MeshAssetId_water_drop;

	tree.leaves.material 	= assets_get_material(game.assets, MaterialAssetId_leaves);
	tree.seedMesh 			= assets_get_mesh(game.assets, seedMesh);
	tree.seedMaterial		= assets_get_material(game.assets, MaterialAssetId_seed);

	if (pushToPhysics)
	{
		physics_world_push_entity(game.physicsWorld, {EntityType_tree_3, (s32)game.trees.array.count - 1});
	}

	return index;
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


// Note(Leo): These seem to naturally depend on the game struct, so they are here.
// Todo(Leo): This may be a case for header file, at least for game itself
#include "game_gui.cpp"

// Todo(Leo): add this to syntax higlight, so that 'GAME UPDATE' is different color
/// ------------------ GAME UPDATE -------------------------
internal bool32 game_game_update(Game * 				game,
								PlatformInput * 		input,
								StereoSoundOutput *	soundOutput,
								f32 					elapsedTimeSeconds)
{
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

	f32 scaledTime 		= elapsedTimeSeconds * game->timeScale;
	f32 unscaledTime 	= elapsedTimeSeconds;

	/// ****************************************************************************

	gui_start_frame(game->gui, input, unscaledTime);

	/* Sadly, we need to draw skybox before game logic, because otherwise
	debug lines would be hidden. This can be fixed though, just make debug pipeline similar to shadows. */ 
	graphics_draw_model(platformGraphics, game->skybox, identity_m44, false, nullptr, 0);
	// graphics_draw_meshes(platformGraphics,
	// 					1, &identity_m44,
	// 					assets_get_mesh(game->assets, MeshAssetId_skysphere),
	// 					assets_get_material(game->assets, MaterialAssetId_sky));


	bool playerInputAvailable = game->cameraMode == CameraMode_player;

	// Game Logic section
	switch (game->cameraMode)
	{
		case CameraMode_player:
		{
			CharacterInput playerCharacterMotorInput = {};

			if (playerInputAvailable)
			{
				// if (input_button_went_down(input, InputButton_dpad_down))
				// {
				// 	game_gui_push_menu(game, MenuView_spawn);
				// }

				if (input_button_went_down(input, InputButton_nintendo_y))
				{
					game->nobleWanderTargetPosition = game->playerCharacterTransform.position;
					game->nobleWanderWaitTimer 		= 0;
					game->nobleWanderIsWaiting 		= false;
				}

				playerCharacterMotorInput = update_player_input(game->playerInputState, game->worldCamera, input);
			}

			update_character_motor(game->playerCharacterMotor, playerCharacterMotorInput, game->collisionSystem, scaledTime, DEBUG_LEVEL_PLAYER);

			update_camera_controller(game->playerCamera, game->playerCharacterTransform.position, input, scaledTime);

			game->worldCamera.position = game->playerCamera.position;
			game->worldCamera.direction = game->playerCamera.direction;

			game->worldCamera.farClipPlane = 1000;

			/// ---------------------------------------------------------------------------------------------------

			FS_DEBUG_NPC(debug_draw_circle_xy(game->nobleWanderTargetPosition + v3{0,0,0.5}, 1.0, colour_bright_green));
			FS_DEBUG_NPC(debug_draw_circle_xy(game->nobleWanderTargetPosition + v3{0,0,0.5}, 0.9, colour_bright_green));
		} break;

		case CameraMode_elevator:
		{
			m44 cameraMatrix = update_free_camera(game->freeCamera, input, unscaledTime);

			game->worldCamera.position = game->freeCamera.position;
			game->worldCamera.direction = game->freeCamera.direction;

			// /// Document(Leo): Teleport player
			// if (game_gui_menu_visible(game) == false && input_button_went_down(input, InputButton_nintendo_a))
			// {
			// 	// game->menuView = MenuView_confirm_teleport;
			// 	game_gui_push_menu(game, MenuView_confirm_teleport);
			// 	gui_ignore_input();
			// 	gui_reset_selection();
			// }

			game->worldCamera.farClipPlane = 2000;
		} break;

		case CameraMode_mouse_and_keyboard:
		{
			update_mouse_camera(game->mouseCamera, input, unscaledTime);

			game->worldCamera.position = game->mouseCamera.position;
			game->worldCamera.direction = game->mouseCamera.direction;

			// /// Document(Leo): Teleport player
			// if (game_gui_menu_visible(game) == false && input_button_went_down(input, InputButton_nintendo_a))
			// {
			// 	// game->menuView = MenuView_confirm_teleport;
			// 	game_gui_push_menu(game, MenuView_confirm_teleport);
			// 	gui_ignore_input();
			// 	gui_reset_selection();
			// }

			game->worldCamera.farClipPlane = 2000;
		} break;

		case CameraModeCount:
			Assert(false && "Bad execution path");
			break;
	}


	/// *******************************************************************************************

	update_camera_system(&game->worldCamera, platformGraphics, platformWindow);

	{
		v3 lightDirection = {0,0,1};
		lightDirection = rotate_v3(quaternion_axis_angle(v3_right, game->skySettings.sunHeightAngle * π), lightDirection);
		lightDirection = rotate_v3(quaternion_axis_angle(v3_up, game->skySettings.sunOrbitAngle * 2 * π), lightDirection);
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
	
	// Todo(leo): Should these be handled differently????
	f32 playerPickupDistance 	= 1.0f;
	f32 playerInteractDistance 	= 1.0f;

	/// PICKUP OR DROP
	if (playerInputAvailable && game->playerInputState.events.pickupOrDrop)
	{
		v3 playerPosition = game->playerCharacterTransform.position;

		switch(game->playerCarriedEntity.type)
		{
			case EntityType_none:
			{
				bool pickup = true;

				if (pickup)
				{
					for(s32 i  = 0; i < game->boxCount; ++i)
					{
						f32 boxPickupDistance = 0.5f + playerPickupDistance;
						if (v3_length(playerPosition - game->boxTransforms[i].position) < boxPickupDistance)
						{
							bool32 canPickInsides = game->boxCarriedEntities[i].type != EntityType_none
													&& game->boxStates[i] == BoxState_open;

							if (canPickInsides == false)
							{
								game->playerCarriedEntity = {EntityType_box, i};
							}
							else
							{
								game->playerCarriedEntity = game->boxCarriedEntities[i];
								game->boxCarriedEntities[i] = {EntityType_none};
							}

							pickup = false;
						}
					}
				}

				v3 playerPosition = game->playerCharacterTransform.position;

				/* Todo(Leo): Do this properly, taking into account player facing direction and distances etc. */
				auto check_pickup = [&](s32 count, Transform3D const * transforms, EntityType entityType)
				{
					for (s32 i = 0; i < count; ++i)
					{
						if (v3_length(playerPosition - transforms[i].position) < playerPickupDistance)
						{
							game->playerCarriedEntity = {entityType, i};

							pickup = false;
						}
					}
				};


				if (pickup)
				{
					check_pickup(game->potCount, game->potTransforms, EntityType_pot);
				}

				if (pickup)
				{
					for (s32 i = 0; i < game->waters.count; ++i)
					{
						if (v3_length(playerPosition - game->waters.positions[i]) < playerPickupDistance)
						{
							game->playerCarriedEntity = {EntityType_water, i};
							pickup = false;
						}
					}


				}


				if (pickup)
				{
					check_pickup(game->raccoonCount, game->raccoonTransforms, EntityType_raccoon);
				}

				if (pickup)
				{
					for (s32 i = 0; i < game->trees.array.count; ++i)
					{
						Tree & tree = game->trees.array[i];

						if (v3_length(playerPosition - game->trees.array[i].position) < playerPickupDistance)
						{
							// if (game->trees.array[i].planted == false)
							{
								game->playerCarriedEntity = {EntityType_tree_3, i};
							}

							pickup = false;
						}

					}
				}
			} break;

			case EntityType_raccoon:
				game->raccoonTransforms[game->playerCarriedEntity.index].rotation = identity_quaternion;
				// Todo(Leo): notice no break here, little sketcthy, maybe do something about it, probably in raccoon movement code

			case EntityType_pot:
			case EntityType_tree_3:
			case EntityType_box:	
			case EntityType_water:
				physics_world_push_entity(game->physicsWorld, game->playerCarriedEntity);
				game->playerCarriedEntity = {EntityType_none};
				break;

			default:
				log_debug(FILE_ADDRESS, "cannot pickup or drop that entity: (", game->playerCarriedEntity.type, ", #", game->playerCarriedEntity.index, ")");
		}
	}

	f32 boxInteractDistance = 0.5f + playerInteractDistance;

	if (game->playerInputState.events.interact)
	{
		bool32 interact = game->playerInputState.events.interact;

		// TRY PUT STUFF INTO BOX
		bool playerCarriesContainableEntity = 	game->playerCarriedEntity.type != EntityType_none
												&& game->playerCarriedEntity.type != EntityType_box;

		if (interact && playerCarriesContainableEntity)
		{
			for (s32 i = 0; i < game->boxCount; ++i)
			{
				f32 distanceToBox = v3_length(game->boxTransforms[i].position - game->playerCharacterTransform.position);
				if (distanceToBox < boxInteractDistance && game->boxCarriedEntities[i].type == EntityType_none)
				{
					game->boxCarriedEntities[i] = game->playerCarriedEntity;
					game->playerCarriedEntity = {EntityType_none};

					interact = false;
				}				
			}
		}

		// TRY PUT STUFF INTO POT
		bool playerCarriesPottableEntity = 	game->playerCarriedEntity.type == EntityType_tree_3
											|| game->playerCarriedEntity.type == EntityType_raccoon;

		if (interact && playerCarriesPottableEntity)
		{
			log_debug("Try put something to pot");

			for (s32 i = 0; i < game->potCount; ++i)
			{
				f32 distanceToPot = v3_length(game->potTransforms[i].position - game->playerCharacterTransform.position);
				if (distanceToPot < playerInteractDistance && game->potCarriedEntities[i].type == EntityType_none)
				{
					log_debug(FILE_ADDRESS, "Stuff put into pot");

					game->potCarriedEntities[i] = game->playerCarriedEntity;
					game->playerCarriedEntity 	= {EntityType_none};

					interact = false;
				}
			}
		}
		
		// TRY OPEN OR CLOSE NEARBY BOX
		bool32 playerCarriesNothing = game->playerCarriedEntity.type == EntityType_none;

		if (interact && playerCarriesNothing)
		{
			for (s32 i = 0; i < game->boxCount; ++i)
			{
				f32 distanceToBox = v3_length(game->boxTransforms[i].position - game->playerCharacterTransform.position);
				if (distanceToBox < boxInteractDistance)
				{
					if (game->boxStates[i] == BoxState_closed)
					{
						game->boxStates[i] 			= BoxState_opening;
						game->boxOpenPercents[i] 	= 0;

						interact = false;
					}

					else if (game->boxStates[i] == BoxState_open)
					{
						game->boxStates[i] 			= BoxState_closing;
						game->boxOpenPercents[i] 	= 0;

						interact = false;
					}
				}
			}
		}

		// PLANT THE CARRIED TREE
		bool32 playerCarriesTree = game->playerCarriedEntity.type == EntityType_tree_3;

		if (interact && playerCarriesTree)
		{
			game->trees.array[game->playerCarriedEntity.index].planted = true;
			v3 position = game->trees.array[game->playerCarriedEntity.index].position;
			position.z = get_terrain_height(game->collisionSystem, position.xy);
			game->trees.array[game->playerCarriedEntity.index].position = position;
			game->playerCarriedEntity.index = -1;
			game->playerCarriedEntity.type = EntityType_none;			

			interact = false;
		}

	}


	for(s32 i = 0; i < game->boxCount; ++i)
	{
		v4 colour = game->boxCarriedEntities[i].type == EntityType_none ? colour_bright_red : colour_bright_green;
		FS_DEBUG_NPC(debug_draw_circle_xy(game->boxTransforms[i].position + v3{0,0,0.6}, 1, colour));
		FS_DEBUG_NPC(debug_draw_circle_xy(game->boxTransforms[i].position + v3{0,0,0.6}, 1.5, colour_bright_cyan));
	}


	auto update_carried_entities_transforms = [&](	s32 				count,
													Transform3D * 		transforms,
													EntityReference * 	carriedEntities,
													v3 					carriedOffset)
	{
		for (s32 i = 0; i < count; ++i)
		{
			v3 carriedPosition 			= multiply_point(transform_matrix(transforms[i]), carriedOffset);
			quaternion carriedRotation 	= transforms[i].rotation;
		
			// Todo(Leo): maybe something like this??
			// entity_set_position(game->playerCarriedEntity, carriedPosition);
			// entity_set_rotation(game->playerCarriedEntity, carriedRotation);
			// entity_set_state(game->playerCarriedEntity, EntityState_carried_by_player);

			// Todo(Leo): this at least is super random pointer access, that also happens quite frequently
			switch(carriedEntities[i].type)
			{
				case EntityType_pot:
					game->potTransforms[carriedEntities[i].index].position = carriedPosition;
					game->potTransforms[carriedEntities[i].index].rotation = carriedRotation;
					break;

				case EntityType_box:
					game->boxTransforms[carriedEntities[i].index].position = carriedPosition;
					game->boxTransforms[carriedEntities[i].index].rotation = carriedRotation;
					break;

				case EntityType_water:
					game->waters.positions[carriedEntities[i].index] = carriedPosition;
					game->waters.rotations[carriedEntities[i].index] = carriedRotation;
					break;

				case EntityType_raccoon:
				{
					game->raccoonTransforms[carriedEntities[i].index].position 	= carriedPosition + v3{0,0,0.2};

					v3 right = rotate_v3(carriedRotation, v3_right);
					game->raccoonTransforms[carriedEntities[i].index].rotation 	= carriedRotation * quaternion_axis_angle(right, 1.4f);
				} break;

				case EntityType_tree_3:
					game->trees.array[carriedEntities[i].index].position = carriedPosition;
					game->trees.array[carriedEntities[i].index].rotation = carriedRotation;
					break;

				default:
					Assert(carriedEntities[i].type == EntityType_none && "That cannot be carried!");
					break;
			}
		}
	};

	update_carried_entities_transforms(1, &game->playerCharacterTransform, &game->playerCarriedEntity, {0, 0.7, 0.7});
	update_carried_entities_transforms(game->boxCount, game->boxTransforms, game->boxCarriedEntities, {0, 0, 0.1});
	update_carried_entities_transforms(game->potCount, game->potTransforms, game->potCarriedEntities, {0, 0, 0.1});

	// UPDATE BOX COVER
	for (s32 i = 0; i < game->boxCount; ++i)
	{
		constexpr f32 openAngle 	= 5.0f/8.0f * π;
		constexpr f32 openingTime 	= 0.7f;

		if (game->boxStates[i] == BoxState_opening)
		{
			game->boxOpenPercents[i] += scaledTime / openingTime;

			if (game->boxOpenPercents[i] > 1.0f)
			{
				game->boxStates[i] 			= BoxState_open;
				game->boxOpenPercents[i] 	= 1.0f;
			}

			f32 angle = openAngle * game->boxOpenPercents[i];
			game->boxCoverLocalTransforms[i].rotation = quaternion_axis_angle(v3_right, angle);
		}
		else if (game->boxStates[i] == BoxState_closing)
		{
			game->boxOpenPercents[i] += scaledTime / openingTime;

			if (game->boxOpenPercents[i] > 1.0f)
			{
				game->boxStates[i] 			= BoxState_closed;
				game->boxOpenPercents[i] 	= 1.0f;
			}

			f32 angle = openAngle * (1 - game->boxOpenPercents[i]);
			game->boxCoverLocalTransforms[i].rotation = quaternion_axis_angle(v3_right, angle);
		}
	}

	update_waters(game->waters, scaledTime);
	update_clouds(game->clouds, game->waters, game->physicsWorld, game->collisionSystem, scaledTime);
	update_physics_world(game->physicsWorld, game, scaledTime);

	
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

		if (game->trainMoveState == TrainState_move)
		{

			v3 trainMoveVector 	= game->trainCurrentTargetPosition - game->trainTransform.position;
			f32 directionDot 	= dot_v3(trainMoveVector, game->trainCurrentDirection);
			f32 moveDistance	= v3_length(trainMoveVector);

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
				game->trainMoveState 		= TrainState_wait;
				game->trainCurrentWaitTime = 0;
			}

		}
		else
		{
			game->trainCurrentWaitTime += scaledTime;
			if (game->trainCurrentWaitTime > game->trainWaitTimeOnStop)
			{
				game->trainMoveState 		= TrainState_move;
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
			case NoblePersonMode_wander_around:
			{
				if (game->nobleWanderIsWaiting)
				{
					game->nobleWanderWaitTimer -= scaledTime;
					if (game->nobleWanderWaitTimer < 0)
					{
						game->nobleWanderTargetPosition = { random_range(-99, 99), random_range(-99, 99)};
						game->nobleWanderIsWaiting = false;
					}


					m44 gizmoTransform = make_transform_matrix(	game->noblePersonTransform.position + v3_up * game->noblePersonTransform.scale.z * 2.0f, 
																game->noblePersonTransform.rotation,
																game->nobleWanderWaitTimer);
					FS_DEBUG_NPC(debug_draw_diamond_xz(gizmoTransform, colour_muted_red));
				}
				
				f32 distance = v2_length(game->noblePersonTransform.position.xy - game->nobleWanderTargetPosition.xy);
				if (distance < 1.0f && game->nobleWanderIsWaiting == false)
				{
					game->nobleWanderWaitTimer = 10;
					game->nobleWanderIsWaiting = true;
				}


				v3 input 			= {};
				input.xy	 		= game->nobleWanderTargetPosition.xy - game->noblePersonTransform.position.xy;
				f32 inputMagnitude 	= v3_length(input);
				input 				= input / inputMagnitude;
				inputMagnitude 		= f32_clamp(inputMagnitude, 0.0f, 1.0f);
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
		// Note(Leo): +1 for player
		s32 maxCarriedRaccoons = game->boxCount + game->potCount + 1;
		Array<s32> carriedRaccoonIndices = push_array<s32>(*global_transientMemory, maxCarriedRaccoons, ALLOC_GARBAGE);
		{
			if(game->playerCarriedEntity.type == EntityType_raccoon)
			{
				carriedRaccoonIndices.push(game->playerCarriedEntity.index);
			}
			
			for (s32 i = 0; i < game->boxCount; ++i)
			{
				if (game->boxCarriedEntities[i].type == EntityType_raccoon)
				{
					carriedRaccoonIndices.push(game->boxCarriedEntities[i].index);
				}
			}

			for (s32 i = 0; i < game->potCount; ++i)
			{
				if (game->potCarriedEntities[i].type == EntityType_raccoon)
				{
					f32 escapeChance = 0.001;
					if (random_value() < escapeChance)
					{
						game->potCarriedEntities[i] = {EntityType_none};
					}			
					else
					{
						carriedRaccoonIndices.push(game->potCarriedEntities[i].index);
					}
				}
			}
		}


		for(s32 i = 0; i < game->raccoonCount; ++i)
		{
			CharacterInput raccoonCharacterMotorInput = {};
	
			v3 toTarget 			= game->raccoonTargetPositions[i] - game->raccoonTransforms[i].position;
			f32 distanceToTarget 	= v3_length(toTarget);

			v3 input = {};

			if (distanceToTarget < 1.0f)
			{
				game->raccoonTargetPositions[i] 	= snap_on_ground(random_inside_unit_square() * 100 - v3{50,50,0});
				toTarget 							= game->raccoonTargetPositions[i] - game->raccoonTransforms[i].position;
			}
			else
			{
				input.xy	 		= game->raccoonTargetPositions[i].xy - game->raccoonTransforms[i].position.xy;
				f32 inputMagnitude 	= v3_length(input);
				input 				= input / inputMagnitude;
				inputMagnitude 		= f32_clamp(inputMagnitude, 0.0f, 1.0f);
				input 				= input * inputMagnitude;
			}

			raccoonCharacterMotorInput = {input, false, false};


			bool isCarried = false;
			for (s32 carriedRaccoonIndex : carriedRaccoonIndices)
			{
				if (carriedRaccoonIndex == i)
				{
					isCarried = true;
					break;
				}
			}


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


	/// SUBMIT COLLIDERS
	// Note(Leo): These colliders are used mainly in next game loop, since we update them here after everything has moved
	// Make a proper decision whether or not this is something we need
	{
		clear_colliders(game->collisionSystem);
		monuments_submit_colliders(game->monuments, game->collisionSystem);

		// Todo(Leo): Maybe make something like that these would have predetermined range, that is only updated when
		// tree has grown a certain amount or somthing. These are kinda semi-permanent by nature.

		auto submit_cylinder_colliders = [&collisionSystem = game->collisionSystem](f32 radius, f32 halfHeight, s32 count, Transform3D * transforms)
		{
			for (s32 i = 0; i < count; ++i)
			{
				submit_cylinder_collider(collisionSystem, {radius, halfHeight, v3{0, 0, halfHeight}}, transforms[i]);
			}
		};

		f32 smallPotColliderRadius = 0.3;
		f32 smallPotColliderHeight = 0.58;
		f32 smallPotHalfHeight = smallPotColliderHeight / 2;
		submit_cylinder_colliders(smallPotColliderRadius, smallPotHalfHeight, game->potCount, game->potTransforms);

		f32 bigPotColliderRadius 	= 0.6;
		f32 bigPotColliderHeight 	= 1.16;
		f32 bigPotHalfHeight 		= bigPotColliderHeight / 2;
		submit_cylinder_colliders(bigPotColliderRadius, bigPotHalfHeight, game->bigPotTransforms.count, game->bigPotTransforms.memory);


		// NEW TREES 3
		for (auto tree : game->trees.array)
		{
			CylinderCollider collider =
			{
				.radius 	= tree.nodes[tree.branches[0].startNodeIndex].radius,
				.halfHeight = 1.0f,
				.center 	= {0,0,1}
			};

			submit_cylinder_collider(game->collisionSystem, collider, {tree.position, identity_quaternion, {1,1,1}});
		}

		submit_box_collider(game->collisionSystem, {{0.5,0.5,0.5}, identity_quaternion, {0,0,0.5}}, game->boxTransforms[0]);
		submit_box_collider(game->collisionSystem, {{0.5,0.5,0.5}, identity_quaternion, {0,0,0.5}}, game->boxTransforms[1]);

		/// BUILDING BLOCKS
		for (s32 i = 0; i < game->scene.buildingBlocks.count; ++i)
		{
			// v3 position = multiply_point(game->scene.buildingBlocks[i], {0,0,0});
			// v3 scale 	= multiply_direction(game->scene.buildingBlocks[i], {1,1,1});

			// v3 position;
			// v3 eulerRotation;
			// v3 scale;

			// ImGuizmo::DecomposeMatrixToComponents(reinterpret_cast<f32*>(&game->scene.buildingBlocks[i]),
			// 										reinterpret_cast<f32*>(&position),
			// 										reinterpret_cast<f32*>(&eulerRotation),
			// 										reinterpret_cast<f32*>(&scale));

			// https://math.stackexchange.com/questions/893984/conversion-of-rotation-matrix-to-quaternion

			// eulerRotation *= (π / 180.0f);

			submit_box_collider(game->collisionSystem,
								{{1,1,1}, identity_quaternion, {0,0,0}},
								game->scene.buildingBlocks[i]);
								// {position, euler_angles_quaternion(eulerRotation), scale});
								// {game->scene.buildingBlocks[i], identity_quaternion, {1,1,1}});
		}
	}

	update_skeleton_animator(game->playerSkeletonAnimator, scaledTime);
	update_skeleton_animator(game->noblePersonSkeletonAnimator, scaledTime);
	
	/// DRAW POTS
	{
		local_persist float testScale = 1;
		constexpr float minTestScale = 0.5;
		constexpr float maxTestScale = 19;

		testScale += input_axis_get_value(input, InputAxis_mouse_scroll);
		testScale = f32_clamp(testScale, minTestScale, maxTestScale);

		// Todo(Leo): store these as matrices, we can easily retrieve position (that is needed somwhere) from that too.
		m44 * potTransformMatrices = push_memory<m44>(*global_transientMemory, game->potCount, ALLOC_GARBAGE);
		for(s32 i = 0; i < game->potCount; ++i)
		{
			potTransformMatrices[i] = transform_matrix(game->potTransforms[i]) * scale_matrix({testScale, testScale, testScale});
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

		monuments_draw(game->monuments, game->assets);
	}


	{
		MeshHandle boxMesh 		= assets_get_mesh(game->assets, MeshAssetId_box);
		MeshHandle boxCoverMesh = assets_get_mesh(game->assets, MeshAssetId_box_cover);
		MaterialHandle material = assets_get_material(game->assets, MaterialAssetId_box);

		m44 * boxTransformMatrices = push_memory<m44>(*global_transientMemory, game->boxCount, ALLOC_GARBAGE);
		m44 * coverTransformMatrices = push_memory<m44>(*global_transientMemory, game->boxCount, ALLOC_GARBAGE);

		for (s32 i = 0; i < game->boxCount; ++i)
		{
			boxTransformMatrices[i] = transform_matrix(game->boxTransforms[i]);
			coverTransformMatrices[i] = boxTransformMatrices[i] * transform_matrix(game->boxCoverLocalTransforms[i]);
		}

		graphics_draw_meshes(platformGraphics, game->boxCount, boxTransformMatrices, boxMesh, material);
		graphics_draw_meshes(platformGraphics, game->boxCount, coverTransformMatrices, boxCoverMesh, material);
	}

	/// DEBUG DRAW COLLIDERS
	{
		FS_DEBUG_BACKGROUND(collisions_debug_draw_colliders(game->collisionSystem));
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

			f32 t = min_f32(1, max_f32(0, dot_v3(position - a, b - a) / square_v3_length(b-a)));
			f32 d = v3_length(position - a  - t * (b -a));

			return d - f32_lerp(0.5,0.1,t);

			// return min_f32(v3_length(a - position) - rA, v3_length(b - position) - rB);
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
	draw_clouds(game->clouds, platformGraphics, game->assets);
	draw_waters(game->waters, platformGraphics, game->assets);

	for (auto & tree : game->trees.array)
	{
		GetWaterFunc get_water = {game->waters, game->playerCarriedEntity.index, game->playerCarriedEntity.type == EntityType_water };
		update_tree_3(tree, scaledTime, get_water);
		
		tree.leaves.position = tree.position;
		tree.leaves.rotation = tree.rotation;


		m44 transform = transform_matrix(tree.position, tree.rotation, {1,1,1});
		// Todo(Leo): make some kind of SLOPPY_ASSERT macro thing
		if (tree.mesh.vertices.count > 0 || tree.mesh.indices.count > 0)
		{
			graphics_draw_procedural_mesh(	platformGraphics,
											tree.mesh.vertices.count, tree.mesh.vertices.memory,
											tree.mesh.indices.count, tree.mesh.indices.memory,
											transform, assets_get_material(game->assets, MaterialAssetId_tree));
		}

		if (tree.drawSeed)
		{
			m44 seedTransform = transform;
			if (tree.planted)
			{
				seedTransform[3].z -= 0.3;
			}
			graphics_draw_meshes(platformGraphics, 1, &seedTransform, tree.seedMesh, tree.seedMaterial);
		}

		if (tree.hasFruit)
		{
			v3 fruitPosition = multiply_point(transform, tree.fruitPosition);

			f32 fruitSize = tree.fruitAge / tree.fruitMaturationTime;
			v3 fruitScale = {fruitSize, fruitSize, fruitSize};

			m44 fruitTransform = transform_matrix(fruitPosition, identity_quaternion, fruitScale);

			FS_DEBUG_ALWAYS(debug_draw_circle_xy(fruitPosition, 0.5, colour_bright_red));

			graphics_draw_meshes(platformGraphics, 1, &fruitTransform, tree.seedMesh, tree.seedMaterial);
		}

		FS_DEBUG_BACKGROUND(debug_draw_circle_xy(tree.position, 2, colour_bright_red));
	}

	{
		for (auto & tree : game->trees.array)
		{
			draw_leaves(tree.leaves, scaledTime, tree.settings->leafSize, tree.settings->leafColour);
		}
	}

	{
		m44 * buildingBlockTransforms = push_memory<m44>(*global_transientMemory, game->scene.buildingBlocks.count, ALLOC_GARBAGE);
		for (s64 i = 0; i < game->scene.buildingBlocks.count; ++i)
		{
			buildingBlockTransforms[i] = game->scene.buildingBlocks[i];
		}
		graphics_draw_meshes(	platformGraphics,
								game->scene.buildingBlocks.count,
								buildingBlockTransforms,
								assets_get_mesh(game->assets, MeshAssetId_cube),
								assets_get_material(game->assets, MaterialAssetId_building_block));
	}

	// ---------- PROCESS AUDIO -------------------------

	{
		local_persist f32 timeToSpawnAudioOnOtherGuy = 0;
		timeToSpawnAudioOnOtherGuy -= scaledTime;
		if (timeToSpawnAudioOnOtherGuy < 0)
		{
			game->audioClipsOnPlay.push({game->stepSFX, 0, random_range(0.8, 1.2), 1, game->noblePersonTransform.position});
			timeToSpawnAudioOnOtherGuy = 1.0f;
		}
		
		for (s32 outputIndex = 0; outputIndex < soundOutput->sampleCount; ++outputIndex)
		{
			auto & output = soundOutput->samples[outputIndex];
			output = {};
			get_next_sample(game->backgroundAudioClip, output);

			for (s32 i = 0; i < game->audioClipsOnPlay.count; ++i)
			{
				auto & clip = game->audioClipsOnPlay[i];

				StereoSoundSample sample;
				bool32 hasSamplesLeft = get_next_sample(clip, sample);

				f32 attenuation;
				{
					f32 distanceToPlayer = v3_length(clip.position - game->playerCharacterTransform.position);
					distanceToPlayer = f32_clamp(distanceToPlayer, 0, 50);
					attenuation = 1 - (distanceToPlayer / 50);
				}

				output.left += sample.left * attenuation;
				output.right += sample.right * attenuation;


				if (hasSamplesLeft == false)
				{
					array_unordered_remove(game->audioClipsOnPlay, i);
					i -= 1;
				}
			}
		}
	}



	// ------------------------------------------------------------------------

	// bool32 toggle_menu = input_button_went_down(input, InputButton_start)
	// 					|| input_button_went_down(input, InputButton_keyboard_escape);


	auto gui_result = do_gui(game, input);

	gui_end_frame();


	return gui_result;
}

internal Game * game_load_game(MemoryArena & persistentMemory, PlatformFileHandle saveFile)
{
	Game * game = push_memory<Game>(persistentMemory, 1, ALLOC_ZERO_MEMORY);
	*game = {};
	game->persistentMemory = &persistentMemory;


	// Todo(Leo): this is stupidly zero initialized here, before we read settings file, so that we don't override its settings
	game->playerCamera = {};


	// Note(Leo): We currently only have statically allocated stuff (or rather allocated with game),
	// this can be read here, at the top. If we need to allocate some stuff, we need to reconsider.
	read_settings_file(game_get_serialized_objects(*game));

	// ----------------------------------------------------------------------------------

	// Todo(Leo): Think more about implications of storing pointer to persistent memory here
	// Note(Leo): We have not previously used persistent allocation elsewhere than in this load function
	game->assets 			= init_game_assets(&persistentMemory);
	game->collisionSystem 	= init_collision_system(persistentMemory);

	game->gui = {}; // Todo(Leo): remove this we now use ImGui
	// game->menuStateIndex 		= 0;
	// game->menuStates[0].view 	= MenuView_off;

	initialize_physics_world(game->physicsWorld, persistentMemory);

	log_application(1, "Allocations succesful! :)");

	// ----------------------------------------------------------------------------------

	// Skysphere
	game->skybox = graphics_memory_push_model(	platformGraphics,
												assets_get_mesh(game->assets, MeshAssetId_skysphere),
												assets_get_material(game->assets, MaterialAssetId_sky));

	// Characters
	{
		game->playerCarriedEntity.type 			= EntityType_none;
		game->playerCharacterTransform 	= {.position = {10, 0, 5}};

		auto & motor 	= game->playerCharacterMotor;
		motor.transform = &game->playerCharacterTransform;

		{
			using namespace CharacterAnimations;			

			motor.animations[WALK] 		= assets_get_animation(game->assets, AnimationAssetId_character_walk);
			motor.animations[RUN] 		= assets_get_animation(game->assets, AnimationAssetId_character_run);
			motor.animations[IDLE] 		= assets_get_animation(game->assets, AnimationAssetId_character_idle);
			motor.animations[JUMP]		= assets_get_animation(game->assets, AnimationAssetId_character_jump);
			motor.animations[FALL]		= assets_get_animation(game->assets, AnimationAssetId_character_fall);
			motor.animations[CROUCH] 	= assets_get_animation(game->assets, AnimationAssetId_character_crouch);
		}

		game->playerSkeletonAnimator = 
		{
			.skeleton 		= assets_get_skeleton(game->assets, SkeletonAssetId_character),
			.animations 	= motor.animations,
			.weights 		= motor.animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		};

		game->playerSkeletonAnimator.boneBoneSpaceTransforms = push_array<Transform3D>(	persistentMemory,
																							game->playerSkeletonAnimator.skeleton->boneCount,
																							ALLOC_ZERO_MEMORY);
		array_fill_with_value(game->playerSkeletonAnimator.boneBoneSpaceTransforms, identity_transform);

		auto model = graphics_memory_push_model(platformGraphics,
												assets_get_mesh(game->assets, MeshAssetId_character),
												assets_get_material(game->assets, MaterialAssetId_character));
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
			
			game->noblePersonCharacterMotor.animations[WALK] 	= assets_get_animation(game->assets, AnimationAssetId_character_walk);
			game->noblePersonCharacterMotor.animations[RUN] 	= assets_get_animation(game->assets, AnimationAssetId_character_run);
			game->noblePersonCharacterMotor.animations[IDLE]	= assets_get_animation(game->assets, AnimationAssetId_character_idle);
			game->noblePersonCharacterMotor.animations[JUMP] 	= assets_get_animation(game->assets, AnimationAssetId_character_jump);
			game->noblePersonCharacterMotor.animations[FALL] 	= assets_get_animation(game->assets, AnimationAssetId_character_fall);
			game->noblePersonCharacterMotor.animations[CROUCH] = assets_get_animation(game->assets, AnimationAssetId_character_crouch);
		}

		game->noblePersonSkeletonAnimator = 
		{
			.skeleton 		= assets_get_skeleton(game->assets, SkeletonAssetId_character),
			.animations 	= game->noblePersonCharacterMotor.animations,
			.weights 		= game->noblePersonCharacterMotor.animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		};
		game->noblePersonSkeletonAnimator.boneBoneSpaceTransforms = push_array<Transform3D>(	persistentMemory,
																								game->noblePersonSkeletonAnimator.skeleton->boneCount,
																								ALLOC_ZERO_MEMORY);
		array_fill_with_value(game->noblePersonSkeletonAnimator.boneBoneSpaceTransforms, identity_transform);

		auto model = graphics_memory_push_model(platformGraphics,
												assets_get_mesh(game->assets, MeshAssetId_character), 
												assets_get_material(game->assets, MaterialAssetId_character)); 

		game->noblePersonAnimatedRenderer = make_animated_renderer(&game->noblePersonTransform, game->noblePersonSkeletonAnimator.skeleton, model);

	}

	game->worldCamera 				= make_camera(60, 0.1f, 1000.0f);

	// Environment
	{
		/// GROUND AND TERRAIN
		{
			constexpr f32 mapSize 		= 1200;
			constexpr f32 minTerrainElevation = -5;
			constexpr f32 maxTerrainElevation = 100;

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
				TextureAssetData heightmapData = assets_get_texture_data(game->assets, TextureAssetId_heightmap);
				heightmap 				= make_heightmap(&persistentMemory, &heightmapData, heightmapResolution, mapSize, minTerrainElevation, maxTerrainElevation);

				memory_pop_checkpoint(*global_transientMemory, checkpoint);
			}

			s32 terrainCount 			= chunkCountPerDirection * chunkCountPerDirection;
			game->terrainCount 		= terrainCount;
			game->terrainTransforms 	= push_memory<m44>(persistentMemory, terrainCount, ALLOC_GARBAGE);
			game->terrainMeshes 		= push_memory<MeshHandle>(persistentMemory, terrainCount, ALLOC_GARBAGE);
			game->terrainMaterial 		= assets_get_material(game->assets, MaterialAssetId_ground);

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

			game->collisionSystem.terrainCollider 	= heightmap;
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

				seaMeshAsset.indexType = MeshIndexType_uint16;
			}
			mesh_generate_tangents(seaMeshAsset);
			game->seaMesh 		= graphics_memory_push_mesh(platformGraphics, &seaMeshAsset);
			game->seaMaterial 	= assets_get_material(game->assets, MaterialAssetId_sea);
			game->seaTransform = transform_matrix({0,0,0}, identity_quaternion, {mapSize, mapSize, 1});
		}

		/// TOTEMS
		{
			game->totemMesh 	= assets_get_mesh(game->assets, MeshAssetId_totem);
			game->totemMaterial = assets_get_material(game->assets, MaterialAssetId_environment);

			v3 position0 = {0,0,0}; 	position0.z = get_terrain_height(game->collisionSystem, position0.xy);
			v3 position1 = {0,5,0}; 	position1.z = get_terrain_height(game->collisionSystem, position1.xy);

			game->totemTransforms[0] = {position0, identity_quaternion, {1,1,1}};
			game->totemTransforms[1] = {position1, identity_quaternion, {0.5, 0.5, 0.5}};

			BoxCollider totemCollider = {v3{1,1,5}, identity_quaternion, v3{0,0,2}};

			push_static_box_collider(game->collisionSystem, totemCollider, game->totemTransforms[0]);
			push_static_box_collider(game->collisionSystem, totemCollider, game->totemTransforms[1]);
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
				game->raccoonModes[i]					= RaccoonMode_idle;

				game->raccoonTransforms[i] 			= identity_transform;
				game->raccoonTransforms[i].position 	= random_inside_unit_square() * 100 - v3{50, 50, 0};
				game->raccoonTransforms[i].position.z  = get_terrain_height(game->collisionSystem, game->raccoonTransforms[i].position.xy);

				game->raccoonTargetPositions[i] 		= random_inside_unit_square() * 100 - v3{50,50,0};
				game->raccoonTargetPositions[i].z  	= get_terrain_height(game->collisionSystem, game->raccoonTargetPositions[i].xy);

				// ------------------------------------------------------------------------------------------------
		
				// Note Todo(Leo): Super debug, not at all like this
				game->raccoonCharacterMotors[i] = {};
				game->raccoonCharacterMotors[i].transform = &game->raccoonTransforms[i];
				game->raccoonCharacterMotors[i].walkSpeed = 2;
				game->raccoonCharacterMotors[i].runSpeed = 4;

				{
					using namespace CharacterAnimations;

					game->raccoonCharacterMotors[i].animations[WALK] 	= assets_get_animation(game->assets, AnimationAssetId_raccoon_empty);
					game->raccoonCharacterMotors[i].animations[RUN] 	= assets_get_animation(game->assets, AnimationAssetId_raccoon_empty);
					game->raccoonCharacterMotors[i].animations[IDLE]	= assets_get_animation(game->assets, AnimationAssetId_raccoon_empty);
					game->raccoonCharacterMotors[i].animations[JUMP] 	= assets_get_animation(game->assets, AnimationAssetId_raccoon_empty);
					game->raccoonCharacterMotors[i].animations[FALL] 	= assets_get_animation(game->assets, AnimationAssetId_raccoon_empty);
					game->raccoonCharacterMotors[i].animations[CROUCH] 	= assets_get_animation(game->assets, AnimationAssetId_raccoon_empty);
				}
			}

			game->raccoonMesh 		= assets_get_mesh(game->assets, MeshAssetId_raccoon);
			game->raccoonMaterial	= assets_get_material(game->assets, MaterialAssetId_raccoon);
		}

		// /// INVISIBLE TEST COLLIDER, for nostalgia :')
		// {
		// 	Transform3D * transform = game->transforms.push_return_pointer({});
		// 	transform->position = {21, 15};
		// 	transform->position.z = get_terrain_height(game->collisionSystem, {20, 20});

		// 	push_box_collider( 	game->collisionSystem,
		// 						v3{2.0f, 0.05f,5.0f},
		// 						v3{0,0, 2.0f},
		// 						transform);
		// }

		game->monuments = init_monuments(persistentMemory, game->assets, game->collisionSystem);

		// TEST ROBOT
		{
			v3 position 			= {21, 10, 0};
			position.z 				= get_terrain_height(game->collisionSystem, position.xy);
			game->robotTransform 	= {position};

			game->robotMesh 		= assets_get_mesh(game->assets, MeshAssetId_robot);
			game->robotMaterial 	= assets_get_material(game->assets, MaterialAssetId_robot);
		}

		/// SMALL SCENERY OBJECTS
		{			
			game->potMesh 		= assets_get_mesh(game->assets, MeshAssetId_small_pot);
			game->potMaterial 	= assets_get_material(game->assets, MaterialAssetId_environment);

			{
				game->potCapacity 			= 10;
				game->potCount 				= game->potCapacity;
				game->potTransforms 		= push_memory<Transform3D>(persistentMemory, game->potCapacity, ALLOC_GARBAGE);
				game->potWaterLevels 		= push_memory<f32>(persistentMemory, game->potCapacity, ALLOC_GARBAGE);
				game->potCarriedEntities 	= push_memory<EntityReference>(persistentMemory, game->potCapacity, ALLOC_ZERO_MEMORY);

				for(s32 i = 0; i < game->potCapacity; ++i)
				{
					v3 position 			= {15, i * 5.0f, 0};
					position.z 				= get_terrain_height(game->collisionSystem, position.xy);
					game->potTransforms[i]	= { .position = position };
				}
			}

			// ----------------------------------------------------------------	

			auto bigPotMesh = assets_get_mesh(game->assets, MeshAssetId_big_pot);

			s32 bigPotCount = 5;

			game->bigPotMesh 		= assets_get_mesh(game->assets, MeshAssetId_big_pot);
			game->bigPotMaterial 	= assets_get_material(game->assets, MaterialAssetId_environment);
			game->bigPotTransforms 	= push_array<Transform3D>(persistentMemory, bigPotCount, ALLOC_GARBAGE);
			game->bigPotTransforms.count = bigPotCount;

			for (s32 i = 0; i < bigPotCount; ++i)
			{
				v3 position = {13, 2.0f + i * 4.0f}; 		position.z = get_terrain_height(game->collisionSystem, position.xy);

				Transform3D * transform = &game->bigPotTransforms[i];
				*transform 				= identity_transform;
				transform->position 	= position;

				// f32 colliderHeight = 1.16;
				// push_cylinder_collider(game->collisionSystem, 0.6, colliderHeight, v3{0,0,colliderHeight / 2}, transform);
			}


			// ----------------------------------------------------------------	

			/// WATERS
			initialize_waters(game->waters, persistentMemory);


		}

		/// TRAIN
		{
			game->trainMesh 		= assets_get_mesh(game->assets, MeshAssetId_train);
			game->trainMaterial 	= assets_get_material(game->assets, MaterialAssetId_environment);

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

			game->trainMoveState 					= TrainState_wait;

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

		game->metaballMaterial = assets_get_material(game->assets, MaterialAssetId_tree);

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
		game->trees.array = push_array<Tree>(persistentMemory, 200, ALLOC_ZERO_MEMORY);
		for (s32 i = 0; i < game->trees.array.capacity; ++i)
		{
			allocate_tree_memory(persistentMemory, game->trees.array.memory[i]);
		}
		game->trees.selectedIndex = 0;
	}

	{
		v3 boxPosition0 		= {20, 2, get_terrain_height(game->collisionSystem, {20, 2})};
		v3 boxPosition1 		= {30, 5, get_terrain_height(game->collisionSystem, {30, 5})};
		v3 coverPositionOffset 	= {0, -0.5, 0.85};

		game->boxCount = 2;
		game->boxTransforms 			= push_memory<Transform3D>(persistentMemory, game->boxCount, ALLOC_GARBAGE);
		game->boxCoverLocalTransforms 	= push_memory<Transform3D>(persistentMemory, game->boxCount, ALLOC_GARBAGE);
		game->boxStates 				= push_memory<BoxState>(persistentMemory, game->boxCount, ALLOC_ZERO_MEMORY);
		game->boxOpenPercents			= push_memory<f32>(persistentMemory, game->boxCount, ALLOC_GARBAGE);
		game->boxCarriedEntities 		= push_memory<EntityReference>(persistentMemory, game->boxCount, ALLOC_ZERO_MEMORY);

		game->boxTransforms[0] = {boxPosition0};
		game->boxTransforms[1] = {boxPosition1};

		game->boxCoverLocalTransforms[0] = {coverPositionOffset};
		game->boxCoverLocalTransforms[1] = {coverPositionOffset};
	
		game->boxCarriedEntities[0] = {EntityType_tree_3, game_spawn_tree(*game, boxPosition0 + v3{0,0,0.1}, 0, false)};
		game->boxCarriedEntities[1] = {EntityType_tree_3, game_spawn_tree(*game, boxPosition0 + v3{0,0,0.1}, 1, false)};
	}


	// ----------------------------------------------------------------------------------

	initialize_clouds(game->clouds, persistentMemory);

	// ----------------------------------------------------------------------------------

	game->scene.buildingBlocks = push_array<m44>(persistentMemory, 1000, ALLOC_GARBAGE);
	scene_asset_load_2(game->scene);

	game->selectedBuildingBlockIndex = 0;

	// ----------------------------------------------------------------------------------

	game->backgroundAudio 	= assets_get_audio(game->assets, SoundAssetId_background);
	game->stepSFX			= assets_get_audio(game->assets, SoundAssetId_step_1);
	game->stepSFX2			= assets_get_audio(game->assets, SoundAssetId_step_2);
	game->stepSFX3			= assets_get_audio(game->assets, SoundAssetId_birds);

	game->backgroundAudioClip 	= {game->backgroundAudio, 0};

	game->audioClipsOnPlay = push_array<AudioClip>(persistentMemory, 1000, ALLOC_ZERO_MEMORY);

	// ----------------------------------------------------------------------------------

	log_application(0, "Game loaded, ", used_percent(*global_transientMemory) * 100, "% of transient memory used. ",
					reverse_gigabytes(global_transientMemory->used), "/", reverse_gigabytes(global_transientMemory->size), " GB");

	log_application(0, "Game loaded, ", used_percent(persistentMemory) * 100, "% of transient memory used. ",
					reverse_gigabytes(persistentMemory.used), "/", reverse_gigabytes(persistentMemory.size), " GB");

	return game;
}
