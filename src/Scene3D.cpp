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

#include "metaballs.cpp"

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
	CARRY_RACCOON,
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

struct FallingObject
{
	s32 type;
	s32 index;
	f32 fallSpeed;
};

struct Scene3d
{
	// Todo(Leo): Remove these "component" arrays, they are stupidly generic solution, that hide away actual data location, at least the way they are now used
	Array<Transform3D> 			transforms;
	Array<Renderer> 			renderers;

	CollisionSystem3D 			collisionSystem;

	// ---------------------------------------

	Transform3D 		playerCharacterTransform;
	CharacterMotor 		playerCharacterMotor;
	SkeletonAnimator 	playerSkeletonAnimator;
	AnimatedRenderer 	playerAnimaterRenderer;

	PlayerInputState	playerInputState;

	Camera 						worldCamera;
	PlayerCameraController 		playerCamera;
	FreeCameraController		freeCamera;
	
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

	/// TREES ------------------------------------
		s32 			lSystemCapacity;
		s32 			lSystemCount;
		TimedLSystem * 	lSystems;
		Leaves * 		lSystemLeavess;
		s32 * 			lSystemsPotIndices;

		// MaterialHandle 	lSystemTreeMaterial;
		MaterialHandle treeMaterials[TREE_TYPE_COUNT];

		MaterialHandle crystalTreeMaterials	[4];
	// ------------------------------------------------------

	Monuments monuments;

	// ------------------------------------------------------

	s32 				raccoonCount;
	s32 *				raccoonModes;
	Transform3D * 		raccoonTransforms;
	v3 *				raccoonTargetPositions;
	CharacterMotor * 	raccoonCharacterMotors;

	MeshHandle 		raccoonMesh;
	MaterialHandle 	raccoonMaterial;

	Animation 		raccoonEmptyAnimation;

	// ------------------------------------------------------

	s32 playerCarryState;
	s32 carriedItemIndex;

	s32 			fallingObjectCapacity;
	s32 			fallingObjectCount;
	FallingObject * fallingObjects;

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

	// Data
	Animation characterAnimations [CharacterAnimations::ANIMATION_COUNT];

	// Random
	ModelHandle skybox;
	bool32 		getSkyColorFromTreeDistance;
	f32 		skyColorSelection;
	f32 		sunHeightAngle;
	f32 		sunOrbitAngle;

	Gui 		gui;
	s32 		cameraMode;
	bool32		drawDebugShadowTexture;


	s32 menuView;

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
	s32 potsLocation;
};

template <typename T>
internal inline void file_write_struct(PlatformFileHandle file, T * value)
{
	platformApi->write_file(file, sizeof(T), value);
}

template <typename T>
internal inline void file_write_memory(PlatformFileHandle file, s32 count, T * memory)
{
	platformApi->write_file(file, sizeof(T) * count, memory);
}

template <typename T>
internal inline void file_read_struct(PlatformFileHandle file, T * value)
{
	platformApi->read_file(file, sizeof(T), value);
}

template <typename T>
internal inline void file_read_memory(PlatformFileHandle file, s32 count, T * memory)
{
	platformApi->read_file(file, sizeof(T) * count, memory);
}

internal void write_save_file(Scene3d * scene)
{
	auto file = platformApi->open_file("save_game.fssave", FILE_MODE_WRITE);

	// Note(Leo): as we go, set values to this struct, and write this to file last.
	Scene3dSaveLoad save;
	platformApi->set_file_position(file, sizeof(save));

	save.playerLocation = platformApi->get_file_position(file);
	platformApi->write_file(file, sizeof(Transform3D), &scene->playerCharacterTransform);

	save.watersLocation = platformApi->get_file_position(file);
	file_write_struct(file, &scene->waters.capacity);
	file_write_struct(file,	&scene->waters.count);
	file_write_memory(file, scene->waters.count, scene->waters.transforms);
	file_write_memory(file, scene->waters.count, scene->waters.levels);

	save.treesLocation = platformApi->get_file_position(file);
	file_write_struct(file, &scene->lSystemCapacity);
	file_write_struct(file, &scene->lSystemCount);
	for (s32 i = 0; i < scene->lSystemCount; ++i)
	{
		file_write_struct(file, &scene->lSystems[i].wordCapacity);
		file_write_struct(file, &scene->lSystems[i].wordCount);

		file_write_memory(file, scene->lSystems[i].wordCount, scene->lSystems[i].aWord);

		file_write_struct(file, &scene->lSystems[i].timeSinceLastUpdate);
		file_write_struct(file, &scene->lSystems[i].position);
		file_write_struct(file, &scene->lSystems[i].rotation);
		file_write_struct(file, &scene->lSystems[i].type);
		file_write_struct(file, &scene->lSystems[i].totalAge);
		file_write_struct(file, &scene->lSystems[i].maxAge);
	}
	file_write_memory(file, scene->lSystemCount, scene->lSystemsPotIndices);

	save.potsLocation = platformApi->get_file_position(file);
	file_write_struct(file, &scene->potCapacity);
	file_write_struct(file, &scene->potCount);
	file_write_memory(file, scene->potCount, scene->potTransforms);
	file_write_memory(file, scene->potCount, scene->potWaterLevels);

	platformApi->set_file_position(file, 0);
	file_write_memory(file, 1, &save);

	platformApi->close_file(file);
}

