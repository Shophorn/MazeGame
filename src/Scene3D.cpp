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

void update_animated_renderer(AnimatedRenderer & renderer)
{
	AnimatedSkeleton const & skeleton = *renderer.skeleton; 
	u32 boneCount = skeleton.bones.count();

	/* Todo(Leo): Skeleton.bones needs to be ordered so that parent ALWAYS comes before
	children, and therefore 0th is always implicitly root */

	// Todo(Leo): Use global value from vulkan layer. There must be room for maximum number of bones here. Currently it is 32, but to make sure we put 64. 
	m44 modelSpaceTransforms [64];

	for (int i = 0; i < boneCount; ++i)
	{
		/*
		1. get bone space matrix from bone's bone space transform
		2. transform bone space matrix to model space matrix
		*/

		m44 matrix = get_matrix(skeleton.bones[i].boneSpaceTransform);

		if (skeleton.bones[i].is_root() == false)
		{
			m44 parentMatrix = modelSpaceTransforms[skeleton.bones[i].parent];
			matrix = parentMatrix * matrix;
		}
		modelSpaceTransforms[i] = matrix;
		renderer.bones[i] = matrix * skeleton.bones[i].inverseBindMatrix;
	}
}


struct Scene3d
{
	// Components
	Array<Transform3D> 			transformStorage;
	Array<SkeletonAnimator> 	skeletonAnimators;	
	Array<CharacterMotor>		characterMotors;
	Array<CharacterInput>	characterMotorInputs;

	Array<RenderSystemEntry> 	renderSystem;
	Array<AnimatedRenderer> 	animatedRenderers;
	CollisionSystem3D 			collisionSystem;

	// Player
	Camera 						worldCamera;
	CameraController3rdPerson 	cameraController;
	s32 						playerMotorIndex;
	Transform3D * 				playerCharacterTransform;

	// Other actors
	FollowerController followers[200];

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

	// Game Logic section
	update_player_input(scene->playerMotorIndex, scene->characterMotorInputs, scene->worldCamera, *input);
	
	for (auto & follower : scene->followers)
	{
		update_follower_input(follower, scene->characterMotorInputs, input->elapsedTime);
	}
	update_camera_controller(&scene->cameraController, input);

	for (int i = 0; i < scene->characterMotors.count(); ++i)
	{
		update_character_motor(	scene->characterMotors[i],
								scene->characterMotorInputs[i],
								scene->collisionSystem,
								input->elapsedTime);
	}


	for (auto & animator : scene->skeletonAnimators)
	{
		update_skeleton_animator(animator, input->elapsedTime);
	}

	// Rendering section
    update_camera_system(&scene->worldCamera, input, graphics, window);
	update_render_system(scene->renderSystem, graphics);


	// update_skeleton_animator(character.animator, input->elapsedTime);

	// {
	// 	m44 modelMatrix = get_matrix(*scene->characterController.transform);
	// 	for (int i = 0; i < character.animator.skeleton.bones.count(); ++i)
	// 	{
	// 		debug::draw_axes(	graphics,
	// 							modelMatrix * get_model_space_transform(character.animator.skeleton, i),
	// 							0.2f,
	// 							functions);
	// 	}
	// }

	// for (auto & renderer : scene->animatedRenderers)
	// {
	// }
	// // render_animated_models(scene->animatedRenderers, graphics);
	for (auto & renderer : scene->animatedRenderers)
	{
		update_animated_renderer(renderer);

		platformApi->draw_model(graphics, renderer.model, get_matrix(*renderer.transform),
								renderer.castShadows, renderer.bones.data(),
								renderer.bones.count());
	
	}

	// ------------------------------------------------------------------------

	Light light = { v3{1, 1.2, -3}.normalized(), {0.95, 0.95, 0.9}};
	v3 ambient 	= {0.2, 0.25, 0.4};
	functions->update_lighting(graphics, &light, &scene->worldCamera, ambient);

	auto result = update_scene_gui(&scene->gui, input, graphics, functions);
	return result;
}

