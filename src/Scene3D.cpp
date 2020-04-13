/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for 3d development scene
=============================================================================*/
#include "TerrainGenerator.cpp"
#include "Collisions3D.cpp"
#include "CharacterController3rdPerson.cpp"

#include "DefaultSceneGui.cpp"

void update_animated_renderer(AnimatedRenderer & renderer, Array<Bone> const & skeleton)
{
	// Todo(Leo): Make more sensible structure that keeps track of this
	assert(skeleton.count() == renderer.bones.count());

	u32 boneCount = skeleton.count();

	/* Todo(Leo): Skeleton needs to be ordered so that parent ALWAYS comes before
	children, and therefore 0th is always implicitly root */

	m44 modelSpaceTransforms [50];

	for (int i = 0; i < boneCount; ++i)
	{
		/*
		1. get bone space matrix from bone's bone space transform
		2. transform bone space matrix to model space matrix
		*/

		m44 matrix = get_matrix(skeleton[i].boneSpaceTransform);

		if (skeleton[i].is_root() == false)
		{
			m44 parentMatrix = modelSpaceTransforms[skeleton[i].parent];
			matrix = parentMatrix * matrix;
		}
		modelSpaceTransforms[i] = matrix;
		renderer.bones[i] = matrix * skeleton[i].inverseBindMatrix;
	}
}


struct Scene3d
{
	Array<Transform3D> transformStorage;
	
	Array<RenderSystemEntry> renderSystem = {};
	Array<AnimatedRenderer> animatedRenderers = {};
	CollisionSystem3D collisionSystem = {};

	Camera worldCamera;
	CameraController3rdPerson cameraController;
	CharacterController3rdPerson characterController;

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
	update_character(	scene->characterController,
						input,
						&scene->worldCamera,
						&scene->collisionSystem,
						graphics,
						functions);
	update_camera_controller(&scene->cameraController, input);

	// Rendering section
    update_camera_system(&scene->worldCamera, input, graphics, window, functions);
	update_render_system(scene->renderSystem, graphics, functions);


	auto & character = scene->characterController;
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

	update_animated_renderer(scene->animatedRenderers[0], character.skeleton.bones);
	render_animated_models(scene->animatedRenderers, graphics, functions);

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
	scene->transformStorage 	= allocate_array<Transform3D>(*persistentMemory, 100);
	scene->animatedRenderers 	= allocate_array<AnimatedRenderer>(*persistentMemory, 10);
	scene->renderSystem 		= allocate_array<RenderSystemEntry>(*persistentMemory, 100);

	allocate_collision_system(&scene->collisionSystem, persistentMemory, 100);

	auto allocate_transform = [](Array<Transform3D> & storage, Transform3D value) -> Transform3D *
	{
		u64 index = storage.count();
		storage.push(value);

		return &storage[index];
	};

	struct MaterialCollection {
		MaterialHandle character;
		MaterialHandle environment;
		MaterialHandle ground;
		MaterialHandle sky;
	} materials;

	PipelineHandle characterPipeline = functions->push_pipeline(graphics, "assets/shaders/animated_vert.spv", "assets/shaders/frag.spv", { .textureCount = 3});
	PipelineHandle normalPipeline 	= functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/frag.spv", {.textureCount = 3});
	PipelineHandle terrainPipeline 	= functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/terrain_frag.spv", {.textureCount = 3});
	PipelineHandle skyPipeline 		= functions->push_pipeline(graphics, "assets/shaders/vert_sky.spv", "assets/shaders/frag_sky.spv", {.enableDepth = false, .textureCount = 1});

	auto load_and_push_texture = [transientMemory, functions, graphics](const char * path) -> TextureHandle
	{
		auto asset = load_texture_asset(path, transientMemory);
		auto result = functions->push_texture(graphics, &asset);
		return result;
	};

