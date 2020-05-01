/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for 3d development scene
=============================================================================*/
#include "TerrainGenerator.cpp"
#include "Collisions3D.cpp"

#include "CharacterMotor.cpp"
#include "PlayerController3rdPerson.cpp"
#include "FollowerController.cpp"

#include "DefaultSceneGui.cpp"

struct Scene3d
{
	// Components
	Array<Transform3D> 			transforms;

	Array<AnimatedSkeleton> 	animatedSkeletons;
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
	CameraController3rdPerson 	cameraController;

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
	
	f32 treeGrowths	[seedCapacity];
	Transform3D treeTransforms[seedCapacity];
	s32 treesInPotCount;
	s32 treesOnGroundCount;

	ModelHandle waterModel;
	ModelHandle seedModel;
	ModelHandle treeModel;
	ModelHandle potModel;

	enum : s32 { CARRY_NONE, CARRY_POT, CARRY_WATER, CARRY_SEED };
	s32 playerCarryState;
	s32 carriedItemIndex;

	// Other actors
	FollowerController followers[150];
	RandomWalkController randomWalkers [10];

	// Data
	Animation characterAnimations [CharacterAnimations::ANIMATION_COUNT];

	// Random
	ModelHandle skybox;
	SceneGui gui;

 	// Todo(Leo): maybe make free functions
	static MenuResult
	update(	void * 						scenePtr, 
			game::Input * 				input,
			platform::Graphics*,
			platform::Window*,
			platform::Functions*);

	static void
	load(	void * 						scenePtr,
			MemoryArena * 				persistentMemory,
			MemoryArena * 				transientMemory,
			platform::Graphics*,
			platform::Window*,
			platform::Functions*);
};

SceneInfo get_3d_scene_info()
{
	return make_scene_info(	get_size_of<Scene3d>,
							Scene3d::load,
							Scene3d::update);
}