void 
Scene3d::load(	void * 						scenePtr, 
				MemoryArena * 				persistentMemory,
				MemoryArena * 				transientMemory,
				platform::Graphics *		graphics,
				platform::Window * 			window,
				platform::Functions *		functions)
{
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);
	
	// Note(Leo): This is good, more this.
	scene->gui = make_scene_gui(transientMemory, graphics, functions);

	// Note(Leo): amounts are SWAG, rethink.
	scene->transformStorage 	= allocate_array<Transform3D>(*persistentMemory, 1200);
	scene->skeletonAnimators 	= allocate_array<SkeletonAnimator>(*persistentMemory, 600);
	scene->animatedRenderers 	= allocate_array<AnimatedRenderer>(*persistentMemory, 600);
	scene->renderSystem 		= allocate_array<RenderSystemEntry>(*persistentMemory, 600);
	scene->characterMotors		= allocate_array<CharacterMotor>(*persistentMemory, 600);
	scene->characterMotorInputs	= allocate_array<CharacterInput>(*persistentMemory, scene->characterMotors.capacity(), ALLOC_FILL | ALLOC_NO_CLEAR);

	allocate_collision_system(&scene->collisionSystem, persistentMemory, 600);


	struct MaterialCollection {
		MaterialHandle character;
		MaterialHandle environment;
		MaterialHandle ground;
		MaterialHandle sky;
	} materials;

	PipelineHandle characterPipeline = functions->push_pipeline(graphics, "assets/shaders/animated_vert.spv", "assets/shaders/frag.spv", { .textureCount = 3});
	PipelineHandle normalPipeline 	= functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/frag.spv", {.textureCount = 3});
	PipelineHandle terrainPipeline 	= functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/terrain_frag.spv", {.textureCount = 3});
	PipelineHandle skyPipeline 		= functions->push_pipeline(graphics, "assets/shaders/vert_sky.spv", "assets/shaders/frag_sky.spv", {.textureCount = 1, .enableDepth = false});

	auto load_and_push_texture = [transientMemory, functions, graphics](const char * path) -> TextureHandle
	{
		auto asset = load_texture_asset(path, transientMemory);
		auto result = functions->push_texture(graphics, &asset);
		return result;
	};

	TextureAsset whiteTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xffffffff}), 1, 1);
	TextureAsset blackTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xff000000}), 1, 1);
	TextureAsset neutralBumpTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xff8080ff}), 1, 1);

	TextureHandle whiteTexture 			= functions->push_texture(graphics, &whiteTextureAsset);
	TextureHandle blackTexture 			= functions->push_texture(graphics, &blackTextureAsset);
	TextureHandle neutralBumpTexture 	= functions->push_texture(graphics, &neutralBumpTextureAsset);

	auto push_material = [functions, transientMemory, graphics](PipelineHandle shader, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
	{
		MaterialAsset asset = make_material_asset(shader, allocate_array(*transientMemory, {a, b, c}));
		MaterialHandle handle = functions->push_material(graphics, &asset);
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
			load_texture_asset("assets/textures/miramar_rt.png", transientMemory),
			load_texture_asset("assets/textures/miramar_lf.png", transientMemory),
			load_texture_asset("assets/textures/miramar_ft.png", transientMemory),
			load_texture_asset("assets/textures/miramar_bk.png", transientMemory),
			load_texture_asset("assets/textures/miramar_up.png", transientMemory),
			load_texture_asset("assets/textures/miramar_dn.png", transientMemory),
		};
		auto skyboxTexture = functions->push_cubemap(graphics, &skyboxTextureAssets);

		auto skyMaterialAsset 	= make_material_asset(skyPipeline, allocate_array(*transientMemory, {skyboxTexture}));	
		materials.sky 			= functions->push_material(graphics, &skyMaterialAsset);
	}

    auto push_model = [functions, graphics] (MeshHandle mesh, MaterialHandle material) -> ModelHandle
    {
    	auto handle = functions->push_model(graphics, mesh, material);
    	return handle;
    };

	// Skybox
    {
    	auto meshAsset 	= create_skybox_mesh(transientMemory);
    	auto meshHandle = functions->push_mesh(graphics, &meshAsset);
    	scene->skybox 	= push_model(meshHandle, materials.sky);
    }

	// Characters
	{
		char const * filename 	= "assets/models/cube_head.glb";
		auto const gltfFile 	= read_gltf_file(*transientMemory, filename);

		auto girlMeshAsset 	= load_mesh_glb(*transientMemory, gltfFile, "cube_head");
		auto girlMesh 		= platformApi->push_mesh(platformGraphics, &girlMeshAsset);

		// --------------------------------------------------------------------

		Transform3D * playerTransform = scene->transformStorage.push_return_pointer({10, 10, 5});
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


		scene->skeletonAnimators.push({
			.skeleton 		= load_skeleton_glb(*persistentMemory, gltfFile, "cube_head"),
			.animations 	= motor->animations,
			.weights 		= motor->animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		});

		// Note(Leo): take the reference here, so we can easily copy this down below.
		auto & cubeHeadSkeleton = scene->skeletonAnimators.last().skeleton;

		auto model = push_model(girlMesh, materials.character);
		scene->animatedRenderers.push(make_animated_renderer(
				playerTransform,
				&scene->skeletonAnimators.last().skeleton,
				model,
				*persistentMemory
			));

		// --------------------------------------------------------------------

		Transform3D * targetTransform = playerTransform;

		for (s32 followerIndex = 0; followerIndex < array_count(scene->followers); ++followerIndex)
		{
			v3 position 		= {RandomRange(-99, 99), RandomRange(-99, 99), 0};
			f32 scale 			= RandomRange(0.8f, 1.5f);
			auto * transform 	= scene->transformStorage.push_return_pointer({.position = position, .scale = scale});

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

			scene->skeletonAnimators.push({
					.skeleton 		= { .bones = copy_array(*persistentMemory, cubeHeadSkeleton.bones) },
					.animations 	= motor->animations,
					.weights 		= motor->animationWeights,
					.animationCount = CharacterAnimations::ANIMATION_COUNT
				});

			auto model = push_model(girlMesh, materials.character); 
			scene->animatedRenderers.push(make_animated_renderer(
					transform,
					&scene->skeletonAnimators.last().skeleton,
					model,
					*persistentMemory
				));
		}

	}

	// Test robot
	{
		auto transform = scene->transformStorage.push_return_pointer({21, 10, 1.2});

		char const * filename = "assets/models/Robot53.glb";
		auto meshAsset = load_mesh_glb(*transientMemory, read_gltf_file(*transientMemory, filename), "model_rigged");	
		auto mesh = functions->push_mesh(graphics, &meshAsset);

		auto albedo = load_and_push_texture("assets/textures/Robot_53_albedo_4k.png");
		auto material = push_material(normalPipeline, albedo, neutralBumpTexture, blackTexture);

		auto model = push_model(mesh, material);
		scene->renderSystem.push({transform, model});
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

			auto heightmapTexture 	= load_texture_asset("assets/textures/heightmap6.jpg", transientMemory);
			auto heightmap 			= make_heightmap(transientMemory, &heightmapTexture, gridSize, mapSize, -terrainHeight / 2, terrainHeight / 2);
			auto groundMeshAsset 	= generate_terrain(transientMemory, 32, &heightmap);

			auto groundMesh 		= functions->push_mesh(graphics, &groundMeshAsset);
			auto model 				= push_model(groundMesh, materials.ground);
			auto transform 			= scene->transformStorage.push_return_pointer({{-mapSize / 2, -mapSize / 2, 0}});

			scene->renderSystem.push({transform, model});

			scene->collisionSystem.terrainCollider = std::move(heightmap);
			scene->collisionSystem.terrainTransform = transform;
		}

		{
			auto totemMesh 			= load_mesh_glb(*transientMemory, read_gltf_file(*transientMemory, "assets/models/totem.glb"), "totem");
			auto totemMeshHandle 	= functions->push_mesh(graphics, &totemMesh);

			auto model = push_model(totemMeshHandle, materials.environment);
			auto transform = scene->transformStorage.push_return_pointer({0,0,-2});
			scene->renderSystem.push({transform, model});

			transform = scene->transformStorage.push_return_pointer({0, 5, -4, 0.5f});
			scene->renderSystem.push({transform, model});
		}

		{
			s32 pillarCountPerDirection = mapSize / 10;
			s32 pillarCount = pillarCountPerDirection * pillarCountPerDirection;

			auto pillarMesh 		= load_mesh_glb(*transientMemory, read_gltf_file(*transientMemory, "assets/models/big_pillar.glb"), "big_pillar");
			auto pillarMeshHandle 	= functions->push_mesh(graphics, &pillarMesh);

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

				// if ((y % 4) != 0)
				// {
				// 	continue;
				// }

				// s32 

				// x = 5;
				// y = 5;
				f32 z = get_terrain_height(&scene->collisionSystem, v2{x, y});

				v3 position = {x, y, z - 1};
				auto transform = scene->transformStorage.push_return_pointer({position});
				
				auto model = push_model(pillarMeshHandle, materials.environment);
				scene->renderSystem.push({transform, model});

				auto collider  = BoxCollider3D{ .extents = {2, 2, 50}, .center = {0, 0, 25}};
				push_collider_to_system(&scene->collisionSystem, collider, transform);
			}

			// auto model 		= push_model(pillarMeshHandle, materials.environment);
			// auto transform 	= scene->transformStorage.push_return_pointer({-width / 4, 0, -5});
			// scene->renderSystem.push({transform, model});
			
			// auto collider 	= BoxCollider3D {
			// 	.extents 	= {2, 2, 50},
			// 	.center 	= {0, 0, 25}
			// };
			// push_collider_to_system(&scene->collisionSystem, collider, transform);

			// model 	= push_model(pillarMeshHandle, materials.environment);
			// transform 	= scene->transformStorage.push_return_pointer({width / 4, 0, -6});
			// scene->renderSystem.push({transform, model});

			// collider 	= BoxCollider3D {
			// 	.extents 	= {2, 2, 50},
			// 	.center 	= {0, 0, 25}
			// };
			// push_collider_to_system(&scene->collisionSystem, collider, transform);
		}

		logConsole() << FILE_ADDRESS << used_percent(*persistentMemory) * 100 <<  "% of persistent memory used.";
		logSystem() << "Scene3d loaded, " << used_percent(*persistentMemory) * 100 << "% of persistent memory used.";
	}
}
