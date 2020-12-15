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
	EditorCameraController 		editorCamera;
	f32 						cameraSelectPercent = 0;
	f32 						cameraTransitionDuration = 0.5;

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
	RaccoonMode *		raccoonModes;
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
	bool32 		guiVisible;

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