MenuResult
Scene3d::update(	void * 					scenePtr,
					game::Input * 			input,
					platform::Graphics *,
					platform::Window *,
					platform::Functions *)
{
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);

	/* Sadly, we need to draw skybox before game logic, because otherwise
	debug lines would be hidden. This can be fixed though, just make debug pipeline similar to shadows. */ 
	platformApi->draw_model(platformGraphics, scene->skybox, m44::identity(), false, nullptr, 0);

	/*
	1. update input
	2. update animation controller
	3. update animator
	4. update animated renderer
	5. draw
	
	*/

	// Game Logic section
	update_player_input(scene->playerInputState, scene->characterInputs, scene->worldCamera, *input);
	
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
		s32 oldCarryState = scene->playerCarryState;
		s32 newCarryState = scene->playerCarryState;

		switch(scene->playerCarryState)
		{
			case CARRY_NONE: {
				for (s32 i = 0; i < scene->totalPotsCount; ++i)
				{
					if (magnitude(playerPosition - scene->potTransforms[i].position) < grabDistance)
					{
						scene->playerCarryState = CARRY_POT;
						scene->carriedItemIndex = i;
						goto ESCAPE;
					}
				}

				for (s32 i = 0; i < scene->waterCount; ++i)
				{
					if(magnitude(playerPosition - scene->waterTransforms[i].position) < grabDistance)
					{
						scene->playerCarryState = CARRY_WATER;
						scene->carriedItemIndex = i;
						goto ESCAPE;
					}
				}

				for (s32 i = 0; i < scene->currentSeedCount; ++i)
				{
					if (magnitude(playerPosition - scene->seedTransforms[i].position) < grabDistance)
					{
						scene->playerCarryState = CARRY_SEED;
						scene->carriedItemIndex = i;
						goto ESCAPE;
					}
				}
			
				// Note(Leo): Goto here if we start carrying in some of the loops above, so we dont override it immediately.
				ESCAPE:;
			} break;

			case CARRY_POT:
				scene->playerCarryState = CARRY_NONE;
				scene->potTransforms[scene->carriedItemIndex].position.z = get_terrain_height(&scene->collisionSystem, *scene->potTransforms[scene->carriedItemIndex].position.xy());
				break;
				
			case CARRY_WATER:
			{
				Transform3D & waterTransform = scene->waterTransforms[scene->carriedItemIndex];

				scene->playerCarryState = CARRY_NONE;
				waterTransform.position.z = get_terrain_height(&scene->collisionSystem, *waterTransform.position.xy());

				/// LOOK FOR NEARBY SEEDS ON GROUND
				for (s32 seedIndex = 0; seedIndex < scene->currentSeedCount; ++seedIndex)
				{
					// Todo(Leo): Once we have some benchmarking, test if this was faster with all distances computed before
					f32 distanceToSeed = magnitude(waterTransform.position - scene->seedTransforms[seedIndex].position);
					if (distanceToSeed < 0.5f)
					{
						s32 treeIndex = scene->treesInPotCount + scene->treesOnGroundCount;
						scene->treeGrowths[treeIndex] = 0.5f;
						scene->treeTransforms[treeIndex].position = scene->seedTransforms[seedIndex].position;
						scene->treeTransforms[treeIndex].rotation = quaternion::identity();
						scene->treeTransforms[treeIndex].scale = scene->treeGrowths[treeIndex];

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
					f32 distanceToPotWithSeed = magnitude(waterTransform.position - scene->potTransforms[potIndex].position);
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
						scene->treeTransforms[newTreeInPotIndex].rotation = quaternion::identity();
						scene->treeTransforms[newTreeInPotIndex].scale = scene->treeGrowths[newTreeInPotIndex];

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
					f32 distanceToTreeInPot = magnitude(waterTransform.position - scene->treeTransforms[treeIndex].position);
					if (distanceToTreeInPot < 0.5f)
					{
						// scene->treeGrowths[treeIndex] += 0.5f;
						scene->treeGrowths[treeIndex] = math::min(scene->treeGrowths[treeIndex] + 0.5f, 1.0f);
						scene->treeTransforms[treeIndex].scale = scene->treeGrowths[treeIndex];

						scene->waterCount -= 1;
						scene->waterTransforms[scene->carriedItemIndex] = scene->waterTransforms[scene->waterCount];

						break;
					}
				}
				
				/// LOOK FOR NEARBY TREES ON GROUND
				for (s32 treeIndex = scene->treesInPotCount; treeIndex < (scene->treesInPotCount + scene->treesOnGroundCount); ++treeIndex)
				{
					f32 distnceToTreeOnGround = magnitude(waterTransform.position - scene->treeTransforms[treeIndex].position);
					if (distnceToTreeOnGround < 0.5f)
					{
						scene->treeGrowths[treeIndex] += 0.5f;
						scene->treeTransforms[treeIndex].scale = scene->treeGrowths[treeIndex];

						scene->waterCount -= 1;
						scene->waterTransforms[scene->carriedItemIndex] = scene->waterTransforms[scene->waterCount];

						break;
					}
				}

			} break;

			case CARRY_SEED:
				scene->playerCarryState = CARRY_NONE;
				scene->seedTransforms[scene->carriedItemIndex].position.z = get_terrain_height(&scene->collisionSystem, *scene->seedTransforms[scene->carriedItemIndex].position.xy());

				s32 firstEmptyPot = scene->potsWithTreeCount + scene->potsWithSeedCount;
				for (s32 potIndex = firstEmptyPot; potIndex < scene->totalPotsCount; ++potIndex)
				{
					f32 distanceToEmptyPot = magnitude(scene->potTransforms[potIndex].position - scene->seedTransforms[scene->carriedItemIndex].position);
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


	v3 carriedPosition = multiply_point(get_matrix(*scene->playerCharacterTransform), {0, 0.7, 0.7});
	switch(scene->playerCarryState)
	{
		case CARRY_POT:
			scene->potTransforms[scene->carriedItemIndex].position = carriedPosition;
			break;

		case CARRY_WATER:
			scene->waterTransforms[scene->carriedItemIndex].position = carriedPosition;
			break;

		case CARRY_SEED:
			scene->seedTransforms[scene->carriedItemIndex].position = carriedPosition;
			break;

		default:
			break;
	}

	// Todo(Leo): We really do not need to update these unless they are carried, figure how to know which one is carried,
	// since they do not directly map to pot indices
	for (s32 i = 0; i < scene->treesInPotCount; ++i)
	{
		// Note(Leo): these happaned to naturally map one to one, see comment in CARRY_WATER case in dropping section
		scene->treeTransforms[i].position = scene->potTransforms[i].position + v3{0, 0, 0.25};
	}
	
	// -----------------------------------------------------------------------------------------------------------

	for (auto const & collider : scene->collisionSystem.cylinderColliders)
	{
		debug::draw_circle_xy(collider.transform->position + collider.center, collider.radius, colors::brightYellow);
	}

	debug::draw_circle_xy(scene->playerCharacterTransform->position + v3{0,0,0.7}, 0.25f, colors::brightGreen);


	for (auto & randomWalker : scene->randomWalkers)
	{
		update_random_walker_input(randomWalker, scene->characterInputs, input->elapsedTime);
	}

	for (auto & follower : scene->followers)
	{
		update_follower_input(follower, scene->characterInputs, input->elapsedTime);
	}
	
	update_camera_controller(&scene->cameraController, input);

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
	for (s32 i = 0; i <scene->waterCount; ++i)
	{
		platformApi->draw_model(platformGraphics, scene->waterModel,
								get_matrix(scene->waterTransforms[i]),
								true,
								nullptr, 0);
	}

	/// DRAW UNUSED SEEDS
	for (s32 i = 0; i < scene->currentSeedCount; ++i)
	{
		platformApi->draw_model(platformGraphics, scene->seedModel,
								get_matrix(scene->seedTransforms[i]),
								true,
								nullptr, 0);
	}

	/// DRAW TREES
	s32 totalTreeCount = scene->treesInPotCount + scene->treesOnGroundCount;
	for (s32 i = 0; i < totalTreeCount; ++i)
	{
		platformApi->draw_model(platformGraphics, scene->treeModel,
								get_matrix(scene->treeTransforms[i]),
								true,
								nullptr, 0);
	}

	/// DRAW POTS
	for (s32 i = 0; i < scene->totalPotsCount; ++i)
	{
		platformApi->draw_model(platformGraphics, scene->potModel,
								get_matrix(scene->potTransforms[i]),
								true,
								nullptr, 0);
	}

	/// DRAW SEEDS INSIDE POTS
	for (s32 i = 0; i < scene->potsWithSeedCount; ++i)
	{
		s32 potWithSeedIndex = i + scene->potsWithTreeCount;
		platformApi->draw_model(platformGraphics, scene->seedModel,
								get_matrix(scene->potTransforms[potWithSeedIndex]),
								true,
								nullptr, 0);
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

	Light light = { .direction 	= v3{1, 1.2, -3}.normalized(), 
					.color 		= v3{0.95, 0.95, 0.9}};
	v3 ambient 	= {0.1, 0.1, 0.5};
	platformApi->update_lighting(platformGraphics, &light, &scene->worldCamera, ambient);

	for (auto & renderer : scene->renderers)
	{
		platformApi->draw_model(platformGraphics, renderer.model,
								get_matrix(*renderer.transform),
								renderer.castShadows,
								nullptr, 0);
	}


	
	for (auto & renderer : scene->animatedRenderers)
	{
		update_animated_renderer(renderer);

		platformApi->draw_model(platformGraphics, renderer.model, get_matrix(*renderer.transform),
								renderer.castShadows, renderer.bones.data(),
								renderer.bones.count());
	}


	// ------------------------------------------------------------------------


	auto result = update_scene_gui(&scene->gui, input, platformGraphics, platformApi);
	return result;
}

void Scene3d::load(	void * 						scenePtr, 
					MemoryArena * 				persistentMemory,
					MemoryArena *,
					platform::Graphics *,
					platform::Window *,
					platform::Functions *)
{
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);
	
	// Note(Leo): This is good, more this.
	scene->gui = make_scene_gui(global_transientMemory, platformGraphics, platformApi);

	// Note(Leo): amounts are SWAG, rethink.
	scene->transforms 			= allocate_array<Transform3D>(*persistentMemory, 1200);
	scene->animatedSkeletons 	= allocate_array<AnimatedSkeleton>(*persistentMemory, 600);
	scene->skeletonAnimators 	= allocate_array<SkeletonAnimator>(*persistentMemory, 600);
	scene->animatedRenderers 	= allocate_array<AnimatedRenderer>(*persistentMemory, 600);
	scene->renderers 			= allocate_array<Renderer>(*persistentMemory, 600);

	// Todo(Leo): Probaly need to allocate these more coupledly, at least they must be able to reordered together
	scene->characterMotors		= allocate_array<CharacterMotor>(*persistentMemory, 600);
	scene->characterInputs		= allocate_array<CharacterInput>(*persistentMemory, scene->characterMotors.capacity(), ALLOC_FILL | ALLOC_NO_CLEAR);

	scene->collisionSystem.boxColliders 	= allocate_array<BoxCollider>(*persistentMemory, 600);
	scene->collisionSystem.cylinderColliders = allocate_array<CylinderCollider>(*persistentMemory, 600);

	logSystem() << "Allocations succesful! :)";

	// ----------------------------------------------------------------------------------

	struct MaterialCollection {
		MaterialHandle character;
		MaterialHandle environment;
		MaterialHandle ground;
		MaterialHandle sky;
	} materials;

	PipelineHandle characterPipeline 	= platformApi->push_pipeline(platformGraphics, "assets/shaders/animated_vert.spv", "assets/shaders/frag.spv", { .textureCount = 3});
	PipelineHandle normalPipeline 		= platformApi->push_pipeline(platformGraphics, "assets/shaders/vert.spv", "assets/shaders/frag.spv", {.textureCount = 3});
	PipelineHandle terrainPipeline 		= platformApi->push_pipeline(platformGraphics, "assets/shaders/vert.spv", "assets/shaders/terrain_frag.spv", {.textureCount = 3});
	PipelineHandle skyPipeline 			= platformApi->push_pipeline(platformGraphics, "assets/shaders/vert_sky.spv", "assets/shaders/frag_sky.spv", {.textureCount = 1, .enableDepth = false});

	auto load_and_push_texture = [](const char * path) -> TextureHandle
	{
		auto asset = load_texture_asset(path, global_transientMemory);
		auto result = platformApi->push_texture(platformGraphics, &asset);
		return result;
	};

	TextureAsset whiteTextureAsset 			= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xffffffff}), 1, 1);
	TextureAsset blackTextureAsset	 		= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff000000}), 1, 1);
	TextureAsset neutralBumpTextureAsset 	= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff8080ff}), 1, 1);
	TextureAsset waterBlueTextureAsset 		= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xffffdb00}), 1, 1);
	TextureAsset seedBrownTextureAsset 		= make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff003399}), 1, 1);

	TextureHandle whiteTexture 			= platformApi->push_texture(platformGraphics, &whiteTextureAsset);
	TextureHandle blackTexture 			= platformApi->push_texture(platformGraphics, &blackTextureAsset);
	TextureHandle neutralBumpTexture 	= platformApi->push_texture(platformGraphics, &neutralBumpTextureAsset);
	TextureHandle waterTextureHandle	= platformApi->push_texture(platformGraphics, &waterBlueTextureAsset);
	TextureHandle seedTextureHandle		= platformApi->push_texture(platformGraphics, &seedBrownTextureAsset);

	auto push_material = [](PipelineHandle shader, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
	{
		MaterialAsset asset = make_material_asset(shader, allocate_array(*global_transientMemory, {a, b, c}));
		MaterialHandle handle = platformApi->push_material(platformGraphics, &asset);
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
			.character 		= push_material(characterPipeline, lavaTexture, neutralBumpTexture, blackTexture),
			.environment 	= push_material(normalPipeline, tilesTexture, neutralBumpTexture, blackTexture),
			.ground 		= push_material(terrainPipeline, groundTexture, neutralBumpTexture, blackTexture),
		};
		
		// Note(Leo): internet told us vulkan(or glsl) cubemap face order is as follows:
		// (+X,-X,+Y,-Y,+Z,-Z).
		StaticArray<TextureAsset,6> skyboxTextureAssets =
		{
			load_texture_asset("assets/textures/miramar_rt.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_lf.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_ft.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_bk.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_up.png", global_transientMemory),
			load_texture_asset("assets/textures/miramar_dn.png", global_transientMemory),
		};
		auto skyboxTexture = platformApi->push_cubemap(platformGraphics, &skyboxTextureAssets);

		auto skyMaterialAsset 	= make_material_asset(skyPipeline, allocate_array(*global_transientMemory, {skyboxTexture}));	
		materials.sky 			= platformApi->push_material(platformGraphics, &skyMaterialAsset);
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

		char const * filename 	= "assets/models/cube_head.glb";
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

			scene->characterAnimations[WALK] 	= load_animation_glb(*persistentMemory, gltfFile, "Walk");
			scene->characterAnimations[RUN] 	= load_animation_glb(*persistentMemory, gltfFile, "Run");
			scene->characterAnimations[IDLE] 	= load_animation_glb(*persistentMemory, gltfFile, "Idle");
			scene->characterAnimations[JUMP]	= load_animation_glb(*persistentMemory, gltfFile, "JumpUp");
			scene->characterAnimations[FALL]	= load_animation_glb(*persistentMemory, gltfFile, "JumpDown");
			scene->characterAnimations[CROUCH] 	= load_animation_glb(*persistentMemory, gltfFile, "Crouch");

			logSystem(1) << "Loading all 6 animations took: " << platformApi->elapsed_seconds(startTime, platformApi->current_time()) << " s";

			motor->animations[WALK] 	= &scene->characterAnimations[WALK];
			motor->animations[RUN] 		= &scene->characterAnimations[RUN];
			motor->animations[IDLE] 	= &scene->characterAnimations[IDLE];
			motor->animations[JUMP]		= &scene->characterAnimations[JUMP];
			motor->animations[FALL]		= &scene->characterAnimations[FALL];
			motor->animations[CROUCH] 	= &scene->characterAnimations[CROUCH];
		}

		auto startTime = platformApi->current_time();

		AnimatedSkeleton * cubeHeadSkeleton = scene->animatedSkeletons.push_return_pointer(load_skeleton_glb(*persistentMemory, gltfFile, "cube_head"));

		logSystem(1) << "Loading skeleton took: " << platformApi->elapsed_seconds(startTime, platformApi->current_time()) << " s";

		scene->skeletonAnimators.push({
			.skeleton 		= cubeHeadSkeleton,
			.animations 	= motor->animations,
			.weights 		= motor->animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		});

		// Note(Leo): take the reference here, so we can easily copy this down below.
		// auto & cubeHeadSkeleton = scene->skeletonAnimators.last().skeleton;

		auto model = push_model(girlMesh, materials.character);
		scene->animatedRenderers.push(make_animated_renderer(
				playerTransform,
				cubeHeadSkeleton,
				model,
				*persistentMemory
			));

		// --------------------------------------------------------------------

		for (s32 randomWalkerIndex = 0; randomWalkerIndex < array_count(scene->randomWalkers); ++randomWalkerIndex)
		{
			v3 position = {RandomRange(-99, 99), RandomRange(-99, 99), 0};
			f32 scale 	= {RandomRange(0.8f, 1.5f)};

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

			AnimatedSkeleton * skeleton = scene->animatedSkeletons.push_return_pointer({});
			skeleton->bones = copy_array(*persistentMemory, cubeHeadSkeleton->bones);

			scene->skeletonAnimators.push({
					.skeleton 		= skeleton,
					.animations 	= motor->animations,
					.weights 		= motor->animationWeights,
					.animationCount = CharacterAnimations::ANIMATION_COUNT
				});

			auto model = push_model(girlMesh, materials.character); 
			scene->animatedRenderers.push(make_animated_renderer(transform,	skeleton, model, *persistentMemory));
		}



		Transform3D * targetTransform = nullptr;
		s32 followersPerWalker = array_count(scene->followers) / array_count(scene->randomWalkers); 

		for (s32 followerIndex = 0; followerIndex < array_count(scene->followers); ++followerIndex)
		{
			v3 position 		= {RandomRange(-99, 99), RandomRange(-99, 99), 0};
			f32 scale 			= RandomRange(0.8f, 1.5f);
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

			AnimatedSkeleton * skeleton = scene->animatedSkeletons.push_return_pointer({});
			skeleton->bones = copy_array(*persistentMemory, cubeHeadSkeleton->bones);

			scene->skeletonAnimators.push({
					.skeleton 		= skeleton,
					.animations 	= motor->animations,
					.weights 		= motor->animationWeights,
					.animationCount = CharacterAnimations::ANIMATION_COUNT
				});

			auto model = push_model(girlMesh, materials.character); 
			scene->animatedRenderers.push(make_animated_renderer(transform,	skeleton, model, *persistentMemory));
		}
	}

	scene->worldCamera = make_camera(50, 0.1f, 1000.0f);
	scene->cameraController = make_camera_controller_3rd_person(&scene->worldCamera, scene->playerCharacterTransform);
	scene->cameraController.baseOffset = {0, 0, 1.5f};
	scene->cameraController.distance = 5;

	// Environment
	{
		constexpr f32 depth = 100;
		constexpr f32 width = 100;
		f32 mapSize = 200;
		f32 terrainHeight = 20;
		{
			// Note(Leo): this is maximum size we support with u16 mesh vertex indices
			s32 gridSize = 256;

			auto heightmapTexture 	= load_texture_asset("assets/textures/heightmap6.jpg", global_transientMemory);
			auto heightmap 			= make_heightmap(global_transientMemory, &heightmapTexture, gridSize, mapSize, -terrainHeight / 2, terrainHeight / 2);
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

			auto transform = scene->transforms.push_return_pointer({});
			transform->position.z = get_terrain_height(&scene->collisionSystem, *transform->position.xy()) - 0.5f;
			scene->renderers.push({transform, model});
			push_box_collider(	scene->collisionSystem,
								v3 {1.0, 1.0, 5.0},
								v3 {0,0,2},
								transform);

			transform = scene->transforms.push_return_pointer({.position = {0, 5, 0} , .scale = 0.5});
			transform->position.z = get_terrain_height(&scene->collisionSystem, *transform->position.xy()) - 0.25f;
			scene->renderers.push({transform, model});
			push_box_collider(	scene->collisionSystem,
								v3{0.5, 0.5, 2.5},
								v3{0,0,1},
								transform);
		}

		{
			s32 pillarCountPerDirection = mapSize / 10;
			s32 pillarCount = pillarCountPerDirection * pillarCountPerDirection;

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


				f32 x = ((pillarIndex % pillarCountPerDirection) / (f32)pillarCountPerDirection) * mapSize - mapSize / 2;
				f32 y = ((pillarIndex / pillarCountPerDirection) / (f32)pillarCountPerDirection) * mapSize - mapSize / 2;

				f32 z = get_terrain_height(&scene->collisionSystem, v2{x, y});

				v3 position = {x, y, z - 1};
				auto transform = scene->transforms.push_return_pointer({position});
				
				auto model = push_model(pillarMeshHandle, materials.environment);
				scene->renderers.push({transform, model});

				push_box_collider(scene->collisionSystem, v3{2,2,50}, v3{0,0,25}, transform);
			}
		}

		// Test robot
		{
			v3 position 			= {21, 10, 0};
			position.z 				= get_terrain_height(&scene->collisionSystem, *position.xy());
			auto * transform 		= scene->transforms.push_return_pointer({.position = position});

			char const * filename 	= "assets/models/Robot53.glb";
			auto meshAsset 			= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, filename), "model_rigged");	
			auto mesh 				= platformApi->push_mesh(platformGraphics, &meshAsset);

			auto albedo 			= load_and_push_texture("assets/textures/Robot_53_albedo_4k.png");
			auto material 			= push_material(normalPipeline, albedo, neutralBumpTexture, blackTexture);

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
				position.z 						= get_terrain_height(&scene->collisionSystem, *position.xy());
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

				transform->position.z 	= get_terrain_height(&scene->collisionSystem, *transform->position.xy());

				scene->renderers.push({transform, model});
				push_cylinder_collider(scene->collisionSystem, 0.6, 1, v3{0,0,0.5}, transform);
			}

			// ----------------------------------------------------------------	

			auto treeTextureAsset 	= load_texture_asset("assets/textures/tree.png", global_transientMemory);
			auto treeTexture 		= platformApi->push_texture(platformGraphics, &treeTextureAsset);			
			auto treeMaterial 		= push_material(normalPipeline, treeTexture, neutralBumpTexture, blackTexture);
			auto treeMeshAsset 		= load_mesh_glb(*global_transientMemory, sceneryFile, "tree");
			auto treeMesh 			= platformApi->push_mesh(platformGraphics, &treeMeshAsset);

			scene->treeModel 		= push_model(treeMesh, treeMaterial);
			
			scene->treesInPotCount = 0;
			scene->treesOnGroundCount = 0;

			// ----------------------------------------------------------------	

			MeshAsset waterDropAsset 	= load_mesh_glb(*global_transientMemory, sceneryFile, "water_drop");
			MeshHandle waterDropHandle 	= platformApi->push_mesh(platformGraphics, &waterDropAsset);

			MaterialAsset waterMaterialAsset =
			{
				.pipeline = normalPipeline,
				.textures = allocate_array(*global_transientMemory, {waterTextureHandle, neutralBumpTexture, blackTexture})
			};

			MaterialHandle waterDropMaterial = platformApi->push_material(platformGraphics, &	waterMaterialAsset);
			scene->waterModel = platformApi->push_model(platformGraphics, waterDropHandle, waterDropMaterial);

			scene->waterCount = scene->waterCapacity;
			for (s32 i = 0; i < scene->waterCount; ++i)
			{
				v3 position = {RandomRange(-50, 50), RandomRange(-50, 50)};
				position.z 	= get_terrain_height(&scene->collisionSystem, *position.xy());

				scene->waterTransforms[i] = { .position = position };
			}

			// ----------------------------------------------------------------

			for(s32 i = 0; i < seedCapacity; ++i)
			{
				scene->seedTransforms[i] = Transform3D::identity();

				v3 position = {RandomRange(-50, 50), RandomRange(-50, 50), 0};
				position.z = get_terrain_height(&scene->collisionSystem, *position.xy());
				scene->seedTransforms[i].position = position;
			}

			scene->currentSeedCount = scene->seedCapacity;

			MaterialAsset seedMaterialAsset = 
			{
				.pipeline = normalPipeline,
				.textures = allocate_array(*global_transientMemory, {seedTextureHandle, neutralBumpTexture, blackTexture})
			};
			auto seedMaterial 	= platformApi->push_material(platformGraphics, &seedMaterialAsset);
			scene->seedModel 	= platformApi->push_model(platformGraphics, waterDropHandle, seedMaterial);

			logDebug(0) << "Debug from empty lambda";
		}

		logSystem(0) << "Scene3d loaded, " << used_percent(*global_transientMemory) * 100 << "% of transient memory used.";
		logSystem(0) << "Scene3d loaded, " << used_percent(*persistentMemory) * 100 << "% of persistent memory used.";
	}
}
