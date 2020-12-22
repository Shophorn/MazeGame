/*
Leo Tamminen
shophorn @ internet

Scene description for 3d development game

Todo(Leo):
	crystal trees 

Todo(leo): audio
	clipping, compressor, dynamic gain, soft knee, hard knee
*/

// Todo(Leo): maybe Actor? as opposed to static Scenery
enum EntityType : s32
{ 	
	// Todo(Leo): none should be default value, but really it would be better to have it not
	// contribute to count, basically be below 0 or above count
	EntityType_none,
	
	EntityType_small_pot,
	EntityType_big_pot,
	EntityType_water,
	EntityType_raccoon,
	EntityType_tree_3,

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

bool operator != (EntityReference const & a, EntityReference const & b)
{
	return !(a == b);
}

// Todo(Leo): Maybe try to get rid of this forward declaration
// Should these be global variables or something
struct Game;

#include "game_assets.cpp"

internal s32 					game_spawn_tree(Game & game, v3 position, s32 treeTypeIndex, bool32 pushToPhysics = true);

internal CollisionSystem3D & 	game_get_collision_system(Game * game);
internal GameAssets & 			game_get_assets(Game * game);
internal MemoryArena & 			game_get_persistent_memory(Game * game);

internal v3 * 			entity_get_position(Game * game, EntityReference entity);
internal quaternion * 	entity_get_rotation(Game * game, EntityReference entity);

static void scene_asset_load (Game * game);
static void editor_scene_asset_write(Game const * game);

#include "map.cpp"

#include "game_settings.cpp"

#include "CharacterMotor.cpp"
#include "PlayerController3rdPerson.cpp"
#include "FollowerController.cpp"

enum CameraMode : s32
{ 
	CameraMode_player, 
	CameraMode_editor,

	CameraModeCount
};

enum RaccoonMode : s32
{
	RaccoonMode_idle,
	RaccoonMode_flee,
	RaccoonMode_carried,
};

struct Pots
{
	s32 capacity;
	s32 count;

	Transform3D * 		transforms;
	f32 * 				waterLevels;
	EntityReference * 	carriedEntities;

	MeshAssetId 	mesh;
	MaterialAssetId material;
};

struct Scenery
{
	MeshAssetId 	mesh;
	MaterialAssetId material;
	Array<m44> 		transforms;
};


#include "physics.cpp"

#include "metaballs.cpp"
#include "dynamic_mesh.cpp"
#include "sky_settings.cpp"

// Note(Leo): This maybe seems nice?
#include "game_waters.cpp"

internal Waters & game_get_waters(Game*);

#include "game_leaves.cpp"
#include "game_trees.cpp"
#include "game_raccoons.cpp"

// Todo(Leo): actually other way aroung, but now scene saving button is on blocks editor
#include "game_building_blocks.cpp"

#include "game_ground.cpp"

struct Game
{
	MemoryArena * 	persistentMemory;
	GameAssets 		assets;

	SkySettings 	skySettings;

	// ---------------------------------------

	CollisionSystem3D 	collisionSystem;
	PhysicsWorld 		physicsWorld;

	CameraMode 					cameraMode;
	Camera 						worldCamera;
	GameCameraController 		gameCamera;	
	EditorCameraController 		editorCamera;
	f32 						cameraSelectPercent = 0;
	f32 						cameraTransitionDuration = 0.5;

	// ---------------------------------------

	Player 		player;
	Raccoons 	raccoons;
	Trees 		trees;
	Waters 		waters;
	
	Pots smallPots;
	Pots bigPots;

	Array<Scenery> sceneries;

	Ground ground;

	// ----------------------------------------------------------

	// Random
	Gui 		gui;

	struct
	{
		bool32		drawDebugShadowTexture;
		f32 		timeScale = 1;
		bool32 		guiVisible;
	} editorOptions;

	// ----------------------------------------------

	s64 buildingBlockSceneryIndex;
	s64 buildingPipeSceneryIndex;

