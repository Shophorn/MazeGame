struct Raccoons
{
	s32 				count;
	RaccoonMode *		modes;
	Transform3D * 		transforms;
	v3 *				targetPositions;
	CharacterMotor * 	characterMotors;

	MeshHandle 			mesh;
	MaterialHandle 		material;
};

internal void raccoons_update(Raccoons & raccoons, Pots & smallPots, Game * game, EntityReference playerCarriedEntity, f32 elapsedTime)
{
	// Note(Leo): +1 for player
	s32 maxCarriedRaccoons = smallPots.count + 1;
	Array<s32> carriedRaccoonIndices = push_array<s32>(*global_transientMemory, maxCarriedRaccoons, ALLOC_GARBAGE);
	{
		if(playerCarriedEntity.type == EntityType_raccoon)
		{
			carriedRaccoonIndices.push(playerCarriedEntity.index);
		}

		for (s32 i = 0; i < smallPots.count; ++i)
		{
			if (smallPots.carriedEntities[i].type == EntityType_raccoon)
			{
				f32 escapeChance = 0.001;
				if (random_value() < escapeChance)
				{
					smallPots.carriedEntities[i] = {EntityType_none};
				}			
				else
				{
					carriedRaccoonIndices.push(smallPots.carriedEntities[i].index);
				}
			}
		}
	}


	for(s32 i = 0; i < raccoons.count; ++i)
	{
		CharacterInput raccoonCharacterMotorInput = {};

		v3 toTarget 			= raccoons.targetPositions[i] - raccoons.transforms[i].position;
		f32 distanceToTarget 	= v3_length(toTarget);

		v3 input = {};

		if (distanceToTarget < 1.0f)
		{
			// Todo(Leo): Raccoons probably dont operate on terrain level...
			v3 newTargetPosition 			= random_inside_unit_square() * 100 - v3({50,50,0});
			newTargetPosition.z 			= get_terrain_height(game_get_collision_system(game), newTargetPosition.xy);
			raccoons.targetPositions[i] 	= newTargetPosition;
			toTarget 						= raccoons.targetPositions[i] - raccoons.transforms[i].position;
		}
		else
		{
			input.xy	 		= raccoons.targetPositions[i].xy - raccoons.transforms[i].position.xy;
			f32 inputMagnitude 	= v3_length(input);
			input 				= input / inputMagnitude;
			inputMagnitude 		= f32_clamp(inputMagnitude, 0.0f, 1.0f);
			input 				= input * inputMagnitude;
		}

		raccoonCharacterMotorInput = {input, false, false};


		bool isCarried = false;
		for (s32 carriedRaccoonIndex : carriedRaccoonIndices)
		{
			if (carriedRaccoonIndex == i)
			{
				isCarried = true;
				break;
			}
		}


		if (isCarried == false)
		{
			update_character_motor( raccoons.characterMotors[i],
									raccoonCharacterMotorInput,
									game_get_collision_system(game),
									elapsedTime,
									DEBUG_LEVEL_NPC);
		}

		// debug_draw_circle_xy(snap_on_ground(raccoons.targetPositions[i].xy) + v3{0,0,1}, 1, colour_bright_red, DEBUG_LEVEL_ALWAYS);
	}
}


internal Raccoons raccoons_initialize(MemoryArena & allocator, Game * game)
{
	Raccoons raccoons = {};

	raccoons.count = 4;
	push_multiple_memories(	allocator,
							raccoons.count,
							ALLOC_ZERO_MEMORY,
							
							&raccoons.modes,
							&raccoons.transforms,
							&raccoons.targetPositions,
							&raccoons.characterMotors);

	for (s32 i = 0; i < raccoons.count; ++i)
	{
		raccoons.modes[i]					= RaccoonMode_idle;

		raccoons.transforms[i] 				= identity_transform;
		raccoons.transforms[i].position 	= random_inside_unit_square() * 100 - v3{50, 50, 0};
		raccoons.transforms[i].position.z  	= get_terrain_height(game_get_collision_system(game), raccoons.transforms[i].position.xy);

		raccoons.targetPositions[i] 		= random_inside_unit_square() * 100 - v3{50,50,0};
		raccoons.targetPositions[i].z  		= get_terrain_height(game_get_collision_system(game), raccoons.targetPositions[i].xy);

		// ------------------------------------------------------------------------------------------------

		// Note Todo(Leo): Super debug, not at all like this
		raccoons.characterMotors[i] = {};
		raccoons.characterMotors[i].transform = &raccoons.transforms[i];
		raccoons.characterMotors[i].walkSpeed = 2;
		raccoons.characterMotors[i].runSpeed = 4;

		{
			raccoons.characterMotors[i].animations[CharacterAnimation_walk] 	= assets_get_animation(game_get_assets(game), AnimationAssetId_raccoon_empty);
			raccoons.characterMotors[i].animations[CharacterAnimation_run] 		= assets_get_animation(game_get_assets(game), AnimationAssetId_raccoon_empty);
			raccoons.characterMotors[i].animations[CharacterAnimation_idle]		= assets_get_animation(game_get_assets(game), AnimationAssetId_raccoon_empty);
			raccoons.characterMotors[i].animations[CharacterAnimation_jump] 	= assets_get_animation(game_get_assets(game), AnimationAssetId_raccoon_empty);
			raccoons.characterMotors[i].animations[CharacterAnimation_fall] 	= assets_get_animation(game_get_assets(game), AnimationAssetId_raccoon_empty);
			raccoons.characterMotors[i].animations[CharacterAnimation_crouch] 	= assets_get_animation(game_get_assets(game), AnimationAssetId_raccoon_empty);
		}
	}

	raccoons.mesh 		= assets_get_mesh(game_get_assets(game), MeshAssetId_raccoon);
	raccoons.material	= assets_get_material(game_get_assets(game), MaterialAssetId_raccoon);

	return raccoons;
}
