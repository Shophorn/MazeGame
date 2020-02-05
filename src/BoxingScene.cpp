/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for boxing game
=============================================================================*/

// #include "DefaultSceneGui.cpp"

namespace boxing_scene
{
	struct Scene
	{
		ArenaArray<RenderSystemEntry> renderSystem;

		// Todo(Leo): make this similar 'system' to others
		CollisionManager2D collisionManager;

		Camera worldCamera;

		// CameraControllerSideScroller cameraController;
		CharacterControllerSideScroller characterController;

		SceneGui gui;
	};

	internal u64
	get_alloc_size() { return sizeof(Scene); };

	internal MenuResult
	update(	void * 						scenePtr,
			game::Input * 				input,
			platform::Graphics*,
			platform::Window*,
			platform::Functions*);

	internal void
	load(	void * 						scenePtr,
			MemoryArena * 				persistentMemory,
			MemoryArena * 				transientMemory,
			platform::Graphics*,
			platform::Window*,
			platform::Functions*);
}

global_variable SceneInfo 
boxingSceneInfo = make_scene_info(	boxing_scene::get_alloc_size,
									boxing_scene::load,
									boxing_scene::update);

internal MenuResult
boxing_scene::update(	void * scenePtr,
						game::Input * input,
						platform::Graphics * graphics,
						platform::Window * window,
						platform::Functions * functions)
{
	Scene * scene = reinterpret_cast<Scene*>(scenePtr);

	scene->collisionManager.do_collisions();

	/// Update Character
	scene->characterController.update(input, &scene->collisionManager);

    update_camera_system(&scene->worldCamera, input, graphics, window, functions);
	update_render_system(scene->renderSystem, graphics, functions);

	Light light = { vector::normalize(vector3{1, 1, -3}), {0.95, 0.95, 0.9}};
	float3 ambient = {0.2, 0.25, 0.4};
	functions->update_lighting(graphics, &light, &scene->worldCamera, ambient);


	auto result = update_scene_gui(&scene->gui, input, graphics, functions);
	return result;
}

internal void 
boxing_scene::load(	void * scenePtr,
					MemoryArena * persistentMemory,
					MemoryArena * transientMemory,
					platform::Graphics * graphics,
					platform::Window * platform,
					platform::Functions * functions)
{
	Scene * scene = reinterpret_cast<Scene*>(scenePtr);

	// Note(Leo): lol this is so much better than others :D
	scene->gui = make_scene_gui(transientMemory, graphics, functions);

	allocate_for_handle<Transform3D>(persistentMemory, 100);
	allocate_for_handle<Collider2D>(persistentMemory, 100);
	allocate_for_handle<Renderer>(persistentMemory, 100);

	scene->renderSystem = reserve_array<RenderSystemEntry>(persistentMemory, 100);

	scene->collisionManager =
	{
		.colliders 	= reserve_array<Handle<Collider2D>>(persistentMemory, 200),
		.collisions = reserve_array<Collision2D>(persistentMemory, 300) // Todo(Leo): Not enough..
	};

	struct MaterialCollection {
		MaterialHandle character;
		MaterialHandle environment;
	} materials;

	// Create MateriaLs
	{
		PipelineHandle shader = functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/frag.spv", {.textureCount = 3});

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
		auto lavaTexture 	= load_and_push_texture("assets/textures/lava.jpg");
		auto faceTexture 	= load_and_push_texture("assets/textures/texture.jpg");

		auto push_material = [shader, functions, transientMemory, graphics](MaterialType type, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
		{
			MaterialAsset asset = make_material_asset(shader, push_array(transientMemory, {a, b, c}));
			MaterialHandle handle = functions->push_material(graphics, &asset);
			return handle;
		};

		materials =
		{
			.character 	= push_material(	MaterialType::Character,
											lavaTexture,
											faceTexture,
											blackTexture),

			.environment = push_material(	MaterialType::Character,
											tilesTexture,
											blackTexture,
											blackTexture)
		};
	}

    auto push_model = [functions, graphics] (MeshHandle mesh, MaterialHandle material) -> ModelHandle
    {
    	auto handle = functions->push_model(graphics, mesh, material);
    	return handle;
    };

	// Characters
	Handle<Transform3D> characterTransform = {};
	{
		auto characterMesh 			= load_model_obj(transientMemory, "assets/models/character.obj");
		auto characterMeshHandle 	= functions->push_mesh(graphics, &characterMesh);

		// Our dude
		auto transform = make_handle<Transform3D>({0, 0, 1});
		auto renderer = make_handle<Renderer>({push_model(characterMeshHandle, materials.character)});

		characterTransform = transform;

		push_one(scene->renderSystem, {transform, renderer});

		scene->characterController 	= 
		{
			.transform 	= transform,
			.collider 	= push_collider(&scene->collisionManager, transform, {0.4f, 0.8f}, {0.0f, 0.5f}),
		};

		// Other dude
		transform 	= make_handle<Transform3D>({2, 0.5f, 12.25f});
		renderer 	= make_handle<Renderer>({push_model(characterMeshHandle, materials.character)});
		push_one(scene->renderSystem, {transform, renderer});
	}

    scene->worldCamera = make_camera(25, 0.1f, 1000.0f);
    scene->worldCamera.position = {0, -12.0f, 3.5f};
    scene->worldCamera.direction = vector::normalize((characterTransform->position + vector3{0, 2.5f, 0}) - scene->worldCamera.position);

    // scene->cameraController =
    // {
    // 	.camera 		= &scene->worldCamera,
    // 	.target 		= characterTransform,

    // 	.baseOffset = {0, 0, 1.5f},
    // 	.distance = 10,
    // };

	// Environment
	{
		constexpr float depth = 100;
		constexpr float width = 100;

		{
			auto groundQuad 	= mesh_primitives::create_quad(transientMemory, false);
			auto meshTransform	= matrix::multiply(	make_translation_matrix({-width / 2, -depth /2, 0}),
													make_scale_matrix({width, depth, 0}));
			mesh_ops::transform(&groundQuad, meshTransform);
			mesh_ops::transform_tex_coords(&groundQuad, {0,0}, {width / 2, depth / 2});

			auto groundQuadHandle 	= functions->push_mesh(graphics, &groundQuad);
			auto renderer 			= make_handle<Renderer>({push_model(groundQuadHandle, materials.environment)});
			auto transform 			= make_handle<Transform3D>({});

			push_one(scene->renderSystem, {transform, renderer});
			push_collider(&scene->collisionManager, transform, {100, 1}, {0, -1.0f});
		}

		{
			auto ringMesh 		= load_model_glb(transientMemory, "assets/models/boxing_ring.glb", "BoxingRing");
			auto ringMeshHandle = functions->push_mesh(graphics, &ringMesh);

			auto renderer = make_handle<Renderer>({push_model(ringMeshHandle, materials.environment)});
			auto transform = make_handle<Transform3D>({});

			push_one(scene->renderSystem, {transform, renderer}); 
			push_collider(&scene->collisionManager, transform, {2.5f, 0.5f}, {0, 0.5f});
			push_collider(&scene->collisionManager, transform, {0.1f, 20.0f}, {-2.55, 1.0f});
			push_collider(&scene->collisionManager, transform, {0.1f, 20.0f}, {2.55, 1.0f});
		}
	}
}