	s64 selectedBuildingBlockIndex;
	s64 selectedBuildingPipeIndex;

	// ----------------------------------------------
	
	// AUDIO

	AudioAsset * backgroundAudio;
	AudioAsset * stepSFX;
	AudioAsset * stepSFX2;

	AudioClip 			backgroundAudioClip;
	Array<AudioClip> 	audioClipsOnPlay;

	v3 testRayPosition 	= {-10,0,45};
	v3 testRayDirection = {0,1,0};
	f32 testRayLength 	= 1;
};

#include "scene_data.cpp"


/// SYSTEM THINGS
internal CollisionSystem3D & game_get_collision_system(Game * game) { return game->collisionSystem; }
internal GameAssets & game_get_assets(Game * game) { return game->assets; }
internal MemoryArena & game_get_persistent_memory(Game * game) { return *game->persistentMemory; }

/// ENTITY THINGS
internal Waters & game_get_waters(Game * game) { return game->waters; } 

internal v3 * entity_get_position(Game * game, EntityReference entity)
{
	switch(entity.type)
	{
		case EntityType_raccoon:	return &game->raccoons.transforms[entity.index].position;
		case EntityType_water: 		return &game->waters.positions[entity.index];
		case EntityType_small_pot:	return &game->smallPots.transforms[entity.index].position;
		case EntityType_big_pot:	return &game->bigPots.transforms[entity.index].position;
		case EntityType_tree_3: 	return &game->trees.array[entity.index].position;

		default:
			return nullptr;
	}
}

internal quaternion * entity_get_rotation(Game * game, EntityReference entity)
{
	switch(entity.type)
	{
		case EntityType_raccoon:	return &game->raccoons.transforms[entity.index].rotation;
		case EntityType_water: 		return &game->waters.rotations[entity.index];
		case EntityType_small_pot:	return &game->smallPots.transforms[entity.index].rotation;
		case EntityType_big_pot:	return &game->bigPots.transforms[entity.index].rotation;
		case EntityType_tree_3: 	return &game->trees.array[entity.index].rotation;

		default:
			return nullptr;
	}
}



internal auto game_get_serialized_objects(Game & game)
{
	auto serializedObjects = make_property_list
	(
		serialize_object("sky", game.skySettings),
		serialize_object("tree_0", game.trees.settings[0]),
		serialize_object("tree_1", game.trees.settings[1]),
		serialize_object("player_camera", game.gameCamera)	, 
		serialize_object("editor_camera", game.editorCamera)
	);

	return serializedObjects;
}

internal void game_spawn_water(Game & game, s32 count)
{
	Waters & waters = game.waters;

	v2 center = game.player.characterTransform.position.xy;

	count = f32_min(count, waters.capacity - waters.count);

	for (s32 i = 0; i < count; ++i)
	{
		f32 distance 	= random_range(1, 5);
		f32 angle 		= random_range(0, 2 * π);

		f32 x = f32_cos(angle) * distance;
		f32 y = sine(angle) * distance;

		v3 position = { x + center.x, y + center.y, 0 };
		position.z 	= get_terrain_height(game.collisionSystem, position.xy);

		waters_instantiate(game.waters, position, waters.fullWaterLevel);
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
	v2 center = game.player.characterTransform.position.xy;

	f32 distance 	= random_range(1, 5);
	f32 angle 		= random_range(0, 2 * π);

	f32 x = f32_cos(angle) * distance;
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
#include "game_render.cpp"

// Todo(Leo): add this to syntax higlight, so that 'GAME UPDATE' is different color
/// ------------------ GAME UPDATE -------------------------
internal bool32 game_game_update(Game * 				game,
								PlatformInput * 		input,
								StereoSoundOutput *		soundOutput,
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

	
	f32 scaledTime 		= elapsedTimeSeconds * game->editorOptions.timeScale;
	f32 unscaledTime 	= elapsedTimeSeconds;

	/// ****************************************************************************

	gui_start_frame(game->gui, input, unscaledTime);

	FS_DEBUG_ALWAYS(debug_draw_circle_xy(game->player.characterTransform.position + v3{0,0,2}, 2, colour_bright_red));

	if (input_button_went_down(input, InputButton_keyboard_f1))
	{
		game->cameraMode = 	game->cameraMode == CameraMode_player ?
							CameraMode_editor :
							CameraMode_player;

		platform_window_set(platformWindow,
							PlatformWindowSetting_cursor_hidden_and_locked,
							game->cameraMode == CameraMode_player);
	}

	bool mouseInputAvailable = !ImGui::GetIO().WantCaptureMouse;

	// Game Logic section

	if (mouseInputAvailable)
	{
		switch (game->cameraMode)
		{
			case CameraMode_player:
			{
				player_camera_update(game->gameCamera,	 game->player.characterTransform.position, input, scaledTime);
				game->cameraSelectPercent = f32_max(0, game->cameraSelectPercent - unscaledTime / game->cameraTransitionDuration);
			} break;

			case CameraMode_editor:
			{
				editor_camera_update(game->editorCamera, input, scaledTime);
				game->cameraSelectPercent = f32_min(1, game->cameraSelectPercent + unscaledTime / game->cameraTransitionDuration);
			} break;

			case CameraModeCount:
				Assert(false && "Bad execution path");
				break;
		}
	}

	f32 cameraSelectPercent 		= f32_mathfun_smooth(game->cameraSelectPercent);
	game->worldCamera.position 		= v3_lerp(game->gameCamera.	position, game->editorCamera.position, cameraSelectPercent);
	game->worldCamera.direction 	= v3_lerp(game->gameCamera.	direction, game->editorCamera.direction, cameraSelectPercent);
	game->worldCamera.direction 	= v3_normalize(game->worldCamera.direction);
	game->worldCamera.farClipPlane 	= 1000;

	/// -------------------------------------------------------------------------------------------

	{
		bool playerInputAvailable = game->cameraMode == CameraMode_player;


		CharacterInput playerCharacterMotorInput = {};
		PlayerInput playerInput = {};
		if (playerInputAvailable)
		{
			playerInput 				= player_input_from_platform_input(input);
			playerCharacterMotorInput 	= update_player_input(game->worldCamera, playerInput);
		}
		update_character_motor(game->player.characterMotor, playerCharacterMotorInput, game->collisionSystem, scaledTime, DEBUG_LEVEL_PLAYER);


		// Todo(leo): Should these be handled differently????
		f32 playerPickupDistance 	= 1.0f;
		f32 playerInteractDistance 	= 1.0f;

		/// PICKUP OR DROP
		if (playerInput.pickupOrDrop)
		{
			v3 playerPosition = game->player.characterTransform.position;

			switch(game->player.carriedEntity.type)
			{
				case EntityType_none:
				{
					bool pickup = true;

					/* Todo(Leo): Do this properly, taking into account player facing direction and distances etc. */
					auto check_pickup = [&](s32 count, Transform3D const * transforms, EntityType entityType)
					{
						for (s32 i = 0; i < count; ++i)
						{
							if (v3_length(playerPosition - transforms[i].position) < playerPickupDistance)
							{
								game->player.carriedEntity = {entityType, i};
								pickup = false;
							}
						}
					};


					if (pickup)
					{
						check_pickup(game->smallPots.count, game->smallPots.transforms, EntityType_small_pot);
					}

					if (pickup)
					{
						check_pickup(game->bigPots.count, game->bigPots.transforms, EntityType_big_pot);
					}

					if (pickup)
					{
						for (s32 i = 0; i < game->waters.count; ++i)
						{
							if (v3_length(playerPosition - game->waters.positions[i]) < playerPickupDistance)
							{
								game->player.carriedEntity = {EntityType_water, i};
								pickup = false;
							}
						}


					}

					if (pickup)
					{
						check_pickup(game->raccoons.count, game->raccoons.transforms, EntityType_raccoon);
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
									game->player.carriedEntity = {EntityType_tree_3, i};
								}

								pickup = false;
							}

						}
					}

					// Todo(Leo): maybe this is bad, as in should we only do this if we actually started carrying something
					physics_world_remove_entity(game->physicsWorld, game->player.carriedEntity);

				} break;

				case EntityType_raccoon:
					game->raccoons.transforms[game->player.carriedEntity.index].rotation = quaternion_identity;
					// Todo(Leo): notice no break here, little sketcthy, maybe do something about it, probably in raccoon movement code
					// Todo(Leo): keep raccoon oriented normally, add animation or simulation make them look hangin..

				case EntityType_small_pot:
				case EntityType_big_pot:
				case EntityType_tree_3:
				case EntityType_water:
					physics_world_push_entity(game->physicsWorld, game->player.carriedEntity);
					game->player.carriedEntity = {EntityType_none};
					break;

				default:
					log_debug(FILE_ADDRESS, "cannot pickup or drop that entity: (", game->player.carriedEntity.type, ", #", game->player.carriedEntity.index, ")");
			}
		}

		f32 boxInteractDistance = 0.5f + playerInteractDistance;

		if (playerInput.interact)
		{
			bool32 interact = true;

			// TRY PUT STUFF INTO POT
			bool playerCarriesPottableEntity = 	game->player.carriedEntity.type == EntityType_tree_3
												|| game->player.carriedEntity.type == EntityType_raccoon;

			if (interact && playerCarriesPottableEntity)
			{
				log_debug("Try put something to pot");

				for (s32 i = 0; i < game->smallPots.count; ++i)
				{
					f32 distanceToPot = v3_length(game->smallPots.transforms[i].position - game->player.characterTransform.position);
					if (distanceToPot < playerInteractDistance && game->smallPots.carriedEntities[i].type == EntityType_none)
					{
						log_debug(FILE_ADDRESS, "Stuff put into pot");

						game->smallPots.carriedEntities[i] = game->player.carriedEntity;
						game->player.carriedEntity 	= {EntityType_none};

						interact = false;
					}
				}
			}
			
			// PLANT THE CARRIED TREE
			bool32 playerCarriesTree = game->player.carriedEntity.type == EntityType_tree_3;

			if (interact && playerCarriesTree)
			{
				game->trees.array[game->player.carriedEntity.index].planted = true;
				v3 position = game->trees.array[game->player.carriedEntity.index].position;
				position.z = get_terrain_height(game->collisionSystem, position.xy);
				game->trees.array[game->player.carriedEntity.index].position = position;
				game->player.carriedEntity.index = -1;
				game->player.carriedEntity.type = EntityType_none;			

				interact = false;
			}

		}
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
			// entity_set_position(game->player.carriedEntity, carriedPosition);
			// entity_set_rotation(game->player.carriedEntity, carriedRotation);
			// entity_set_state(game->player.carriedEntity, EntityState_carried_by_player);

			if (carriedEntities[i].type == EntityType_raccoon)
			{
				game->raccoons.transforms[carriedEntities[i].index].position 	= carriedPosition + v3{0,0,0.2};

				v3 right = quaternion_rotate_v3(carriedRotation, v3_right);
				game->raccoons.transforms[carriedEntities[i].index].rotation 	= carriedRotation * quaternion_axis_angle(right, 1.4f);
			}
			else if (carriedEntities[i].type != EntityType_none)
			{
				// Todo(Leo): if we ever crash here start doing checks
				*entity_get_position(game, carriedEntities[i]) = carriedPosition;
				*entity_get_rotation(game, carriedEntities[i]) = carriedRotation;
			}
		}
	};

	update_carried_entities_transforms(1, &game->player.characterTransform, &game->player.carriedEntity, {0, 0.7, 0.7});
	update_carried_entities_transforms(game->smallPots.count, game->smallPots.transforms, game->smallPots.carriedEntities, {0, 0, 0.1});

	update_waters(game->waters, scaledTime);
	update_physics_world(game->physicsWorld, game, scaledTime);

	// -----------------------------------------------------------------------------------------------------------
	
	raccoons_update(game->raccoons, game->smallPots, game, game->player.carriedEntity, scaledTime);

	// -----------------------------------------------------------------------------------------------------------


	/// SUBMIT COLLIDERS
	// Note(Leo): These colliders are used mainly in next game loop, since we update them here after everything has moved
	// Make a proper decision whether or not this is something we need
	// Todo(Leo): most colliders are really static, so this is major stupid
	{
		collision_system_reset_submitted_colliders(game->collisionSystem);

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
		submit_cylinder_colliders(smallPotColliderRadius, smallPotHalfHeight, game->smallPots.count, game->smallPots.transforms);

		f32 bigPotColliderRadius 	= 0.6;
		f32 bigPotColliderHeight 	= 1.16;
		f32 bigPotHalfHeight 		= bigPotColliderHeight / 2;
		submit_cylinder_colliders(bigPotColliderRadius, bigPotHalfHeight, game->bigPots.count, game->bigPots.transforms);


		// NEW TREES 3
		for (auto tree : game->trees.array)
		{
			CylinderCollider collider =
			{
				.radius 	= tree.nodes[tree.branches[0].startNodeIndex].radius,
				.halfHeight = 1.0f,
				.center 	= {0,0,1}
			};

			submit_cylinder_collider(game->collisionSystem, collider, {tree.position, quaternion_identity, {1,1,1}});
		}

		/// BUILDING BLOCKS
		for (s32 i = 0; i < game->sceneries[game->buildingBlockSceneryIndex].transforms.count; ++i)
		{
			submit_box_collider(game->collisionSystem, {{1,1,1}, quaternion_identity, {0,0,0}}, game->sceneries[game->buildingBlockSceneryIndex].transforms[i]);
		}

		// for (s32 i = 0; i < game->buildingPipes.transforms.count; ++i)
		// {
		// 	// submit_cylinder_collider(game->)
		// }


		ImGui::Begin("Test collider");
		{
			ImGui::DragFloat3("corner 0", &game->collisionSystem.testTriangleCollider[0].x);
			ImGui::DragFloat3("corner 1", &game->collisionSystem.testTriangleCollider[1].x);
			ImGui::DragFloat3("corner 2", &game->collisionSystem.testTriangleCollider[2].x);

			ImGui::Spacing();
			ImGui::DragFloat3("ray position", &game->testRayPosition.x, 0.1);
			if (ImGui::DragFloat3("ray direction", &game->testRayDirection.x), 0.1)
			{
				game->testRayDirection = v3_normalize(game->testRayDirection);
			}
			ImGui::DragFloat("ray length", &game->testRayLength, 0.1);
		}
		ImGui::End();

		RaycastResult rayResult;
		bool hit = raycast_3d(&game->collisionSystem, game->testRayPosition, game->testRayDirection, game->testRayLength, &rayResult);


		if (hit)
		{
			FS_DEBUG_ALWAYS(debug_draw_vector(game->testRayPosition, rayResult.hitPosition - game->testRayPosition, colour_bright_red));
			// FS_DEBUG_ALWAYS(debug_draw_line(game->testRayPosition, rayResult.hitPosition, colour_bright_red));
		}
		else
		{
			FS_DEBUG_ALWAYS(debug_draw_vector(game->testRayPosition, game->testRayDirection * game->testRayLength, colour_bright_green));
			// FS_DEBUG_ALWAYS(debug_draw_line(game->testRayPosition, game->testRayPosition + game->testRayDirection * game->testRayLength, colour_bright_green));
		}


		FS_DEBUG_ALWAYS(debug_draw_line(game->collisionSystem.testTriangleCollider[0], game->collisionSystem.testTriangleCollider[1], colour_bright_green));
		FS_DEBUG_ALWAYS(debug_draw_line(game->collisionSystem.testTriangleCollider[1], game->collisionSystem.testTriangleCollider[2], colour_bright_green));
		FS_DEBUG_ALWAYS(debug_draw_line(game->collisionSystem.testTriangleCollider[2], game->collisionSystem.testTriangleCollider[0], colour_bright_green));
	}

	update_skeleton_animator(game->player.skeletonAnimator, scaledTime);
	
	// GetWaterFunc get_water = { game->waters, game->player.carriedEntity.index, game->player.carriedEntity.type == EntityType_water };
	trees_update(game->trees, game, scaledTime);

	// ---------- PROCESS AUDIO -------------------------

	{		
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
					f32 distanceToPlayer = v3_length(clip.position - game->player.characterTransform.position);
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

	if (input_button_went_down(input, InputButton_keyboard_escape))
	{
		game->editorOptions.guiVisible = !game->editorOptions.guiVisible;
	}

	auto gui_result = game->editorOptions.guiVisible ? do_gui(game, input) : true;

	gui_end_frame();

	return gui_result;
}

