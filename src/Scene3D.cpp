/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for 3d development scene
=============================================================================*/
#include "TerrainGenerator.cpp"
#include "Collisions3D.cpp"
#include "CharacterController3rdPerson.cpp"

#include "DefaultSceneGui.cpp"

struct Scene3d
{
	ArenaArray<RenderSystemEntry> renderSystem = {};
	CollisionSystem3D collisionSystem = {};

	Camera worldCamera;
	CameraController3rdPerson cameraController;
	CharacterController3rdPerson characterController;

	// Todo(Leo): make animation state controller or similar for these
	AnimationClip 	laddersUpAnimation;
	AnimationClip 	laddersDownAnimation;

	// Todo(Leo): make controller for these
	CharacterControllerSideScroller::LadderTriggerFunc ladderTrigger1;
	CharacterControllerSideScroller::LadderTriggerFunc ladderTrigger2;
	bool32 ladderOn = false;
	bool32 ladder2On = false;

	ModelHandle skybox;

	SceneGui gui;

	// using ComponentTypes = TypeList<Transform3D,
	// 								BoxCollider3D,
	// 								Renderer>;

	// ComponentManager<ComponentTypes> componentManager;

	ArenaArray<Transform3D> 	transformStorage;
	ArenaArray<BoxCollider3D> 	colliderStorage;
	ArenaArray<Renderer> 		rendererStorage;

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


global_variable SceneInfo 
scene3dInfo = make_scene_info(	get_size_of<Scene3d>,
								Scene3d::load,
								Scene3d::update);

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
	functions->draw_model(graphics, scene->skybox, matrix::make_identity<Matrix44>(), false);

	// Game Logic section
	update_character(	&scene->characterController,
						input,
						&scene->worldCamera,
						&scene->collisionSystem,
						graphics,
						functions);
	update_camera_controller(&scene->cameraController, input);

	// Rendering section
    update_camera_system(&scene->worldCamera, input, graphics, window, functions);
	update_render_system(scene->renderSystem, graphics, functions);


	Light light = { vector::normalize(vector3{1, 1, -3}), {0.95, 0.95, 0.9}};
	float3 ambient = {0.2, 0.25, 0.4};
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
	// Todo(Leo): this kind of allocation to static variables prevents us from hot-reloading
	allocate_for_handle<Transform3D>	(persistentMemory, 100);
	allocate_for_handle<BoxCollider3D>	(persistentMemory, 100);
	allocate_for_handle<Renderer>		(persistentMemory, 100);

	scene->renderSystem = reserve_array<RenderSystemEntry>(persistentMemory, 100);
	scene->collisionSystem.colliders = reserve_array<CollisionSystemEntry>(persistentMemory, 100);

	struct MaterialCollection {
		MaterialHandle character;
		MaterialHandle environment;
		MaterialHandle ground;
		MaterialHandle sky;
	} materials;

