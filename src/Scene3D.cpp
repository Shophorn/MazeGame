/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for 3d development scene
=============================================================================*/
#include "TerrainGenerator.cpp"
#include "Collisions3D.cpp"
#include "CharacterController3rdPerson.cpp"

#include "DefaultSceneGui.cpp"

void update_animated_renderer(AnimatedRenderer & renderer, Skeleton const & skeleton)
{
	// Todo(Leo): Make more sensible structure that keeps track of this
	assert(skeleton.bones.count() == renderer.bones.count());

	u32 boneCount = skeleton.bones.count();

	/* Todo(Leo): Skeleton needs to be ordered so that parent ALWAYS comes before
	children, and therefore 0th is always implicitly root */

	m44 modelSpaceTransforms [20];

	for (int i = 0; i < boneCount; ++i)
	{
		/*
		1. get bone space matrix from bone's bone space transform
		2. transform bone space matrix to model space matrix
		*/

		m44 matrix = get_matrix(skeleton.bones[i].boneSpaceTransform);

		if (skeleton.bones[i].isRoot == false)
		{
			m44 parentMatrix = modelSpaceTransforms[skeleton.bones[i].parent];
			matrix = parentMatrix * matrix;
		}
		modelSpaceTransforms[i] = matrix;
		renderer.bones[i] = matrix * skeleton.bones[i].inverseBindMatrix;
	}
}

s32 previous_rotation_keyframe(BoneAnimation const & animation, float time)
{
	/* Note(Leo): In a looping animation, we have two possibilities to return 
	the last keyframe. First, if actual time is before the first actual keyframe,
	we then return last keyframe (from last loop round). The other is, if time is 
	more than that keyframes time, so that it was actually the last. */

	// Note(Leo): Last keyframe from previous loop round
	if (time < animation.rotations[0].time)
	{
		return animation.rotations.count() - 1;
	}

	for (int i = 0; i < animation.rotations.count() - 1; ++i)
	{
		float thisTime = animation.rotations[i].time;
		float nextTime = animation.rotations[i + 1].time;

		if (thisTime <= time && time <= nextTime)
		{
			return i;
		}
	}
	
	// Note(Leo): this loop rounds last keyframe 
	return animation.rotations.count() - 1;
}

s32 previous_position_keyframe(BoneAnimation const & animation, float time)
{
	/* Note(Leo): In a looping animation, we have two possibilities to return 
	the last keyframe. First, if actual time is before the first actual keyframe,
	we then return last keyframe (from last loop round). The other is, if time is 
	more than that keyframes time, so that it was actually the last. */

	// Note(Leo): Last keyframe from previous loop round
	if (time < animation.positions[0].time)
	{
		return animation.positions.count() - 1;
	}

	for (int i = 0; i < animation.positions.count() - 1; ++i)
	{
		float thisTime = animation.positions[i].time;
		float nextTime = animation.positions[i + 1].time;

		if (thisTime <= time && time <= nextTime)
		{
			return i;
		}
	}
	
	// Note(Leo): this loop rounds last keyframe 
	return animation.positions.count() - 1;
}

quaternion get_rotation(BoneAnimation const & animation, float duration, float time)
{
	u32 previousKeyframeIndex = previous_rotation_keyframe(animation, time);
	u32 nextKeyframeIndex = (previousKeyframeIndex + 1) % animation.rotations.count();

	// std::cout << "bone " << animation.boneIndex << ", " << previousKeyframeIndex << ", " << nextKeyframeIndex << "\n";

	// Get keyframes
	auto previousKeyframe 		= animation.rotations[previousKeyframeIndex];
	auto nextKeyframe 			= animation.rotations[nextKeyframeIndex];
	
	float timeToCurrent = time - previousKeyframe.time;
	if (timeToCurrent < 0.0f)
	{
		timeToCurrent = duration - timeToCurrent;
	} 

	float timeToNext = nextKeyframe.time - previousKeyframe.time;
	if (timeToNext < 0.0f)
	{
		timeToNext = duration - timeToNext;
	}

	float relativeTime = timeToCurrent / timeToNext;
	return interpolate(previousKeyframe.rotation, nextKeyframe.rotation, relativeTime); 
}