internal Game * game_load_game(MemoryArena & persistentMemory, PlatformFileHandle saveFile)
{
	Game * game = push_memory<Game>(persistentMemory, 1, ALLOC_ZERO_MEMORY);
	*game = {};
	game->persistentMemory = &persistentMemory;


	// Todo(Leo): this is stupidly zero initialized here, before we read settings file, so that we don't override its settings
	game->gameCamera 	= {};
	game->editorCamera = {};

	// Note(Leo): We currently only have statically allocated stuff (or rather allocated with game),
	// this can be read here, at the top. If we need to allocate some stuff, we need to reconsider.
	read_settings_file(game_get_serialized_objects(*game));

	game->cameraMode = CameraMode_player;
	platform_window_set(platformWindow, PlatformWindowSetting_cursor_hidden_and_locked, true);

	// ----------------------------------------------------------------------------------

	// Todo(Leo): Think more about implications of storing pointer to persistent memory here
	// Note(Leo): We have not previously used persistent allocation elsewhere than in this load function
	game->assets 			= game_assets_initialize(&persistentMemory);
	game->collisionSystem 	= collision_system_initialize(persistentMemory);

	game->ground 			= ground_initialize(persistentMemory, game->assets, game->collisionSystem);

	game->gui = {}; // Todo(Leo): remove this we now use ImGui

	initialize_physics_world(game->physicsWorld, persistentMemory);

	log_application(1, "Allocations succesful! :)");

	// ----------------------------------------------------------------------------------

	// Characters
	{
		game->player.carriedEntity.type 			= EntityType_none;
		game->player.characterTransform 	= {.position = {10, 0, 5}};

		auto & motor 	= game->player.characterMotor;
		motor.transform = &game->player.characterTransform;

		{
			motor.animations[CharacterAnimation_walk] 		= assets_get_animation(game->assets, AnimationAssetId_character_walk);
			motor.animations[CharacterAnimation_run] 		= assets_get_animation(game->assets, AnimationAssetId_character_run);
			motor.animations[CharacterAnimation_idle] 		= assets_get_animation(game->assets, AnimationAssetId_character_idle);
			motor.animations[CharacterAnimation_jump]		= assets_get_animation(game->assets, AnimationAssetId_character_jump);
			motor.animations[CharacterAnimation_fall]		= assets_get_animation(game->assets, AnimationAssetId_character_fall);
			motor.animations[CharacterAnimation_crouch] 	= assets_get_animation(game->assets, AnimationAssetId_character_crouch);
			motor.animations[CharacterAnimation_climb] 		= assets_get_animation(game->assets, AnimationAssetId_character_climb);
		}

		game->player.skeletonAnimator = 
		{
			.skeleton 		= assets_get_skeleton(game->assets, SkeletonAssetId_character),
			.animations 	= motor.animations,
			.weights 		= motor.animationWeights,
			.animationCount = CharacterAnimationCount
		};

		game->player.skeletonAnimator.boneBoneSpaceTransforms = push_array<Transform3D>(	persistentMemory,
																							game->player.skeletonAnimator.skeleton->boneCount,
																							ALLOC_ZERO_MEMORY);
		array_fill_with_value(game->player.skeletonAnimator.boneBoneSpaceTransforms, identity_transform);
	}

	game->worldCamera 				= make_camera(60, 0.1f, 1000.0f);

	// Environment
	{

		game->sceneries = push_array<Scenery>(persistentMemory, 100, ALLOC_GARBAGE);

		/// TOTEMS
		{
			Scenery & totems = game->sceneries.push({});

			s64 totemCount = 2;
			totems =
			{
				.mesh 		= MeshAssetId_totem,
				.material 	= MaterialAssetId_building_block,
				.transforms = push_array_2<m44>(persistentMemory, totemCount, totemCount, ALLOC_GARBAGE)
			};

			v3 position0 = {0,0,0}; 	position0.z = get_terrain_height(game->collisionSystem, position0.xy);
			v3 position1 = {0,5,0}; 	position1.z = get_terrain_height(game->collisionSystem, position1.xy);

			totems.transforms[0] = transform_matrix({position0, quaternion_identity, {1,1,1}});
			totems.transforms[1] = transform_matrix({position1, quaternion_identity, {0.5, 0.5, 0.5}});

			BoxCollider totemCollider = {v3{1,1,5}, quaternion_identity, v3{0,0,2}};

			push_static_box_collider(game->collisionSystem, totemCollider, totems.transforms[0]);
			push_static_box_collider(game->collisionSystem, totemCollider, totems.transforms[1]);
		}

		/// RACCOONS
		game->raccoons = raccoons_initialize(persistentMemory, game);

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

		// TEST ROBOT
		{
			Scenery & robots = game->sceneries.push({});

			robots =
			{
				.mesh 		= MeshAssetId_robot,
				.material 	= MaterialAssetId_robot,
				.transforms = push_array_2<m44>(persistentMemory, 1, 1, ALLOC_GARBAGE)
			};

			v3 position 				= {21, 10, 0};
			position.z 					= get_terrain_height(game->collisionSystem, position.xy);
			robots.transforms[0] 	= translation_matrix(position);
		}

		/// SMALL SCENERY OBJECTS
		{	
			{
				game->smallPots = {};

				game->smallPots.capacity 			= 10;
				game->smallPots.count 				= game->smallPots.capacity;
				game->smallPots.transforms 			= push_memory<Transform3D>(persistentMemory, game->smallPots.capacity, ALLOC_GARBAGE);
				game->smallPots.waterLevels 		= push_memory<f32>(persistentMemory, game->smallPots.capacity, ALLOC_GARBAGE);
				game->smallPots.carriedEntities 	= push_memory<EntityReference>(persistentMemory, game->smallPots.capacity, ALLOC_ZERO_MEMORY);

				for(s32 i = 0; i < game->smallPots.capacity; ++i)
				{
					v3 position 			= {15, i * 5.0f, 0};
					position.z 				= get_terrain_height(game->collisionSystem, position.xy);
					game->smallPots.transforms[i]	= { .position = position };
				}

				game->smallPots.mesh 		= MeshAssetId_small_pot;
				game->smallPots.material 	= MaterialAssetId_environment;
			}

			// ----------------------------------------------------------------	

			{
				game->bigPots = {};

				game->bigPots.capacity 			= 5;
				game->bigPots.count 			= game->bigPots.capacity;
				game->bigPots.transforms 		= push_memory<Transform3D>(persistentMemory, game->bigPots.capacity, ALLOC_GARBAGE);
				game->bigPots.waterLevels 		= push_memory<f32>(persistentMemory, game->bigPots.capacity, ALLOC_GARBAGE);
				game->bigPots.carriedEntities 	= push_memory<EntityReference>(persistentMemory, game->bigPots.capacity, ALLOC_ZERO_MEMORY);

				for(s32 i = 0; i < game->bigPots.capacity; ++i)
				{
					v3 position 				= {13, 2.0f + i * 4.0f}; 	
					position.z 					= get_terrain_height(game->collisionSystem, position.xy);
					game->bigPots.transforms[i]	= { .position = position };
				}

				game->bigPots.mesh 		= MeshAssetId_big_pot;
				game->bigPots.material 	= MaterialAssetId_environment;
			}

			// ----------------------------------------------------------------	

			/// WATERS
			initialize_waters(game->waters, persistentMemory);


		}
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
	// ----------------------------------------------------------------------------------

	Scenery & buildingBlocks 	= game->sceneries.push({});
	Scenery & buildingPipes 	= game->sceneries.push({});


	// Todo(Leo): use mapped array
	game->buildingBlockSceneryIndex = array_get_index_of(game->sceneries, buildingBlocks);
	game->buildingPipeSceneryIndex 	= array_get_index_of(game->sceneries, buildingPipes);

	buildingBlocks.transforms 	= push_array<m44>(persistentMemory, 1000, ALLOC_GARBAGE);
	buildingPipes.transforms 	= push_array<m44>(persistentMemory, 1000, ALLOC_GARBAGE);
	scene_asset_load(game);

	buildingBlocks.mesh 		= MeshAssetId_default_cube;
	buildingBlocks.material 	= MaterialAssetId_building_block;

	buildingPipes.mesh 			= MeshAssetId_default_cylinder;
	buildingPipes.material 		= MaterialAssetId_building_block;

	game->selectedBuildingBlockIndex = 0;
	game->selectedBuildingPipeIndex = 0;

	// ----------------------------------------------------------------------------------

	{
		Scenery & castle = game->sceneries.push({});
		castle = 
		{
			.mesh 		= MeshAssetId_castle_main,
			.material 	= MaterialAssetId_building_block,
			.transforms = push_array_2<m44>(persistentMemory, 1, 1, ALLOC_GARBAGE)
		};

		castle.transforms[0] = translation_matrix({70, 70, 20});

		auto castleMesh 	= assets_get_mesh_asset_data(game->assets, MeshAssetId_castle_main, *global_transientMemory);
		s64 triangleCount 	= castleMesh.indexCount / 3;

		game->collisionSystem.triangleColliders = push_array<TriangleCollider>(persistentMemory, triangleCount, ALLOC_GARBAGE);

		for (s64 i = 0; i < triangleCount; ++i)
		{
			s64 t0 = castleMesh.indices[i * 3 + 0];
			s64 t1 = castleMesh.indices[i * 3 + 1];
			s64 t2 = castleMesh.indices[i * 3 + 2];

			TriangleCollider collider = {};
			collider.vertices[0] = multiply_point(castle.transforms[0], castleMesh.vertices[t0].position);
			collider.vertices[1] = multiply_point(castle.transforms[0], castleMesh.vertices[t1].position);
			collider.vertices[2] = multiply_point(castle.transforms[0], castleMesh.vertices[t2].position);

			game->collisionSystem.triangleColliders.push(collider);
		}

	}
	// ----------------------------------------------------------------------------------

	game->backgroundAudio 	= assets_get_audio(game->assets, SoundAssetId_background);
	game->stepSFX			= assets_get_audio(game->assets, SoundAssetId_step_1);
	game->stepSFX2			= assets_get_audio(game->assets, SoundAssetId_step_2);
	// game->stepSFX3			= assets_get_audio(game->assets, SoundAssetId_birds);

	game->backgroundAudioClip 	= {game->backgroundAudio, 0};

	game->audioClipsOnPlay = push_array<AudioClip>(persistentMemory, 1000, ALLOC_ZERO_MEMORY);

	// ----------------------------------------------------------------------------------

	log_application(0, "Game loaded, ", used_percent(*global_transientMemory) * 100, "% of transient memory used. ",
					reverse_gigabytes(global_transientMemory->used), "/", reverse_gigabytes(global_transientMemory->size), " GB");

	log_application(0, "Game loaded, ", used_percent(persistentMemory) * 100, "% of transient memory used. ",
					reverse_gigabytes(persistentMemory.used), "/", reverse_gigabytes(persistentMemory.size), " GB");

	return game;
}