	// Create MateriaLs
	{
		PipelineHandle normalPipeline 	= functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/frag.spv", {.textureCount = 3});
		PipelineHandle terrainPipeline 	= functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/terrain_frag.spv", {.textureCount = 3});
		PipelineHandle skyPipeline 		= functions->push_pipeline(graphics, "assets/shaders/vert_sky.spv", "assets/shaders/frag_sky.spv", {.enableDepth = false, .textureCount = 1});

		TextureAsset whiteTextureAsset = make_texture_asset(push_array<u32>(transientMemory, {0xffffffff}), 1, 1);
		TextureAsset blackTextureAsset = make_texture_asset(push_array<u32>(transientMemory, {0xff000000}), 1, 1);

		TextureHandle whiteTexture = functions->push_texture(graphics, &whiteTextureAsset);
		TextureHandle blackTexture = functions->push_texture(graphics, &blackTextureAsset);

		auto load_and_push_texture = [transientMemory, functions, graphics](const char * path) -> TextureHandle
		{
			auto asset = load_texture_asset(path, transientMemory);
			auto result = functions->push_texture(graphics, &asset);
			return result;
		};

		auto tilesTexture 	= load_and_push_texture("assets/textures/tiles.jpg");
		auto groundTexture 	= load_and_push_texture("assets/textures/ground.png");
		auto lavaTexture 	= load_and_push_texture("assets/textures/lava.jpg");
		auto faceTexture 	= load_and_push_texture("assets/textures/texture.jpg");

		auto push_material = [functions, transientMemory, graphics](PipelineHandle shader, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
		{
			MaterialAsset asset = make_material_asset(shader, push_array(transientMemory, {a, b, c}));
			MaterialHandle handle = functions->push_material(graphics, &asset);
			return handle;
		};

		materials =
		{
			.character 		= push_material(normalPipeline, lavaTexture, faceTexture, blackTexture),
			.environment 	= push_material(normalPipeline, tilesTexture, blackTexture, blackTexture),
			.ground 		= push_material(terrainPipeline, groundTexture, blackTexture, blackTexture),
		};

		
		// internet: (+X,-X,+Y,-Y,+Z,-Z).
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

		auto skyMaterialAsset 	= make_material_asset(skyPipeline, push_array(transientMemory, {skyboxTexture}));	
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
	Handle<Transform3D> characterTransform = {};
	{
		auto characterMesh 			= load_model_obj(transientMemory, "assets/models/character.obj");
		auto characterMeshHandle 	= functions->push_mesh(graphics, &characterMesh);

		// Our dude
		auto transform = make_handle<Transform3D>({0, 0, 5});
		auto renderer = make_handle<Renderer>({push_model(characterMeshHandle, materials.character)});

		characterTransform = transform;

		push_one(scene->renderSystem, {transform, renderer});

		scene->characterController = make_character(transform);

		// scene->characterController.OnTriggerLadder1 = &scene->ladderTrigger1;
		// scene->characterController.OnTriggerLadder2 = &scene->ladderTrigger2;

		// Other dude
		transform 	= make_handle<Transform3D>({2, 0.5f, 12.25f});
		renderer 	= make_handle<Renderer>({push_model(characterMeshHandle, materials.character)});
		push_one(scene->renderSystem, {transform, renderer});
	}

	scene->worldCamera = make_camera(50, 0.1f, 1000.0f);
	scene->cameraController = make_camera_controller_3rd_person(&scene->worldCamera, characterTransform);

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
			auto renderer 			= make_handle<Renderer>({push_model(groundMesh, materials.ground)});
			auto transform 			= make_handle<Transform3D>({{-50, -50, 0}});

			push_one(scene->renderSystem, {transform, renderer});

			scene->collisionSystem.terrainCollider = heightmap;
			scene->collisionSystem.terrainTransform = transform;
		}

		{
			auto pillarMesh 		= load_model_glb(transientMemory, "assets/models/big_pillar.glb", "big_pillar");
			auto pillarMeshHandle 	= functions->push_mesh(graphics, &pillarMesh);

			auto renderer 	= make_handle<Renderer> ({push_model(pillarMeshHandle, materials.environment)});
			auto transform 	= make_handle<Transform3D> ({-width / 4, 0, 0});
			auto collider 	= make_handle<BoxCollider3D> ({
				.extents 	= {2, 2, 50},
				.center 	= {0, 0, 25}
			});

			push_one(scene->renderSystem, {transform, renderer});
			push_collider_to_system(&scene->collisionSystem, collider, transform);

			renderer 	= make_handle<Renderer>({push_model(pillarMeshHandle, materials.environment)});
			transform 	= make_handle<Transform3D>({width / 4, 0, 0});
			collider 	= make_handle<BoxCollider3D>({
				.extents 	= {2, 2, 50},
				.center 	= {0, 0, 25}
			});

			push_one(scene->renderSystem, {transform, renderer});
			push_collider_to_system(&scene->collisionSystem, collider, transform);
		}

		{
			vector3 platformPositions [] =
			{
				{-6, 0, 6},
				{-4, 0, 6},
				{-2, 0, 6},
	
				{2, 0, 6},
				{4, 0, 6},
				{6, 0, 6},
				{8, 0, 6},
				{10, 0, 6},

				{2, 0, 12},
				{4, 0, 12},
				{6, 0, 12},
				{8, 0, 12},
			};

			auto platformMeshAsset 	= load_model_obj(transientMemory, "assets/models/platform.obj");
			auto platformMeshHandle = functions->push_mesh(graphics, &platformMeshAsset);

			int platformCount = 12;
			for (int platformIndex = 0; platformIndex < platformCount; ++platformIndex)
			{
				auto renderer 	= make_handle<Renderer>({push_model(platformMeshHandle, materials.environment)});
				auto transform 	= make_handle<Transform3D>({platformPositions[platformIndex]});

				push_one(scene->renderSystem, {transform, renderer});
			}
		}

		{
			auto keyholeMeshAsset 	= load_model_obj(transientMemory, "assets/models/keyhole.obj");
			auto keyholeMeshHandle 	= functions->push_mesh(graphics, &keyholeMeshAsset);

			auto renderer 	= make_handle<Renderer>({push_model(keyholeMeshHandle, materials.environment)});
			auto transform 	= make_handle<Transform3D>({vector3{-3, 0, 0}});
			auto collider 	= make_handle<BoxCollider3D>({
				.extents 	= {0.3f, 0.3f, 0.6f},
				.center 	= {0, 0, 0.3f}
			});

			push_one(scene->renderSystem, {transform, renderer});
			push_collider_to_system(&scene->collisionSystem, collider, transform);

			renderer 	= make_handle<Renderer>({push_model(keyholeMeshHandle, materials.environment)});
			transform 	= make_handle<Transform3D>({vector3{4, 0, 6}});
			collider 	= make_handle<BoxCollider3D>({
				.extents 	= {0.3f, 0.3f, 0.6f},
				.center 	= {0, 0, 0.3f}
			});
			push_one(scene->renderSystem, {transform, renderer});
			push_collider_to_system(&scene->collisionSystem, collider, transform);
		}
	}
}