#include "scene3d_gui.cpp"

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

	// auto snap_on_ground = [&collisionSystem = scene->collisionSystem](v3 position) -> v3
	// {
	// 	v3 result = {position.x, position.y, get_terrain_height(collisionSystem, position.xy)};
	// 	return result;
	// };

	/// Todo(Leo): static may cause probblems
	local_persist SnapOnGround snap_on_ground = {scene->collisionSystem};

	/// ****************************************************************************
	/// TIME

	f32 const timeScales [scene->timeScaleCount] { 1.0f, 0.1f, 0.5f }; 
	f32 scaledTime 		= time.elapsedTime * timeScales[scene->timeScaleIndex];
	f32 unscaledTime 	= time.elapsedTime;

	/// ****************************************************************************

	gui_start_frame(scene->gui, input, unscaledTime);

	/* Sadly, we need to draw skybox before game logic, because otherwise
	debug lines would be hidden. This can be fixed though, just make debug pipeline similar to shadows. */ 
	platformApi->draw_model(platformGraphics, scene->skybox, identity_m44, false, nullptr, 0);

	// Game Logic section
	if(scene->cameraMode == CAMERA_MODE_PLAYER)
	{
		CharacterInput playerCharacterMotorInput = {};

		if (scene->menuView == MENU_OFF)
		{
			if (is_clicked(input.down))
			{
				scene->nobleWanderTargetPosition 	= scene->playerCharacterTransform.position;
				scene->nobleWanderWaitTimer 		= 0;
				scene->nobleWanderIsWaiting 		= false;
			}

			playerCharacterMotorInput = update_player_input(scene->playerInputState, scene->worldCamera, input);
		}

		update_character_motor(scene->playerCharacterMotor, playerCharacterMotorInput, scene->collisionSystem, scaledTime, DEBUG_PLAYER);

		update_camera_controller(&scene->playerCamera, scene->playerCharacterTransform.position, input, scaledTime);

		scene->worldCamera.position = scene->playerCamera.position;
		scene->worldCamera.direction = scene->playerCamera.direction;

		scene->worldCamera.farClipPlane = 1000;

		/// ---------------------------------------------------------------------------------------------------

		debug_draw_circle_xy(scene->nobleWanderTargetPosition + v3{0,0,0.5}, 1.0, colour_bright_green, DEBUG_NPC);
		debug_draw_circle_xy(scene->nobleWanderTargetPosition + v3{0,0,0.5}, 0.9, colour_bright_green, DEBUG_NPC);

	}
	else
	{
		m44 cameraMatrix = update_free_camera(scene->freeCamera, input, unscaledTime);

		scene->worldCamera.position = scene->freeCamera.position;
		scene->worldCamera.direction = scene->freeCamera.direction;

		/// Document(Leo): Teleport player
		if (scene->menuView == MENU_OFF && is_clicked(input.A))
		{
			scene->menuView = MENU_CONFIRM_TELEPORT;
			gui_ignore_input();
			gui_reset_selection();
		}

		scene->worldCamera.farClipPlane = 2000;
	}	

	/// *******************************************************************************************

	update_camera_system(&scene->worldCamera, platformGraphics, platformWindow);

	{
		v3 lightDirection = {0,0,1};
		lightDirection = rotate_v3(axis_angle_quaternion(right_v3, scene->sunHeightAngle * π), lightDirection);
		lightDirection = rotate_v3(axis_angle_quaternion(up_v3, scene->sunOrbitAngle * 2 * π), lightDirection);
		lightDirection = normalize_v3(-lightDirection);

		Light light = { .direction 	= lightDirection, //normalize_v3({-1, 1.2, -8}), 
						.color 		= v3{0.95, 0.95, 0.9}};
		v3 ambient 	= {0.05, 0.05, 0.3};


		if (scene->getSkyColorFromTreeDistance)
		{
			float playerDistanceFromClosestTree = highest_f32;
			for (s32 i = 0; i < scene->lSystemCount; ++i)
			{
				if (scene->lSystems[i].totalAge < 0.0001f)
				{
					continue;
				}

				constexpr f32 treeRadiusRatio = 0.2;
				f32 treeRadius = scene->lSystems[i].totalAge * treeRadiusRatio;

				debug_draw_circle_xy(scene->lSystems[i].position + v3{0,0,0.2}, treeRadius, colour_bright_green, DEBUG_NPC);

				f32 distanceFromTree 			= magnitude_v3(scene->playerCharacterTransform.position - scene->lSystems[i].position) - treeRadius;
				playerDistanceFromClosestTree 	= min_f32(playerDistanceFromClosestTree, distanceFromTree);
			}

			constexpr f32 minPlayerDistance = 0;
			constexpr f32 maxPlayerDistance = 5;

			// f32 playerDistance 				= magnitude_v2(scene->playerCharacterTransform->position.xy);
			light.skyColorSelection 		= clamp_f32((playerDistanceFromClosestTree - minPlayerDistance) / (maxPlayerDistance - minPlayerDistance), 0, 1);
		}
		else
		{
			light.skyColorSelection = scene->skyColorSelection;
		}


		platformApi->update_lighting(platformGraphics, &light, &scene->worldCamera, ambient);
	}

	/// *******************************************************************************************
	

	/// SPAWN WATER
	bool playerInputAvailable = scene->cameraMode == CAMERA_MODE_PLAYER
								&& scene->menuView == MENU_OFF;

	if (playerInputAvailable && is_clicked(input.Y))
	{
		Waters & waters = scene->waters;

		v2 center = scene->playerCharacterTransform.position.xy;

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
	if (playerInputAvailable && is_clicked(input.A))
	{
		auto push_falling_object = [fallingObjects 			= scene->fallingObjects,
									fallingObjectCapacity 	= scene->fallingObjectCapacity,
									&fallingObjectCount 	= scene->fallingObjectCount]
									(s32 type, s32 index)
		{
			Assert(fallingObjectCount < fallingObjectCapacity);

			fallingObjects[fallingObjectCount] 	= {type, index, 0};
			fallingObjectCount 					+= 1;
		};

		v3 playerPosition = scene->playerCharacterTransform.position;
		f32 grabDistance = 1.0f;

		switch(scene->playerCarryState)
		{
			case CARRY_NONE: {
				/* Todo(Leo): Do this properly, taking into account player facing direction and distances etc. */
				auto check_pickup = [	playerPosition 		= scene->playerCharacterTransform.position,
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

				check_pickup(scene->potCount, scene->potTransforms, CARRY_POT);
				check_pickup(scene->waters.count, scene->waters.transforms, CARRY_WATER);
				check_pickup(scene->raccoonCount, scene->raccoonTransforms, CARRY_RACCOON);

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

					f32 distanceToLSystemTree = magnitude_v3(lSystem.position - scene->playerCharacterTransform.position);
					if(distanceToLSystemTree < grabDistance)
					{
						scene->playerCarryState = CARRY_TREE;
						scene->carriedItemIndex = i;
					}
				}

			} break;

			case CARRY_POT:
				push_falling_object(CARRY_POT, scene->carriedItemIndex);
				scene->playerCarryState = CARRY_NONE;

				break;
	
			case CARRY_WATER:
			{
				Transform3D & waterTransform = scene->waters.transforms[scene->carriedItemIndex];

				push_falling_object(CARRY_WATER, scene->carriedItemIndex);
				scene->playerCarryState = CARRY_NONE;


				constexpr f32 waterSnapDistance 	= 0.5f;
				constexpr f32 smallPotMaxWaterLevel = 1.0f;

				f32 droppedWaterLevel = scene->waters.levels[scene->carriedItemIndex];

				for (s32 i = 0; i < scene->potCount; ++i)
				{
					f32 distance = magnitude_v3(scene->potTransforms[i].position - waterTransform.position);
					if (distance < waterSnapDistance)
					{
						scene->potWaterLevels[i] += droppedWaterLevel;
						scene->potWaterLevels[i] = min_f32(scene->potWaterLevels[i] + droppedWaterLevel, smallPotMaxWaterLevel);

						scene->waters.count -= 1;
						scene->waters.transforms[scene->carriedItemIndex] 	= scene->waters.transforms[scene->waters.count];
						scene->waters.levels[scene->carriedItemIndex] 		= scene->waters.levels[scene->waters.count];

						break;	
					}
				}


			} break;

			case CARRY_TREE:
			{
				push_falling_object(CARRY_TREE, scene->carriedItemIndex);
				scene->playerCarryState = CARRY_NONE;
				
				constexpr f32 treeSnapDistance = 0.5f;

				for (s32 potIndex = 0; potIndex < scene->potCount; ++potIndex)
				{
					f32 distance = magnitude_v3(scene->potTransforms[potIndex].position - scene->lSystems[scene->carriedItemIndex].position);
					if (distance < treeSnapDistance)
					{
						scene->lSystemsPotIndices[scene->carriedItemIndex] = potIndex;
					}
				}

			} break;

			case CARRY_RACCOON:
			{
				scene->raccoonTransforms[scene->carriedItemIndex].rotation = identity_quaternion;
				push_falling_object(CARRY_RACCOON, scene->carriedItemIndex);
				scene->playerCarryState = CARRY_NONE;

			} break;
		}
	} // endif input


	v3 carriedPosition 			= multiply_point(transform_matrix(scene->playerCharacterTransform), {0, 0.7, 0.7});
	quaternion carriedRotation 	= scene->playerCharacterTransform.rotation;
	switch(scene->playerCarryState)
	{
		case CARRY_POT:
			scene->potTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->potTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_WATER:
			scene->waters.transforms[scene->carriedItemIndex].position = carriedPosition;
			scene->waters.transforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_TREE:
			scene->lSystems[scene->carriedItemIndex].position = carriedPosition;
			scene->lSystems[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_RACCOON:
		{
			scene->raccoonTransforms[scene->carriedItemIndex].position 	= carriedPosition + v3{0,0,0.2};

			v3 right = rotate_v3(carriedRotation, right_v3);
			scene->raccoonTransforms[scene->carriedItemIndex].rotation 	= carriedRotation * axis_angle_quaternion(right, 1.4f);


		} break;

		case CARRY_NONE:
			break;

		default:
			Assert(false && "That cannot be carried!");
			break;
	}

	/// UPDATE TREES IN POTS POSITIONS
	for (s32 i = 0; i < scene->lSystemCount; ++i)
	{
		if (scene->lSystemsPotIndices[i] > 0)
		{
			s32 potIndex = scene->lSystemsPotIndices[i];
			scene->lSystems[i].position = multiply_point(transform_matrix(scene->potTransforms[potIndex]), v3{0,0,0.25});
			scene->lSystems[i].rotation = scene->potTransforms[potIndex].rotation;
		}
	}

	// UPDATE falling objects
	{
		for (s32 i = 0; i < scene->fallingObjectCount; ++i)
		{
			s32 index 		= scene->fallingObjects[i].index;
			f32 & fallSpeed = scene->fallingObjects[i].fallSpeed;

			v3 * position;

			switch (scene->fallingObjects[i].type)
			{
				case CARRY_RACCOON:	position = &scene->raccoonTransforms[index].position; 	break;
				case CARRY_WATER: position = &scene->waters.transforms[index].position; 	break;
				case CARRY_TREE: position =	&scene->lSystems[index].position;				break;
				case CARRY_POT:	position = &scene->potTransforms[index].position;			break;
			}
		
			f32 targetHeight 	= get_terrain_height(scene->collisionSystem, position->xy);

			fallSpeed 			+= scaledTime * physics_gravity_acceleration;
			position->z 		+= scaledTime * fallSpeed;

			if (position->z < targetHeight)
			{
				position->z = targetHeight;

				/// POP falling object
				scene->fallingObjects[i] = scene->fallingObjects[scene->fallingObjectCount];
				scene->fallingObjectCount -= 1;
				i--;
			}
		}
	}
	
	// -----------------------------------------------------------------------------------------------------------
	/// TRAIN
	{
		v3 trainWayPoints [] = 
		{
			scene->trainStopPosition, 
			scene->trainFarPositionA,
			scene->trainStopPosition, 
			scene->trainFarPositionB,
		};

		if (scene->trainMoveState == TRAIN_MOVE)
		{

			v3 trainMoveVector 	= scene->trainCurrentTargetPosition - scene->trainTransform.position;
			f32 directionDot 	= dot_v3(trainMoveVector, scene->trainCurrentDirection);
			f32 moveDistance	= magnitude_v3(trainMoveVector);

			if (moveDistance > scene->trainBrakeBeforeStationDistance)
			{
				scene->trainCurrentSpeed += scaledTime * scene->trainAcceleration;
				scene->trainCurrentSpeed = min_f32(scene->trainCurrentSpeed, scene->trainFullSpeed);
			}
			else
			{
				scene->trainCurrentSpeed -= scaledTime * scene->trainAcceleration;
				scene->trainCurrentSpeed = max_f32(scene->trainCurrentSpeed, scene->trainStationMinSpeed);
			}

			if (directionDot > 0)
			{
				scene->trainTransform.position += scene->trainCurrentDirection * scene->trainCurrentSpeed * scaledTime;
			}
			else
			{
				scene->trainMoveState 		= TRAIN_WAIT;
				scene->trainCurrentWaitTime = 0;
			}

		}
		else
		{
			scene->trainCurrentWaitTime += scaledTime;
			if (scene->trainCurrentWaitTime > scene->trainWaitTimeOnStop)
			{
				scene->trainMoveState 		= TRAIN_MOVE;
				scene->trainCurrentSpeed 	= 0;

				v3 start 	= trainWayPoints[scene->trainWayPointIndex];
				v3 end 		= trainWayPoints[(scene->trainWayPointIndex + 1) % array_count(trainWayPoints)];

				scene->trainWayPointIndex += 1;
				scene->trainWayPointIndex %= array_count(trainWayPoints);

				scene->trainCurrentTargetPosition 	= end;
				scene->trainCurrentDirection 		= normalize_v3(end - start);
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------
	/// NOBLE PERSON CHARACTER
	{
		CharacterInput nobleCharacterMotorInput = {};

		switch(scene->noblePersonMode)
		{
			case NOBLE_WANDER_AROUND:
			{
				if (scene->nobleWanderIsWaiting)
				{
					scene->nobleWanderWaitTimer -= scaledTime;
					if (scene->nobleWanderWaitTimer < 0)
					{
						scene->nobleWanderTargetPosition = { random_range(-99, 99), random_range(-99, 99)};
						scene->nobleWanderIsWaiting = false;
					}


					m44 gizmoTransform = make_transform_matrix(	scene->noblePersonTransform.position + up_v3 * scene->noblePersonTransform.scale.z * 2.0f, 
																scene->noblePersonTransform.rotation,
																scene->nobleWanderWaitTimer);
					debug_draw_diamond_xz(gizmoTransform, colour_muted_red, DEBUG_NPC);
				}
				
				f32 distance = magnitude_v2(scene->noblePersonTransform.position.xy - scene->nobleWanderTargetPosition.xy);
				if (distance < 1.0f && scene->nobleWanderIsWaiting == false)
				{
					scene->nobleWanderWaitTimer = 10;
					scene->nobleWanderIsWaiting = true;
				}


				v3 input 			= {};
				input.xy	 		= scene->nobleWanderTargetPosition.xy - scene->noblePersonTransform.position.xy;
				f32 inputMagnitude 	= magnitude_v3(input);
				input 				= input / inputMagnitude;
				inputMagnitude 		= clamp_f32(inputMagnitude, 0.0f, 1.0f);
				input 				= input * inputMagnitude;

				nobleCharacterMotorInput = {input, false, false};

			} break;
		}
	
		update_character_motor(	scene->noblePersonCharacterMotor,
								nobleCharacterMotorInput,
								scene->collisionSystem,
								scaledTime,
								DEBUG_NPC);
	}

	// -----------------------------------------------------------------------------------------------------------
	/// Update RACCOONS
	{

		for(s32 i = 0; i < scene->raccoonCount; ++i)
		{
			CharacterInput raccoonCharacterMotorInput = {};
	
			v3 toTarget 			= scene->raccoonTargetPositions[i] - scene->raccoonTransforms[i].position;
			f32 distanceToTarget 	= magnitude_v3(toTarget);

			v3 input = {};

			if (distanceToTarget < 1.0f)
			{
				scene->raccoonTargetPositions[i] 	= snap_on_ground(random_inside_unit_square() * 100 - v3{50,50,0});
				toTarget 							= scene->raccoonTargetPositions[i] - scene->raccoonTransforms[i].position;
			}
			else
			{
				input.xy	 		= scene->raccoonTargetPositions[i].xy - scene->raccoonTransforms[i].position.xy;
				f32 inputMagnitude 	= magnitude_v3(input);
				input 				= input / inputMagnitude;
				inputMagnitude 		= clamp_f32(inputMagnitude, 0.0f, 1.0f);
				input 				= input * inputMagnitude;
			}

			raccoonCharacterMotorInput = {input, false, false};

			bool isCarried = (scene->playerCarryState == CARRY_RACCOON) && (scene->carriedItemIndex == i);

			if (isCarried == false)
			{
				update_character_motor( scene->raccoonCharacterMotors[i],
										raccoonCharacterMotorInput,
										scene->collisionSystem,
										scaledTime,
										DEBUG_NPC);
			}

			// debug_draw_circle_xy(snap_on_ground(scene->raccoonTargetPositions[i].xy) + v3{0,0,1}, 1, colour_bright_red, DEBUG_ALWAYS);
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
		submit_cylinder_colliders(smallPotColliderRadius, smallPotHalfHeight, scene->potCount, scene->potTransforms);

		constexpr f32 baseRadius = 0.12;
		constexpr f32 baseHeight = 2;
	}


	update_skeleton_animator(scene->playerSkeletonAnimator, scaledTime);
	update_skeleton_animator(scene->noblePersonSkeletonAnimator, scaledTime);
	
	
	/// DRAW POTS
	{
		// Todo(Leo): store these as matrices, we can easily retrieve position (that is needed somwhere) from that too.
		m44 * potTransformMatrices = push_memory<m44>(*global_transientMemory, scene->potCount, ALLOC_NO_CLEAR);
		for(s32 i = 0; i < scene->potCount; ++i)
		{
			potTransformMatrices[i] = transform_matrix(scene->potTransforms[i]);
		}
		platformApi->draw_meshes(platformGraphics, scene->potCount, potTransformMatrices, scene->potMesh, scene->potMaterial);
	}

	/// DRAW STATIC SCENERY
	{
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

		debug_draw_circle_xy(scene->playerCharacterTransform.position + v3{0,0,0.7}, 0.25f, colour_bright_green, DEBUG_PLAYER);
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

	if (scene->drawMCStuff)
	{
		v3 position = multiply_point(scene->metaballTransform, {0,0,0});
		// debug_draw_circle_xy(position, 4, {1,0,1,1}, DEBUG_ALWAYS);
		// platformApi->draw_meshes(platformGraphics, 1, &scene->metaballTransform, scene->metaballMesh, scene->metaballMaterial);

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

		generate_mesh_marching_cubes(	scene->metaballVertexCapacity, scene->metaballVertices, &scene->metaballVertexCount,
										scene->metaballIndexCapacity, scene->metaballIndices, &scene->metaballIndexCount,
										sample_four_sdf_balls, positions, scene->metaballGridScale);

		platformApi->draw_procedural_mesh(	platformGraphics,
											scene->metaballVertexCount, scene->metaballVertices,
											scene->metaballIndexCount, scene->metaballIndices,
											scene->metaballTransform,
											scene->metaballMaterial);

		auto sample_sdf_2 = [](v3 position, void const * data)
		{
			v3 a = {2,2,2};
			v3 b = {5,3,5};

			f32 rA = 1;
			f32 rB = 1;

			// f32 d = min_f32(1, max_f32(0, dot_v3()))

			f32 t = min_f32(1, max_f32(0, dot_v3(position - a, b - a) / square_magnitude_v3(b-a)));
			f32 d = magnitude_v3(position - a  - t * (b -a));

			return d - f32_lerp(0.5,0.1,t);

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

		generate_mesh_marching_cubes(	scene->metaballVertexCapacity2, scene->metaballVertices2, &scene->metaballVertexCount2,
										scene->metaballIndexCapacity2, scene->metaballIndices2, &scene->metaballIndexCount2,
										sample_heightmap_for_mc, &scene->collisionSystem.terrainCollider, scene->metaballGridScale);

		debug_draw_circle_xy(multiply_point(scene->metaballTransform2, scene->metaballVertices2[0].position), 5.0f, colour_bright_green, DEBUG_ALWAYS);
		// logDebug(0) << multiply_point(scene->metaballTransform2, scene->metaballVertices2[0].position);

		if (scene->metaballVertexCount2 > 0 && scene->metaballIndexCount2 > 0)
		{
			platformApi->draw_procedural_mesh(	platformGraphics,
												scene->metaballVertexCount2, scene->metaballVertices2,
												scene->metaballIndexCount2, scene->metaballIndices2,
												scene->metaballTransform2,
												scene->metaballMaterial);
		}
	}

	{
		m44 trainTransformMatrix = transform_matrix(scene->trainTransform);
		platformApi->draw_meshes(platformGraphics, 1, &trainTransformMatrix, scene->trainMesh, scene->trainMaterial);
	}

	/// DRAW RACCOONS
	{
		m44 * raccoonTransformMatrices = push_memory<m44>(*global_transientMemory, scene->raccoonCount, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < scene->raccoonCount; ++i)
		{
			raccoonTransformMatrices[i] = transform_matrix(scene->raccoonTransforms[i]);
		}
		platformApi->draw_meshes(platformGraphics, scene->raccoonCount, raccoonTransformMatrices, scene->raccoonMesh, scene->raccoonMaterial);
	}

	for (auto & renderer : scene->renderers)
	{
		platformApi->draw_model(platformGraphics, renderer.model,
								transform_matrix(*renderer.transform),
								renderer.castShadows,
								nullptr, 0);
	}


	/// PLAYER
	/// CHARACTER 2
	{

		m44 boneTransformMatrices [32];
		
		// -------------------------------------------------------------------------------

		update_animated_renderer(boneTransformMatrices, scene->playerAnimaterRenderer.skeleton->bones);

		platformApi->draw_model(platformGraphics,
								scene->playerAnimaterRenderer.model,
								transform_matrix(scene->playerCharacterTransform),
								scene->playerAnimaterRenderer.castShadows,
								boneTransformMatrices,
								array_count(boneTransformMatrices));

		// -------------------------------------------------------------------------------

		update_animated_renderer(boneTransformMatrices, scene->noblePersonAnimatedRenderer.skeleton->bones);

		platformApi->draw_model(platformGraphics,
								scene->noblePersonAnimatedRenderer.model,
								transform_matrix(scene->noblePersonTransform),
								scene->noblePersonAnimatedRenderer.castShadows,
								boneTransformMatrices,
								array_count(boneTransformMatrices));
	}

	/// UPDATE LSYSTEM TREES
	{
		// TODO(Leo): This is stupid, the two use cases are too different, and we have to capture too much stuff :(

		auto get_region_colour_index = [](v2 position) -> s32
		{
			// Todo(Leo): sample from some kind of map
			return (s32)((position.y < 0) + 2 * (position.x < 0));
		};

		local_persist s32 persist_updatedLSystemIndex = -1;
		persist_updatedLSystemIndex += 1;
		persist_updatedLSystemIndex %= 10;
		s32 updatedLSystemIndex = persist_updatedLSystemIndex;

		auto update_lsystem = [	&waters 				= scene->waters,
								potWaterLevels 			= scene->potWaterLevels,
								lSystemsPotIndices 		= scene->lSystemsPotIndices,
								get_region_colour_index,
								get_water, 
								get_water_from_pot,
								updatedLSystemIndex,
								scaledTime]
							(s32 count, TimedLSystem * lSystems, Leaves * leavess)
		{

			for (int i = 0; i < count; ++i)
			{
				bool isInPot = lSystemsPotIndices[i] > 0;

				leavess[i].position 	= lSystems[i].position;
				leavess[i].rotation 	= lSystems[i].rotation;
				leavess[i].colourIndex 	= get_region_colour_index(leavess[i].position.xy);

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
										get_water_from_pot(potWaterLevels, lSystemsPotIndices[i], treeWaterDrainSpeed * timePassed) : 
										get_water (waters, lSystems[i].position, waterDistanceThreshold, treeWaterDrainSpeed * timePassed);

					if (waterLevel > 0)
					{
						if (lSystems[i].type == TREE_TYPE_1)
						{
							advance_lsystem_time(lSystems[i], timePassed);
							update_lsystem_mesh(lSystems[i], leavess[i]);
						}
						else if(lSystems[i].type == TREE_TYPE_2)
						{
							advance_lsystem_time_tree2(lSystems[i], timePassed);
							update_lsystem_mesh_tree2(lSystems[i], leavess[i]);
						}
						else if (lSystems[i].type == TREE_TYPE_CRYSTAL)
						{
							advance_lsystem_time_tree2(lSystems[i], timePassed);
							update_lsystem_mesh_tree2(lSystems[i], leavess[i]);
						}

						mesh_generate_normals(	lSystems[i].vertices.count, lSystems[i].vertices.data,
												lSystems[i].indices.count, lSystems[i].indices.data);

						mesh_generate_tangents(	lSystems[i].vertices.count, lSystems[i].vertices.data,
												lSystems[i].indices.count, lSystems[i].indices.data);

					}
				}

			}			
		};

		update_lsystem(scene->lSystemCount, scene->lSystems, scene->lSystemLeavess);

		auto draw_l_system_trees = [&materials 				= scene->treeMaterials, 
									&crystalTreeMaterials 	= scene->crystalTreeMaterials,
									get_region_colour_index] 
									(s32 count, TimedLSystem * lSystems)
		{
			for (s32 i = 0; i < count; ++i)
			{
				m44 transform = transform_matrix(lSystems[i].position, lSystems[i].rotation, make_uniform_v3(1));

				if (lSystems[i].vertices.count > 0 && lSystems[i].indices.count > 0)
				{

					MaterialHandle material;
					if (lSystems[i].type == TREE_TYPE_CRYSTAL)
					{
						s32 colourIndex = get_region_colour_index(lSystems[i].position.xy);
						material 		= crystalTreeMaterials[colourIndex];
					}
					else
					{
						material = materials[lSystems[i].type];
					}

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

	return do_gui(scene, input);
}

void * load_scene_3d(MemoryArena & persistentMemory, PlatformFileHandle saveFile)
{
	Scene3dSaveLoad save;
	if (saveFile != nullptr)
	{
		logDebug(0) << "Loading saved game";

		platformApi->set_file_position(saveFile, 0);
		file_read_struct(saveFile, &save);
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

	u32 guiTexturePixelColor 		= 0xffffffff;
	TextureAsset guiTextureAsset 	= make_texture_asset(&guiTexturePixelColor, 1, 1, 4);
	scene->gui.panelTexture			= platformApi->push_gui_texture(platformGraphics, &guiTextureAsset);

	// ----------------------------------------------------------------------------------

	// TODO(Leo): these should probably all go away
	// Note(Leo): amounts are SWAG, rethink.
	scene->transforms 			= allocate_array<Transform3D>(persistentMemory, 1200);
	scene->renderers 			= allocate_array<Renderer>(persistentMemory, 600);

	scene->collisionSystem.boxColliders 		= allocate_array<BoxCollider>(persistentMemory, 600);
	scene->collisionSystem.cylinderColliders 	= allocate_array<CylinderCollider>(persistentMemory, 600);
	scene->collisionSystem.staticBoxColliders 	= allocate_array<StaticBoxCollider>(persistentMemory, 2000);

	scene->fallingObjectCapacity = 100;
	scene->fallingObjectCount = 0;
	scene->fallingObjects = push_memory<FallingObject>(persistentMemory, scene->fallingObjectCapacity, ALLOC_CLEAR);

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

	u32 whitePixelColour		= 0xffffffff;
	u32 blackPixelColour 		= 0xff000000;
	u32 neutralBumpPixelColour 	= colour_rgba_u32(colour_bump);
	u32 waterBluePixelColour 	= colour_rgb_alpha_32(colour_aqua_blue.rgb, 0.7);
	u32 seedBrownPixelColour 	= 0xff003399;

	TextureAsset whiteTextureAsset 			= make_texture_asset(&whitePixelColour, 1, 1, 4);
	TextureAsset blackTextureAsset	 		= make_texture_asset(&blackPixelColour, 1, 1, 4);
	TextureAsset neutralBumpTextureAsset 	= make_texture_asset(&neutralBumpPixelColour, 1, 1, 4);
	TextureAsset waterBlueTextureAsset 		= make_texture_asset(&waterBluePixelColour, 1, 1, 4);
	TextureAsset seedBrownTextureAsset 		= make_texture_asset(&seedBrownPixelColour, 1, 1, 4);

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
		auto tilesAlbedo 	= load_and_push_texture("assets/textures/tiles.png");
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
		
		{
			v4 niceSkyGradientColors [] =
			{
				{0.11, 0.07, 0.25, 	1.0},
				{0.4, 0.9, 1.2,		1.0},
				{0.8, 0.99, 1.2,	1.0},
				{2,2,2, 			1.0},
			};
			f32 niceSkyGradientTimes [] 		= {0.1, 0.3, 0.9, 0.99};

			TextureAsset niceSkyGradientAsset 	= generate_gradient(*global_transientMemory, 4, niceSkyGradientColors, niceSkyGradientTimes, 128);
			niceSkyGradientAsset.addressMode 	= TEXTURE_ADDRESS_MODE_CLAMP;
			auto niceSkyGradient				= platformApi->push_texture(platformGraphics, &niceSkyGradientAsset);

			v4 meanSkyGradientColors [] =
			{
				{0.1,0.05,0.05,		1.0},
				{1.1, 0.1, 0.05,	1.0},
				{1.1, 0.1, 0.05,	1.0},
				{1.4, 0.95, 0.8,	1.0},
				{2, 1.2, 1.2, 		1.0},
			};
			f32 meanSkyGradientTimes [] 			= {0.1, 0.3, 0.7, 0.9, 0.99};
			TextureAsset meanSkyGradientAsset 		= generate_gradient(*global_transientMemory, 5, meanSkyGradientColors, meanSkyGradientTimes, 128);
			meanSkyGradientAsset.addressMode 		= TEXTURE_ADDRESS_MODE_CLAMP;
			auto meanSkyGradientTexture				= platformApi->push_texture(platformGraphics, &meanSkyGradientAsset);

			TextureHandle skyGradientTextures [] 	= {niceSkyGradient, meanSkyGradientTexture};

			materials.sky 							= platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_SKYBOX, 2, skyGradientTextures);
		}
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

		// Exporting animations from blender is not easy, this file has working animations, previous has proper model
		auto const animationFile 	= read_gltf_file(*global_transientMemory, "assets/models/cube_head_v3.glb");

		auto girlMeshAsset 	= load_mesh_glb(*global_transientMemory, gltfFile, "cube_head");
		auto girlMesh 		= platformApi->push_mesh(platformGraphics, &girlMeshAsset);

		// --------------------------------------------------------------------
	
		scene->playerCharacterTransform = {.position = {10, 0, 5}};

		if (saveFile != nullptr)
		{
			platformApi->set_file_position(saveFile, save.playerLocation);
			file_read_struct(saveFile, &scene->playerCharacterTransform);
		}

		auto * motor 		= &scene->playerCharacterMotor;
		motor->transform 	= &scene->playerCharacterTransform;

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

		scene->playerSkeletonAnimator = 
		{
			.skeleton 		= load_skeleton_glb(persistentMemory, gltfFile, "cube_head"),
			.animations 	= motor->animations,
			.weights 		= motor->animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		};

		// Note(Leo): take the reference here, so we can easily copy this down below.
		auto & cubeHeadSkeleton = scene->playerSkeletonAnimator.skeleton;

		auto model = push_model(girlMesh, materials.character);
		scene->playerAnimaterRenderer = make_animated_renderer(&scene->playerCharacterTransform, &cubeHeadSkeleton, model);

		// --------------------------------------------------------------------

		{
			v3 position = {random_range(-99, 99), random_range(-99, 99), 0};
			v3 scale 	= make_uniform_v3(random_range(0.8f, 1.5f));

			scene->noblePersonTransform.position 	= position;
			scene->noblePersonTransform.scale 		= scale;

			scene->noblePersonCharacterMotor = {};
			scene->noblePersonCharacterMotor.transform = &scene->noblePersonTransform;

			{
				using namespace CharacterAnimations;
				
				scene->noblePersonCharacterMotor.animations[WALK] 	= &scene->characterAnimations[WALK];
				scene->noblePersonCharacterMotor.animations[RUN] 	= &scene->characterAnimations[RUN];
				scene->noblePersonCharacterMotor.animations[IDLE]	= &scene->characterAnimations[IDLE];
				scene->noblePersonCharacterMotor.animations[JUMP] 	= &scene->characterAnimations[JUMP];
				scene->noblePersonCharacterMotor.animations[FALL] 	= &scene->characterAnimations[FALL];
				scene->noblePersonCharacterMotor.animations[CROUCH] = &scene->characterAnimations[CROUCH];
			}

			scene-> noblePersonSkeletonAnimator = 
			{
				.skeleton 		= { .bones = copy_array(persistentMemory, cubeHeadSkeleton.bones) },
				.animations 	= scene->noblePersonCharacterMotor.animations,
				.weights 		= scene->noblePersonCharacterMotor.animationWeights,
				.animationCount = CharacterAnimations::ANIMATION_COUNT
			};

			auto model = push_model(girlMesh, materials.character); 
			scene->noblePersonAnimatedRenderer = make_animated_renderer(&scene->noblePersonTransform, &scene->noblePersonSkeletonAnimator.skeleton, model);

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

		/// RACCOONS
		{
			scene->raccoonEmptyAnimation = {};

			scene->raccoonCount 			= 4;
			scene->raccoonModes 			= push_memory<s32>(persistentMemory, scene->raccoonCount, ALLOC_CLEAR);
			scene->raccoonTransforms 		= push_memory<Transform3D>(persistentMemory, scene->raccoonCount, ALLOC_CLEAR);
			scene->raccoonTargetPositions 	= push_memory<v3>(persistentMemory, scene->raccoonCount, ALLOC_CLEAR);
			scene->raccoonCharacterMotors	= push_memory<CharacterMotor>(persistentMemory, scene->raccoonCount, ALLOC_CLEAR);

			for (s32 i = 0; i < scene->raccoonCount; ++i)
			{
				scene->raccoonModes[i]					= RACCOON_IDLE;

				scene->raccoonTransforms[i] 			= identity_transform;
				scene->raccoonTransforms[i].position 	= random_inside_unit_square() * 100 - v3{50, 50, 0};
				scene->raccoonTransforms[i].position.z  = get_terrain_height(scene->collisionSystem, scene->raccoonTransforms[i].position.xy);

				scene->raccoonTargetPositions[i] 		= random_inside_unit_square() * 100 - v3{50,50,0};
				scene->raccoonTargetPositions[i].z  	= get_terrain_height(scene->collisionSystem, scene->raccoonTargetPositions[i].xy);

				// ------------------------------------------------------------------------------------------------
		
				scene->raccoonCharacterMotors[i] = {};
				scene->raccoonCharacterMotors[i].transform = &scene->raccoonTransforms[i];

				{
					using namespace CharacterAnimations;

					scene->raccoonCharacterMotors[i].animations[WALK] 		= &scene->raccoonEmptyAnimation;
					scene->raccoonCharacterMotors[i].animations[RUN] 		= &scene->raccoonEmptyAnimation;
					scene->raccoonCharacterMotors[i].animations[IDLE]		= &scene->raccoonEmptyAnimation;
					scene->raccoonCharacterMotors[i].animations[JUMP] 		= &scene->raccoonEmptyAnimation;
					scene->raccoonCharacterMotors[i].animations[FALL] 		= &scene->raccoonEmptyAnimation;
					scene->raccoonCharacterMotors[i].animations[CROUCH] 	= &scene->raccoonEmptyAnimation;
				}
			}

			auto raccoonGltfFile 	= read_gltf_file(*global_transientMemory, "assets/models/raccoon.glb");
			auto raccoonMeshAsset 	= load_mesh_glb(*global_transientMemory, raccoonGltfFile, "raccoon");
			scene->raccoonMesh 		= platformApi->push_mesh(platformGraphics, &raccoonMeshAsset);

			auto albedoTexture = load_and_push_texture("assets/textures/RaccoonAlbedo.png");
			TextureHandle textures [] =	{albedoTexture, neutralBumpTexture, blackTexture};

			scene->raccoonMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, textures);




			// scene-> noblePersonSkeletonAnimator = 
			// {
			// 	.skeleton 		= { .bones = copy_array(persistentMemory, cubeHeadSkeleton.bones) },
			// 	.animations 	= scene->noblePersonCharacterMotor.animations,
			// 	.weights 		= scene->noblePersonCharacterMotor.animationWeights,
			// 	.animationCount = CharacterAnimations::ANIMATION_COUNT
			// };

			// auto model = push_model(girlMesh, materials.character); 
			// scene->noblePersonAnimatedRenderer = make_animated_renderer(&scene->noblePersonTransform, &scene->noblePersonSkeletonAnimator.skeleton, model);

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

			if (saveFile != nullptr)
			{
				platformApi->set_file_position(saveFile, save.potsLocation);
				file_read_struct(saveFile, &scene->potCapacity);
				file_read_struct(saveFile, &scene->potCount);

				scene->potTransforms = push_memory<Transform3D>(persistentMemory, scene->potCapacity, ALLOC_NO_CLEAR);
				scene->potWaterLevels = push_memory<f32>(persistentMemory, scene->potCapacity, ALLOC_NO_CLEAR);

				file_read_memory(saveFile, scene->potCount, scene->potTransforms);
				file_read_memory(saveFile, scene->potCount, scene->potWaterLevels);
			}
			else
			{
				scene->potCapacity 			= 10;
				scene->potCount 		= scene->potCapacity;
				scene->potTransforms 	= push_memory<Transform3D>(persistentMemory, scene->potCapacity, ALLOC_NO_CLEAR);
				scene->potWaterLevels 	= push_memory<f32>(persistentMemory, scene->potCapacity, 0);

				for(s32 i = 0; i < scene->potCapacity; ++i)
				{
					v3 position 					= {15, i * 5.0f, 0};
					position.z 						= get_terrain_height(scene->collisionSystem, position.xy);
					scene->potTransforms[i]	= { .position = position };
				}
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

		/// TRAIN
		{
			auto file 				= read_gltf_file(*global_transientMemory, "assets/models/train.glb");
			auto meshAsset 			= load_mesh_glb(*global_transientMemory, file, "train");
			scene->trainMesh 		= platformApi->push_mesh(platformGraphics, &meshAsset);
			scene->trainMaterial 	= materials.environment;

			// ----------------------------------------------------------------------------------

			scene->trainStopPosition 	= {50, 0, 0};
			scene->trainStopPosition.z 	= get_terrain_height(scene->collisionSystem, scene->trainStopPosition.xy);
			scene->trainStopPosition.z 	= max_f32(0, scene->trainStopPosition.z);

			f32 trainFarDistance 		= 2000;

			scene->trainFarPositionA 	= {50, trainFarDistance, 0};
			scene->trainFarPositionA.z 	= get_terrain_height(scene->collisionSystem, scene->trainFarPositionA.xy);
			scene->trainFarPositionA.z 	= max_f32(0, scene->trainFarPositionA.z);

			scene->trainFarPositionB 	= {50, -trainFarDistance, 0};
			scene->trainFarPositionB.z 	= get_terrain_height(scene->collisionSystem, scene->trainFarPositionB.xy);
			scene->trainFarPositionB.z 	= max_f32(0, scene->trainFarPositionB.z);

			scene->trainTransform.position 			= scene->trainStopPosition;

			scene->trainMoveState 					= TRAIN_WAIT;

			scene->trainFullSpeed 					= 200;
			scene->trainStationMinSpeed 			= 1;
			scene->trainAcceleration 				= 20;
			scene->trainWaitTimeOnStop 				= 5;

			/*
			v = v0 + a*t
			-> t = (v - v0) / a
			d = d0 + v0*t + 1/2*a*t^2
			d - d0 = v0*t + 1/2*a*t^2
			*/

			f32 timeToBrakeBeforeStation 			= (scene->trainFullSpeed - scene->trainStationMinSpeed) / scene->trainAcceleration;
			scene->trainBrakeBeforeStationDistance 	= scene->trainFullSpeed * timeToBrakeBeforeStation
													// Note(Leo): we brake, so acceleration term is negative
													- 0.5f * scene->trainAcceleration * timeToBrakeBeforeStation * timeToBrakeBeforeStation;

			logDebug(0) << "brake time: " << timeToBrakeBeforeStation << ", distance: " << scene->trainBrakeBeforeStationDistance;

			scene->trainCurrentWaitTime = 0;
			scene->trainCurrentSpeed 	= 0;

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
		TextureAsset albedo = load_texture_asset(*global_transientMemory, "assets/textures/bark.png");
		TextureAsset normal = load_texture_asset(*global_transientMemory, "assets/textures/bark_normal.png");

		TextureHandle treeTextures [] =
		{	
			platformApi->push_texture(platformGraphics, &albedo),
			platformApi->push_texture(platformGraphics, &normal),
			blackTexture
		};
		// scene->lSystemTreeMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, treeTextures);
		MaterialHandle treeMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_NORMAL, 3, treeTextures);
		scene->treeMaterials[TREE_TYPE_1] = treeMaterial;
		scene->treeMaterials[TREE_TYPE_2] = treeMaterial;
		scene->treeMaterials[TREE_TYPE_CRYSTAL] = scene->waterMaterial;

		MeshHandle tree1SeedMesh = seedMesh1;
		MeshHandle tree2SeedMesh = seedMesh2;

		MaterialHandle tree1SeedMaterial = seedMaterial1;
		MaterialHandle tree2SeedMaterial = seedMaterial2;

		MaterialHandle leavesMaterial;
		{
			auto leafTextureAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/leaf_mask.png");
			auto leavesTexture 		= platformApi->push_texture(platformGraphics, &leafTextureAsset);
			leavesMaterial 			= platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_LEAVES, 1, &leavesTexture);
		}
		
		/// CRYSTAL TREE MATERIALS
		{
			v4 regionColours [] =
			{
				{0.62, 0.3, 0.8, 0.6},
				{0.3, 0.62, 0.8, 0.6},
				{0.47, 0.7, 0.40, 0.6},
				{1.0, 0.7, 0.8, 0.6},
			};

			for (s32 i = 0; i < 4; ++i)
			{
				u32 pixelColour = colour_rgba_u32(regionColours[i]);
				auto asset = make_texture_asset(&pixelColour, 1, 1, 4);
				auto texture = platformApi->push_texture(platformGraphics, &asset);

				TextureHandle treeTextures [] = {texture, treeTextures[1], blackTexture};
				scene->crystalTreeMaterials[i] = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_WATER, 3, treeTextures);
			}
		}

		if (saveFile != nullptr)
		{
			platformApi->set_file_position(saveFile, save.treesLocation);
			
			file_read_struct(saveFile, &scene->lSystemCapacity);
			file_read_struct(saveFile, &scene->lSystemCount);

			scene->lSystems 			= push_memory<TimedLSystem>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);
			scene->lSystemLeavess 		= push_memory<Leaves>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);
			scene->lSystemsPotIndices 	= push_memory<s32>(persistentMemory, scene->lSystemCapacity, ALLOC_NO_CLEAR);

			for (s32 i = 0; i < scene->lSystemCapacity; ++i)
			{
				file_read_struct(saveFile, &scene->lSystems[i].wordCapacity);
				file_read_struct(saveFile, &scene->lSystems[i].wordCount);

				scene->lSystems[i].aWord = push_memory<Module>(persistentMemory, scene->lSystems[i].wordCapacity, ALLOC_NO_CLEAR);
				scene->lSystems[i].bWord = push_memory<Module>(persistentMemory, scene->lSystems[i].wordCapacity, ALLOC_NO_CLEAR);
				file_read_memory(saveFile, scene->lSystems[i].wordCount, scene->lSystems[i].aWord);

				file_read_struct(saveFile, &scene->lSystems[i].timeSinceLastUpdate);
				file_read_struct(saveFile, &scene->lSystems[i].position);
				file_read_struct(saveFile, &scene->lSystems[i].rotation);
				file_read_struct(saveFile, &scene->lSystems[i].type);
				file_read_struct(saveFile, &scene->lSystems[i].totalAge);
				file_read_struct(saveFile, &scene->lSystems[i].maxAge);

				scene->lSystems[i].vertices = push_memory_view<Vertex>(persistentMemory, 15'000);
				scene->lSystems[i].indices = push_memory_view<u16>(persistentMemory, 45'000);

				// scene->lSystemsPotIndices[i] = -1;
				scene->lSystemLeavess[i] 			= make_leaves(persistentMemory, 4000);
				scene->lSystemLeavess[i].material 	= leavesMaterial;

				if (scene->lSystems[i].type == TREE_TYPE_1)
				{
					scene->lSystems[i].seedMesh 	= tree1SeedMesh;
					scene->lSystems[i].seedMaterial = tree1SeedMaterial;
				}
				else if (scene->lSystems[i].type == TREE_TYPE_2)
				{
					scene->lSystems[i].seedMesh 	= tree2SeedMesh;
					scene->lSystems[i].seedMaterial = tree1SeedMaterial;
				}
				else if (scene->lSystems[i].type == TREE_TYPE_CRYSTAL)
				{
					scene->lSystems[i].seedMesh 	= tree1SeedMesh;
					scene->lSystems[i].seedMaterial = scene->waterMaterial;
				}
			
				update_lsystem_mesh_tree2(scene->lSystems[i], scene->lSystemLeavess[i]);
			}	
			file_read_memory(saveFile, scene->lSystemCount, scene->lSystemsPotIndices);

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
				scene->lSystemLeavess[i].material = leavesMaterial;

				if (i < 5)
				{
					scene->lSystems[i].type 		= TREE_TYPE_CRYSTAL;
					scene->lSystems[i].maxAge		= 50;
					scene->lSystems[i].aWord[0]		= {'A', 0, 0};

					scene->lSystems[i].seedMesh 	= tree1SeedMesh;
					scene->lSystems[i].seedMaterial = scene->waterMaterial;
				}
				else if ((i & 2) == 0)
				{
					scene->lSystems[i].type 		= TREE_TYPE_1;
					scene->lSystems[i].seedMesh 	= tree1SeedMesh;
					scene->lSystems[i].seedMaterial = tree1SeedMaterial;
				}
				else
				{
					scene->lSystems[i].type 		= TREE_TYPE_2;
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

	/// MARCHING CUBES AND METABALLS TESTING
	{
		v3 position = {-10, -30, 0};
		position.z = get_terrain_height(scene->collisionSystem, position.xy) + 3;

		s32 vertexCountPerCube 	= 14;
		s32 indexCountPerCube 	= 36;
		s32 cubeCapacity 		= 100000;
		s32 vertexCapacity 		= vertexCountPerCube * cubeCapacity;
		s32 indexCapacity 		= indexCountPerCube * cubeCapacity;

		Vertex * vertices 	= push_memory<Vertex>(persistentMemory, vertexCapacity, ALLOC_NO_CLEAR);
		u16 * indices 		= push_memory<u16>(persistentMemory, indexCapacity, ALLOC_NO_CLEAR);

		u32 vertexCount;
		u32 indexCount;

		scene->metaballGridScale = 0.3;

		scene->metaballVertexCount 	= vertexCount;
		scene->metaballVertices 	= vertices;

		scene->metaballIndexCount 	= indexCount;
		scene->metaballIndices 		= indices;

		scene->metaballVertexCapacity 	= vertexCapacity;
		scene->metaballIndexCapacity 	= indexCapacity;

		scene->metaballTransform 	= translation_matrix(position);

		// Todo(Leo): textures are copied too many times: from file to stb, from stb to TextureAsset, from TextureAsset to graphics.
		TextureAsset albedo = load_texture_asset(*global_transientMemory, "assets/textures/Acorn_albedo.png");
		// TextureAsset normal = load_texture_asset(*global_transientMemory, "assets/textures/ground_normal.png");

		TextureHandle treeTextures [] =
		{	
			platformApi->push_texture(platformGraphics, &albedo),
		};
		scene->metaballMaterial = platformApi->push_material(platformGraphics, GRAPHICS_PIPELINE_TRIPLANAR, 1, treeTextures);

		// ----------------------------------------------------------------------------------

		scene->metaballVertexCapacity2 	= vertexCapacity;
		scene->metaballVertexCount2 	= 0;
		scene->metaballVertices2 		= push_memory<Vertex>(persistentMemory, vertexCapacity, ALLOC_NO_CLEAR);

		scene->metaballIndexCapacity2 	= vertexCapacity;
		scene->metaballIndexCount2 		= 0;
		scene->metaballIndices2 		= push_memory<u16>(persistentMemory, indexCapacity, ALLOC_NO_CLEAR);

		v3 position2 				= {0, -30, 0};
		position2.z 				= get_terrain_height(scene->collisionSystem, position2.xy) + 3;
		scene->metaballTransform2 	= translation_matrix(position2);

	}

	// ----------------------------------------------------------------------------------

	logSystem(0) << "Scene3d loaded, " << used_percent(*global_transientMemory) * 100 << "% of transient memory used.";
	logSystem(0) << "Scene3d loaded, " << used_percent(persistentMemory) * 100 << "% of persistent memory used.";

	return scenePtr;
}