v3 get_position(BoneAnimation const & animation, float duration, float time)
{
	u32 previousKeyframeIndex = previous_position_keyframe(animation, time);
	u32 nextKeyframeIndex = (previousKeyframeIndex + 1) % animation.positions.count();

	// std::cout << "bone " << animation.boneIndex << ", " << previousKeyframeIndex << ", " << nextKeyframeIndex << "\n";

	// Get keyframes
	auto previousKeyframe 		= animation.positions[previousKeyframeIndex];
	auto nextKeyframe 			= animation.positions[nextKeyframeIndex];
	
	float timeToCurrent = time - previousKeyframe.time;
	if (timeToCurrent < 0.0f)
	{
		timeToCurrent = duration - timeToCurrent;
	} 

	float timeToNext = nextKeyframe.time - previousKeyframe.time;
	if (timeToNext < 0.0f)
	{
		timeToNext = duration - timeToNext;
	}

	float relativeTime = timeToCurrent / timeToNext;
	return vector::interpolate(previousKeyframe.position, nextKeyframe.position, relativeTime);
}

inline bool has_rotations(BoneAnimation const & boneAnimation)
{
	return boneAnimation.rotations.count() > 0;
}

inline bool has_positions(BoneAnimation const & boneAnimation)
{
	return boneAnimation.positions.count() > 0;
}


struct SkeletonAnimator
{
	Skeleton skeleton;

	float animationTime;
	Animation const * animation;
};

