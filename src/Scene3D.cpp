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
	s32 						playerMotorIndex;
	Transform3D * 				playerCharacterTransform;

	// Other actors
	FollowerController followers[20];

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
					platform::Graphics * 	graphics,
					platform::Window *	 	window,
					platform::Functions * 	functions)
{
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);

	/* Sadly, we need to draw skybox before game logic, because otherwise
	debug lines would be hidden. This can be fixed though, just make debug pipeline similar to shadows. */ 
	functions->draw_model(graphics, scene->skybox, m44::identity(), false, nullptr, 0);

	/*
	1. update input
	2. update animation controller
	3. update animator
	4. update animated renderer
	5. draw
	
	*/



	// Game Logic section
	update_player_input(scene->playerMotorIndex, scene->characterInputs, scene->worldCamera, *input);
	
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

	  //////////////////////////////
	 /// 	RENDERING 			///
	//////////////////////////////
	/*
		1. Update camera and lights
		2. Render normal models
		3. Render animated models
	*/

    update_camera_system(&scene->worldCamera, input, graphics, window);

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

		platformApi->draw_model(graphics, renderer.model, get_matrix(*renderer.transform),
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
	scene->characterMotors		= allocate_array<CharacterMotor>(*persistentMemory, 600);
	scene->characterInputs		= allocate_array<CharacterInput>(*persistentMemory, scene->characterMotors.capacity(), ALLOC_FILL | ALLOC_NO_CLEAR);

	allocate_collision_system(&scene->collisionSystem, persistentMemory, 600);

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

	TextureAsset whiteTextureAsset = make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xffffffff}), 1, 1);
	TextureAsset blackTextureAsset = make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff000000}), 1, 1);
	TextureAsset neutralBumpTextureAsset = make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff8080ff}), 1, 1);

	TextureHandle whiteTexture 			= platformApi->push_texture(platformGraphics, &whiteTextureAsset);
	TextureHandle blackTexture 			= platformApi->push_texture(platformGraphics, &blackTextureAsset);
	TextureHandle neutralBumpTexture 	= platformApi->push_texture(platformGraphics, &neutralBumpTextureAsset);

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
		char const * filename 	= "assets/models/cube_head.glb";
		auto const gltfFile 	= read_gltf_file(*global_transientMemory, filename);

		auto girlMeshAsset 	= load_mesh_glb(*global_transientMemory, gltfFile, "cube_head");
		auto girlMesh 		= platformApi->push_mesh(platformGraphics, &girlMeshAsset);

		// --------------------------------------------------------------------

		Transform3D * playerTransform = scene->transforms.push_return_pointer({10, 10, 5});
		scene->playerCharacterTransform = playerTransform;

		scene->playerMotorIndex = scene->characterMotors.count();
		auto * motor = scene->characterMotors.push_return_pointer();
		motor->transform = playerTransform;

		{
			using namespace CharacterAnimations;			

			scene->characterAnimations[WALK] 	= load_animation_glb(*persistentMemory, gltfFile, "Walk");
			scene->characterAnimations[RUN] 	= load_animation_glb(*persistentMemory, gltfFile, "Run");
			scene->characterAnimations[IDLE] 	= load_animation_glb(*persistentMemory, gltfFile, "Idle");
			scene->characterAnimations[JUMP]	= load_animation_glb(*persistentMemory, gltfFile, "JumpUp");
			scene->characterAnimations[FALL]	= load_animation_glb(*persistentMemory, gltfFile, "JumpDown");
			scene->characterAnimations[CROUCH] 	= load_animation_glb(*persistentMemory, gltfFile, "Crouch");

			motor->animations[WALK] 	= &scene->characterAnimations[WALK];
			motor->animations[RUN] 		= &scene->characterAnimations[RUN];
			motor->animations[IDLE] 	= &scene->characterAnimations[IDLE];
			motor->animations[JUMP]		= &scene->characterAnimations[JUMP];
			motor->animations[FALL]		= &scene->characterAnimations[FALL];
			motor->animations[CROUCH] 	= &scene->characterAnimations[CROUCH];
		}

		AnimatedSkeleton * cubeHeadSkeleton = scene->animatedSkeletons.push_return_pointer(load_skeleton_glb(*persistentMemory, gltfFile, "cube_head"));

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

		Transform3D * targetTransform = playerTransform;

		for (s32 followerIndex = 0; followerIndex < array_count(scene->followers); ++followerIndex)
		{
			v3 position 		= {RandomRange(-99, 99), RandomRange(-99, 99), 0};
			f32 scale 			= RandomRange(0.8f, 1.5f);
			auto * transform 	= scene->transforms.push_return_pointer({.position = position, .scale = scale});

			s32 motorIndex 		= scene->characterMotors.count();
			auto * motor 		= scene->characterMotors.push_return_pointer();
			motor->transform 	= transform;

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
			push_collider_to_system(&scene->collisionSystem,
									BoxCollider3D{ .extents = {1.0, 1.0, 5.0}, .center = {0,0,2} },
									transform);

			transform = scene->transforms.push_return_pointer({.position = {0, 5, 0} , .scale = 0.5});
			transform->position.z = get_terrain_height(&scene->collisionSystem, *transform->position.xy()) - 0.25f;
			scene->renderers.push({transform, model});
			push_collider_to_system(&scene->collisionSystem,
									BoxCollider3D{ .extents = {0.5, 0.5, 2.5}, .center = {0,0,1} },
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

				auto collider  = BoxCollider3D{ .extents = {2, 2, 50}, .center = {0, 0, 25}};
				push_collider_to_system(&scene->collisionSystem, collider, transform);
			}
		}

		// Test robot
		{
			v3 position 			= {21, 10, 0};
			position.z 				= get_terrain_height(&scene->collisionSystem, *position.xy());
			auto * transform = scene->transforms.push_return_pointer({.position = position});

			char const * filename 	= "assets/models/Robot53.glb";
			auto meshAsset 			= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, filename), "model_rigged");	
			auto mesh 				= platformApi->push_mesh(platformGraphics, &meshAsset);

			auto albedo 			= load_and_push_texture("assets/textures/Robot_53_albedo_4k.png");
			auto material 			= push_material(normalPipeline, albedo, neutralBumpTexture, blackTexture);

			auto model 				= push_model(mesh, material);
			scene->renderers.push({transform, model});
		}

		logConsole() << FILE_ADDRESS << used_percent(*persistentMemory) * 100 <<  "% of persistent memory used.";
		logSystem() << "Scene3d loaded, " << used_percent(*persistentMemory) * 100 << "% of persistent memory used.";
	}
}