	TextureAsset whiteTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xffffffff}), 1, 1);
	TextureAsset blackTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xff000000}), 1, 1);

	TextureHandle whiteTexture = functions->push_texture(graphics, &whiteTextureAsset);
	TextureHandle blackTexture = functions->push_texture(graphics, &blackTextureAsset);

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
			.character 		= push_material(characterPipeline, lavaTexture, faceTexture, blackTexture),
			.environment 	= push_material(normalPipeline, tilesTexture, blackTexture, blackTexture),
			.ground 		= push_material(terrainPipeline, groundTexture, blackTexture, blackTexture),
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
		auto & character = scene->characterController;

		auto transform 				= allocate_transform(scene->transformStorage, {10, 10, 5});
		character 					= make_character(transform);

		char const * filename 		= "assets/models/cube_head.glb";
		auto gltfFile 				= read_gltf_file(*transientMemory, filename);

		character.skeleton.bones 	= load_skeleton_glb(*persistentMemory, gltfFile, "cube_head");

		auto log = logDebug(0);
		log << "Bones in skeleton:\n";
		for (auto & bone : character.skeleton.bones)
		{
			log << "\t" << bone.name << "\n";
		}
		{
			using namespace CharacterAnimations;			

			character.animations[WALK] 		= load_animation_glb(*persistentMemory, gltfFile, "Walk");
			character.animations[RUN] 		= load_animation_glb(*persistentMemory, gltfFile, "Run");
			character.animations[IDLE] 		= load_animation_glb(*persistentMemory, gltfFile, "Idle");
			character.animations[JUMP]		= load_animation_glb(*persistentMemory, gltfFile, "JumpUp");
			character.animations[FALL]		= load_animation_glb(*persistentMemory, gltfFile, "JumpDown");
			character.animations[CROUCH] 	= load_animation_glb(*persistentMemory, gltfFile, "Crouch");
		}

		auto girlMeshAsset 			= load_model_glb(*transientMemory, gltfFile, "cube_head");
		auto girlMesh 				= functions->push_mesh(graphics, &girlMeshAsset);
		auto model 					= push_model(girlMesh, materials.character);

		auto bones = allocate_array<m44>(*persistentMemory, character.skeleton.bones.count(), ALLOC_FILL_UNINITIALIZED);
		scene->animatedRenderers.push({transform, model, true, std::move(bones)});
	}

	// Test robot
	{
		auto transform = allocate_transform(scene->transformStorage, {21, 10, 1.2});

		char const * filename = "assets/models/Robot53.glb";
		auto meshAsset = load_model_glb(*transientMemory, read_gltf_file(*transientMemory, filename), "model_rigged");	
		auto mesh = functions->push_mesh(graphics, &meshAsset);

		auto albedo = load_and_push_texture("assets/textures/Robot_53_albedo_4k.png");
		auto material = push_material(normalPipeline, albedo, blackTexture, blackTexture);

		auto model = push_model(mesh, material);
		scene->renderSystem.push({transform, model});
	}

	scene->worldCamera = make_camera(50, 0.1f, 1000.0f);
	scene->cameraController = make_camera_controller_3rd_person(&scene->worldCamera, scene->characterController.transform);
	scene->cameraController.baseOffset = {0, 0, 1.5f};
	scene->cameraController.distance = 5;

	// Environment
	{
		constexpr float depth = 100;
		constexpr float width = 100;

		{
			// Note(Leo): this is maximum size we support with u16 mesh vertex indices
			s32 gridSize = 256;
			float mapSize = 400;

			auto heightmapTexture 	= load_texture_asset("assets/textures/heightmap6.jpg", transientMemory);
			auto heightmap 			= make_heightmap(transientMemory, &heightmapTexture, gridSize, mapSize, -20, 20);
			auto groundMeshAsset 	= generate_terrain(transientMemory, 32, &heightmap);

			auto groundMesh 		= functions->push_mesh(graphics, &groundMeshAsset);
			auto model 				= push_model(groundMesh, materials.ground);
			auto transform 			= allocate_transform(scene->transformStorage, {{-50, -50, 0}});

			scene->renderSystem.push({transform, model});

			scene->collisionSystem.terrainCollider = std::move(heightmap);
			scene->collisionSystem.terrainTransform = transform;
		}

		{
			auto totemMesh 			= load_model_glb(*transientMemory, read_gltf_file(*transientMemory, "assets/models/totem.glb"), "totem");
			auto totemMeshHandle 	= functions->push_mesh(graphics, &totemMesh);

			auto model = push_model(totemMeshHandle, materials.environment);
			auto transform = allocate_transform(scene->transformStorage, {0,0,-2});
			scene->renderSystem.push({transform, model});

			transform = allocate_transform(scene->transformStorage, {0, 5, -4, 0.5f});
			scene->renderSystem.push({transform, model});
		}

		{
			auto pillarMesh 		= load_model_glb(*transientMemory, read_gltf_file(*transientMemory, "assets/models/big_pillar.glb"), "big_pillar");
			auto pillarMeshHandle 	= functions->push_mesh(graphics, &pillarMesh);

			auto model 		= push_model(pillarMeshHandle, materials.environment);
			auto transform 	= allocate_transform(scene->transformStorage, {-width / 4, 0, -5});
			scene->renderSystem.push({transform, model});
			
			auto collider 	= BoxCollider3D {
				.extents 	= {2, 2, 50},
				.center 	= {0, 0, 25}
			};
			push_collider_to_system(&scene->collisionSystem, collider, transform);

			model 	= push_model(pillarMeshHandle, materials.environment);
			transform 	= allocate_transform(scene->transformStorage, {width / 4, 0, -6});
			scene->renderSystem.push({transform, model});

			collider 	= BoxCollider3D {
				.extents 	= {2, 2, 50},
				.center 	= {0, 0, 25}
			};
			push_collider_to_system(&scene->collisionSystem, collider, transform);
		}
	}
}