void update_skeleton_animator(SkeletonAnimator & animator, float elapsedTime)
{
	/*This procedure:
		- advances animation time
		- updates skeleton's bones
	*/
	if (animator.animation == nullptr)
		return;

	Animation const & animation = *animator.animation;
	float & animationTime = animator.animationTime;

	// ------------------------------------------------------------------------

	animationTime += elapsedTime;
	animationTime = modulo(animationTime, animator.animation->duration);

	for (auto & boneAnimation : animation.boneAnimations)
	{	
		auto & target = animator.skeleton.bones[boneAnimation.boneIndex].boneSpaceTransform;

		if (has_rotations(boneAnimation))
		{
			target.rotation = get_rotation(boneAnimation, animation.duration, animationTime);
		}

		if (has_positions(boneAnimation))
		{
			target.position = get_position(boneAnimation, animation.duration, animationTime);
		}
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

	SkeletonAnimator animator;
	Animation walkAnimation;

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

	update_skeleton_animator(scene->animator, input->elapsedTime);

	{
		m44 modelMatrix = get_matrix(*scene->characterController.transform);
		for (int i = 0; i < scene->animator.skeleton.bones.count(); ++i)
		{
			debug::draw_axes(	graphics,
								modelMatrix * get_model_space_transform(scene->animator.skeleton, i),
								0.2f,
								functions);
		}
	}

	update_animated_renderer(scene->animatedRenderers[0], scene->animator.skeleton);
	render_animated_models(scene->animatedRenderers, graphics, functions);

	// ------------------------------------------------------------------------

	Light light = { vector::normalize(v3{1, 1, -3}), {0.95, 0.95, 0.9}};
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

	// Create MateriaLs
	{
		PipelineHandle characterPipeline = functions->push_pipeline(graphics, "assets/shaders/animated_vert.spv", "assets/shaders/frag.spv", { .textureCount = 3});
		PipelineHandle normalPipeline 	= functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/frag.spv", {.textureCount = 3});
		PipelineHandle terrainPipeline 	= functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/terrain_frag.spv", {.textureCount = 3});
		PipelineHandle skyPipeline 		= functions->push_pipeline(graphics, "assets/shaders/vert_sky.spv", "assets/shaders/frag_sky.spv", {.enableDepth = false, .textureCount = 1});

		TextureAsset whiteTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xffffffff}), 1, 1);
		TextureAsset blackTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xff000000}), 1, 1);

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
			MaterialAsset asset = make_material_asset(shader, allocate_array(*transientMemory, {a, b, c}));
			MaterialHandle handle = functions->push_material(graphics, &asset);
			return handle;
		};

		materials =
		{
			.character 		= push_material(characterPipeline, lavaTexture, faceTexture, blackTexture),
			.environment 	= push_material(normalPipeline, tilesTexture, blackTexture, blackTexture),
			.ground 		= push_material(terrainPipeline, groundTexture, blackTexture, blackTexture),
		};
		
		// internet: (+X,-X,+Y,-Y,+Z,-Z).
		// Note(Leo): Another shitty comment, don't do this pretty please
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
	Transform3D * characterTransform = {};
	{
		char const * filename 	= "assets/models/cube_head.glb";
		auto gltfFile 			= read_gltf_file(*transientMemory, filename);

		scene->animator.skeleton = load_skeleton_glb(*persistentMemory, gltfFile, "cube_head");
		for(int i = 0; i < scene->animator.skeleton.bones.count(); ++i)
		{
			std::cout << "Bone " << i << ": " << scene->animator.skeleton.bones[i].name << ", " << scene->animator.skeleton.bones[i].boneSpaceTransform.rotation << "\n";
		}

#if 1
		scene->walkAnimation = load_animation_glb(*persistentMemory, gltfFile, "Walk");
#else
		auto find_bone_index = [](Skeleton const & skeleton, char const * name) -> s32
		{
			for (s32 i = 0; i < skeleton.bones.count(); ++i)
			{
				if (skeleton.bones[i].name == name)
				{
					return i;
				}
			}
			return -1;
		};

		scene->walkAnimation = {};
		scene->walkAnimation.boneAnimations = allocate_array<BoneAnimation>(*persistentMemory, 5);
		scene->walkAnimation.duration = 2.0f;

 		u32 headIndex 	= find_bone_index(scene->animator.skeleton, "Head");
		auto rotation0 	= quaternion::euler_angles({to_radians(20), 0, to_radians(45)});
		auto rotation1 	= quaternion::euler_angles({to_radians(20), 0, -to_radians(45)});
		quaternion rotation2 = quaternion::euler_angles({to_radians(-20), 0, 0});

		scene->walkAnimation.boneAnimations.push(
					{	.boneIndex = headIndex,
						.rotations = allocate_array<RotationKeyframe>(*persistentMemory, {{0.0f, rotation0}, {0.7f, rotation1}, {1.4f, rotation2}, {2.0f, rotation0}})
					});


		u32 leftThighIndex 	= find_bone_index(scene->animator.skeleton, "Thigh.L");
		u32 leftShinIndex 	= find_bone_index(scene->animator.skeleton, "Shin.L");
		u32 rightThighIndex = find_bone_index(scene->animator.skeleton, "Thigh.R");
		u32 rightShinIndex 	= find_bone_index(scene->animator.skeleton, "Shin.R");

		quaternion pose0 = quaternion::euler_angles(to_radians(135), 0, 0);
		quaternion pose1 = quaternion::euler_angles(to_radians(225), 0, 0);
		quaternion pose2 = quaternion::euler_angles(to_radians(-45), 0, 0);

		scene->walkAnimation.boneAnimations.push(
					{	.boneIndex = leftThighIndex,
						.rotations = allocate_array<RotationKeyframe>(*persistentMemory, {{0.0f, pose0}, {1.0f, pose1}, {2.0f, pose0}})
					});

		quaternion leftShinRestpose = scene->animator.skeleton.bones[leftShinIndex].boneSpaceTransform.rotation;
		scene->walkAnimation.boneAnimations.push(
					{	.boneIndex = leftShinIndex,
						.rotations = allocate_array<RotationKeyframe>(*persistentMemory, {{0.0f, leftShinRestpose}, {1.0f, pose2}, {2.0f, leftShinRestpose}})
					});

		scene->walkAnimation.boneAnimations.push(
					{ 	.boneIndex = rightThighIndex,
						.rotations = allocate_array<RotationKeyframe>(*persistentMemory, {{0.0f, pose1}, {1.0f, pose0},{2.0f, pose1}})
					});

		quaternion rightShinRestpose = scene->animator.skeleton.bones[rightShinIndex].boneSpaceTransform.rotation;
		scene->walkAnimation.boneAnimations.push(
					{	.boneIndex = rightShinIndex,
						.rotations = allocate_array<RotationKeyframe>(*persistentMemory, {{0.0f, pose2}, {1.0f, rightShinRestpose}, {2.0f, pose2}})
					});

#endif
		scene->animator.animation = &scene->walkAnimation;
		// ------------------------------------------------------------------------------------------------------------

		auto girlMeshAsset = load_model_glb(*transientMemory, gltfFile, "cube_head");

		auto girlMesh = functions->push_mesh(graphics, &girlMeshAsset);

		// Our dude
		auto transform = allocate_transform(scene->transformStorage, {10, 10, 5});
		auto model = push_model(girlMesh, materials.character);

		characterTransform = transform;

		auto bones = allocate_array<m44>(*persistentMemory, scene->animator.skeleton.bones.count(), ALLOC_FILL_UNINITIALIZED);
		scene->animatedRenderers.push({transform, model, true, std::move(bones)});

		scene->characterController = make_character(transform);
	}

	scene->worldCamera = make_camera(50, 0.1f, 1000.0f);
	scene->cameraController = make_camera_controller_3rd_person(&scene->worldCamera, characterTransform);
	scene->cameraController.baseOffset = {0, 0, 0.5f};
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
			auto totemMesh = load_model_glb(*transientMemory, read_gltf_file(*transientMemory, "assets/models/totem.glb"), "totem");
			// auto totemMesh = load_model_obj(transientMemory, "assets/models/totem.obj");
			auto totemMeshHandle = functions->push_mesh(graphics, &totemMesh);

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
