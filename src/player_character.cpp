internal void player_character_update()
{
	CharacterInput playerCharacterMotorInput = {};
	if (playerInputAvailable)
	{
		if (input_button_went_down(input, InputButton_nintendo_y))
		{
			game->nobleWanderTargetPosition = game->playerCharacterTransform.position;
			game->nobleWanderWaitTimer 		= 0;
			game->nobleWanderIsWaiting 		= false;
		}

		playerCharacterMotorInput = update_player_input(game->playerInputState,
														game->worldCamera,
														player_input_from_platform_input(input));
	
		playerCharacterMotorInput.testClimb = input_button_is_down(input, InputButton_keyboard_left_alt);

	}
	update_character_motor(game->playerCharacterMotor, playerCharacterMotorInput, game->collisionSystem, scaledTime, DEBUG_LEVEL_PLAYER);


	// Todo(leo): Should these be handled differently????
	f32 playerPickupDistance 	= 1.0f;
	f32 playerInteractDistance 	= 1.0f;

	/// PICKUP OR DROP
	if (playerInputAvailable && game->playerInputState.events.pickupOrDrop)
	{
		v3 playerPosition 	= game->playerCharacterTransform.position;
		v3 playerDirection 	= quaternion_rotate_v3(game->playerCharacterTransform.rotation, v3_forward);

		switch(game->playerCarriedEntity.type)
		{
			case EntityType_none:
			{
				bool pickup = true;

				if (pickup)
				{
					for(s32 i  = 0; i < game->boxes.count; ++i)
					{
						f32 boxPickupDistance = 0.5f + playerPickupDistance;
						if (v3_length(playerPosition - game->boxes.transforms[i].position) < boxPickupDistance)
						{
							bool32 canPickInsides = game->boxes.carriedEntities[i].type != EntityType_none
													&& game->boxes.states[i] == BoxState_open;

							if (canPickInsides == false)
							{
								game->playerCarriedEntity = {EntityType_box, i};
							}
							else
							{
								game->playerCarriedEntity = game->boxes.carriedEntities[i];
								game->boxes.carriedEntities[i] = {EntityType_none};
							}

							pickup = false;
						}
					}
				}

				v3 playerPosition = game->playerCharacterTransform.position;

				EntityType pickupableEntityTypes [] =
				{
					EntityType_raccoon,
					EntityType_water,
					EntityType_box,
					EntityType_small_pot,
					EntityType_big_pot,
				};

				EntityReference closestEntityForPickup 	= {};
				f32 closestEntityForPickupDistance 		= highest_f32;


				/* Todo(Leo): Do this properly, taking into account player facing direction and distances etc. */
				for (EntityType type : pickupableEntityTypes)
				{
					s32 count = entity_count(game, type);
					for (s32 i = 0; i < count; ++i)
					{
						v3 playerToEntity = *entity_position(game, {type, i}) - playerPosition;
						f32 distance = v3_length(playerToEntity);
						if (distance < playerPickupDistance && distance < closestEntityForPickupDistance)
						{
							closestEntityForPickup 			= {type, i};
							closestEntityForPickupDistance 	= distance;
						}
					}
				}

				if (closestEntityForPickup.type != EntityType_none)
				{
					game->playerCarriedEntity = closestEntityForPickup;
					//entity_on_pickup();
				}

			} break;

			case EntityType_raccoon:
				game->raccoonTransforms[game->playerCarriedEntity.index].rotation = quaternion_identity;
				// Todo(Leo): notice no break here, little sketcthy, maybe do something about it, probably in raccoon movement code
				// Todo(Leo): keep raccoon oriented normally, add animation or simulation make them look hangin..

			case EntityType_small_pot:
			case EntityType_big_pot:
			case EntityType_tree_3:
			case EntityType_box:	
			case EntityType_water:
				physics_world_push_entity(game->physicsWorld, game->playerCarriedEntity);
				game->playerCarriedEntity = {EntityType_none};
				break;

			default:
				log_debug(FILE_ADDRESS, "cannot pickup or drop that entity: (", game->playerCarriedEntity.type, ", #", game->playerCarriedEntity.index, ")");
		}
	}

	f32 boxInteractDistance = 0.5f + playerInteractDistance;

	if (game->playerInputState.events.interact)
	{
		bool32 interact = game->playerInputState.events.interact;

		// TRY PUT STUFF INTO BOX
		bool playerCarriesContainableEntity = 	game->playerCarriedEntity.type != EntityType_none
												&& game->playerCarriedEntity.type != EntityType_box;

		if (interact && playerCarriesContainableEntity)
		{
			for (s32 i = 0; i < game->boxes.count; ++i)
			{
				f32 distanceToBox = v3_length(game->boxes.transforms[i].position - game->playerCharacterTransform.position);
				if (distanceToBox < boxInteractDistance && game->boxes.carriedEntities[i].type == EntityType_none)
				{
					game->boxes.carriedEntities[i] = game->playerCarriedEntity;
					game->playerCarriedEntity = {EntityType_none};

					interact = false;
				}				
			}
		}

		// TRY PUT STUFF INTO POT
		bool playerCarriesPottableEntity = 	game->playerCarriedEntity.type == EntityType_tree_3
											|| game->playerCarriedEntity.type == EntityType_raccoon;

		if (interact && playerCarriesPottableEntity)
		{
			log_debug("Try put something to pot");

			for (s32 i = 0; i < game->smallPots.count; ++i)
			{
				f32 distanceToPot = v3_length(game->smallPots.transforms[i].position - game->playerCharacterTransform.position);
				if (distanceToPot < playerInteractDistance && game->smallPots.carriedEntities[i].type == EntityType_none)
				{
					log_debug(FILE_ADDRESS, "Stuff put into pot");

					game->smallPots.carriedEntities[i] = game->playerCarriedEntity;
					game->playerCarriedEntity 	= {EntityType_none};

					interact = false;
				}
			}
		}
		
		// TRY OPEN OR CLOSE NEARBY BOX
		bool32 playerCarriesNothing = game->playerCarriedEntity.type == EntityType_none;

		if (interact && playerCarriesNothing)
		{
			for (s32 i = 0; i < game->boxes.count; ++i)
			{
				f32 distanceToBox = v3_length(game->boxes.transforms[i].position - game->playerCharacterTransform.position);
				if (distanceToBox < boxInteractDistance)
				{
					if (game->boxes.states[i] == BoxState_closed)
					{
						game->boxes.states[i] 			= BoxState_opening;
						game->boxes.openStates[i] 	= 0;

						interact = false;
					}

					else if (game->boxes.states[i] == BoxState_open)
					{
						game->boxes.states[i] 			= BoxState_closing;
						game->boxes.openStates[i] 	= 0;

						interact = false;
					}
				}
			}
		}

		// PLANT THE CARRIED TREE
		bool32 playerCarriesTree = game->playerCarriedEntity.type == EntityType_tree_3;

		if (interact && playerCarriesTree)
		{
			game->trees.array[game->playerCarriedEntity.index].planted = true;
			v3 position = game->trees.array[game->playerCarriedEntity.index].position;
			position.z = get_terrain_height(game->collisionSystem, position.xy);
			game->trees.array[game->playerCarriedEntity.index].position = position;
			game->playerCarriedEntity.index = -1;
			game->playerCarriedEntity.type = EntityType_none;			

			interact = false;
		}

	}
}