/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/
struct PlayerInput
{
	v2 		movement;
	bool8 	jump;
	bool8 	crouch;
	bool8 	interact;
	bool8 	pickupOrDrop;
};

static PlayerInput player_input_from_platform_input(PlatformInput * platformInput)
{
	PlayerInput playerInput = {};

	if (input_is_device_used(platformInput, InputDevice_gamepad))
	{

		playerInput.movement.x 	= input_axis_get_value(platformInput, InputAxis_move_x);
		playerInput.movement.y 	= input_axis_get_value(platformInput, InputAxis_move_y);
	
		constexpr InputButton jumpButton 			= InputButton_nintendo_x;
		constexpr InputButton crouchButton 			= InputButton_invalid;
		constexpr InputButton interactButton 		= InputButton_nintendo_b;
		constexpr InputButton pickupOrDropButton 	= InputButton_nintendo_a;

		playerInput.jump			= input_button_went_down(platformInput, jumpButton);
		playerInput.crouch 			= input_button_went_down(platformInput, crouchButton);
		playerInput.interact 		= input_button_went_down(platformInput, interactButton);
		playerInput.pickupOrDrop	= input_button_went_down(platformInput, pickupOrDropButton);
	}
	else
	{
		playerInput.movement.x 	= input_button_is_down(platformInput, InputButton_wasd_right)
								- input_button_is_down(platformInput, InputButton_wasd_left);

		playerInput.movement.y 	= input_button_is_down(platformInput, InputButton_wasd_up)
								- input_button_is_down(platformInput, InputButton_wasd_down);

		constexpr InputButton jumpButton 			= InputButton_keyboard_space;
		constexpr InputButton crouchButton 			= InputButton_invalid;
		constexpr InputButton interactButton 		= InputButton_mouse_0;
		constexpr InputButton pickupOrDropButton 	= InputButton_mouse_1;

		playerInput.jump			= input_button_went_down(platformInput, InputButton_keyboard_space);
		playerInput.crouch 			= input_button_went_down(platformInput, crouchButton);
		playerInput.interact 		= input_button_went_down(platformInput, InputButton_mouse_0);
		playerInput.pickupOrDrop	= input_button_went_down(platformInput, InputButton_mouse_1);
	}

	playerInput.movement 	= clamp_length_v2(playerInput.movement, 1);

	

	return playerInput;
}

CharacterInput update_player_input(	Camera & 			playerCamera,
									PlayerInput const & input)
{
	v3 viewForward 		= get_forward(&playerCamera);
	viewForward.z 		= 0;
	viewForward 		= v3_normalize(viewForward);
	v3 viewRight 		= v3_cross(viewForward, v3_up);

	v3 worldSpaceInput 	= viewRight * input.movement.x
						+ viewForward * input.movement.y;


	CharacterInput result 	= {worldSpaceInput, input.jump, input.crouch};
	result.climbInput 		= input.movement;
	return result;
} 

struct Player
{
	Transform3D 		characterTransform;
	CharacterMotor 		characterMotor;
	SkeletonAnimator 	skeletonAnimator;
	EntityReference 	carriedEntity;
};

#if 0
internal void player_update(Player & player, Game * game)
{
	CharacterInput playerCharacterMotorInput = {};
	PlayerInput playerInput = {};
	if (playerInputAvailable)
	{

		if (input_button_went_down(input, InputButton_nintendo_y))
		{
			game->nobleWanderTargetPosition = game->player.characterTransform.position;
			game->nobleWanderWaitTimer 		= 0;
			game->nobleWanderIsWaiting 		= false;
		}

		playerInput 				= player_input_from_platform_input(input);
		playerCharacterMotorInput 	= update_player_input(game->worldCamera, playerInput);
	}
	update_character_motor(game->player.characterMotor, playerCharacterMotorInput, game->collisionSystem, scaledTime, DEBUG_LEVEL_PLAYER);


	// Todo(leo): Should these be handled differently????
	f32 playerPickupDistance 	= 1.0f;
	f32 playerInteractDistance 	= 1.0f;

	/// PICKUP OR DROP
	if (playerInput.pickupOrDrop)
	{
		v3 playerPosition = game->player.characterTransform.position;

		switch(game->player.carriedEntity.type)
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
								game->player.carriedEntity = {EntityType_box, i};
							}
							else
							{
								game->player.carriedEntity = game->boxes.carriedEntities[i];
								game->boxes.carriedEntities[i] = {EntityType_none};
							}

							pickup = false;
						}
					}
				}

				v3 playerPosition = game->player.characterTransform.position;

				/* Todo(Leo): Do this properly, taking into account player facing direction and distances etc. */
				auto check_pickup = [&](s32 count, Transform3D const * transforms, EntityType entityType)
				{
					for (s32 i = 0; i < count; ++i)
					{
						if (v3_length(playerPosition - transforms[i].position) < playerPickupDistance)
						{
							game->player.carriedEntity = {entityType, i};
							pickup = false;
						}
					}
				};


				if (pickup)
				{
					check_pickup(game->smallPots.count, game->smallPots.transforms, EntityType_small_pot);
				}

				if (pickup)
				{
					for (s32 i = 0; i < game->waters.count; ++i)
					{
						if (v3_length(playerPosition - game->waters.positions[i]) < playerPickupDistance)
						{
							game->player.carriedEntity = {EntityType_water, i};
							pickup = false;
						}
					}


				}


				if (pickup)
				{
					check_pickup(game->raccoonCount, game->raccoonTransforms, EntityType_raccoon);
				}

				if (pickup)
				{
					for (s32 i = 0; i < game->trees.array.count; ++i)
					{
						Tree & tree = game->trees.array[i];

						if (v3_length(playerPosition - game->trees.array[i].position) < playerPickupDistance)
						{
							// if (game->trees.array[i].planted == false)
							{
								game->player.carriedEntity = {EntityType_tree_3, i};
							}

							pickup = false;
						}

					}
				}

				// Todo(Leo): maybe this is bad, as in should we only do this if we actually started carrying something
				physics_world_remove_entity(game->physicsWorld, game->player.carriedEntity);

			} break;

			case EntityType_raccoon:
				game->raccoonTransforms[game->player.carriedEntity.index].rotation = quaternion_identity;
				// Todo(Leo): notice no break here, little sketcthy, maybe do something about it, probably in raccoon movement code
				// Todo(Leo): keep raccoon oriented normally, add animation or simulation make them look hangin..

			case EntityType_small_pot:
			case EntityType_tree_3:
			case EntityType_box:	
			case EntityType_water:
				physics_world_push_entity(game->physicsWorld, game->player.carriedEntity);
				game->player.carriedEntity = {EntityType_none};
				break;

			default:
				log_debug(FILE_ADDRESS, "cannot pickup or drop that entity: (", game->player.carriedEntity.type, ", #", game->player.carriedEntity.index, ")");
		}
	}

	f32 boxInteractDistance = 0.5f + playerInteractDistance;

	if (playerInput.interact)
	{
		bool32 interact = true;

		// TRY PUT STUFF INTO BOX
		bool playerCarriesContainableEntity = 	game->player.carriedEntity.type != EntityType_none
												&& game->player.carriedEntity.type != EntityType_box;

		if (interact && playerCarriesContainableEntity)
		{
			for (s32 i = 0; i < game->boxes.count; ++i)
			{
				f32 distanceToBox = v3_length(game->boxes.transforms[i].position - game->player.characterTransform.position);
				if (distanceToBox < boxInteractDistance && game->boxes.carriedEntities[i].type == EntityType_none)
				{
					game->boxes.carriedEntities[i] = game->player.carriedEntity;
					game->player.carriedEntity = {EntityType_none};

					interact = false;
				}				
			}
		}

		// TRY PUT STUFF INTO POT
		bool playerCarriesPottableEntity = 	game->player.carriedEntity.type == EntityType_tree_3
											|| game->player.carriedEntity.type == EntityType_raccoon;

		if (interact && playerCarriesPottableEntity)
		{
			log_debug("Try put something to pot");

			for (s32 i = 0; i < game->smallPots.count; ++i)
			{
				f32 distanceToPot = v3_length(game->smallPots.transforms[i].position - game->player.characterTransform.position);
				if (distanceToPot < playerInteractDistance && game->smallPots.carriedEntities[i].type == EntityType_none)
				{
					log_debug(FILE_ADDRESS, "Stuff put into pot");

					game->smallPots.carriedEntities[i] = game->player.carriedEntity;
					game->player.carriedEntity 	= {EntityType_none};

					interact = false;
				}
			}
		}
		
		// TRY OPEN OR CLOSE NEARBY BOX
		bool32 playerCarriesNothing = game->player.carriedEntity.type == EntityType_none;

		if (interact && playerCarriesNothing)
		{
			for (s32 i = 0; i < game->boxes.count; ++i)
			{
				f32 distanceToBox = v3_length(game->boxes.transforms[i].position - game->player.characterTransform.position);
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
		bool32 playerCarriesTree = game->player.carriedEntity.type == EntityType_tree_3;

		if (interact && playerCarriesTree)
		{
			game->trees.array[game->player.carriedEntity.index].planted = true;
			v3 position = game->trees.array[game->player.carriedEntity.index].position;
			position.z = get_terrain_height(game->collisionSystem, position.xy);
			game->trees.array[game->player.carriedEntity.index].position = position;
			game->player.carriedEntity.index = -1;
			game->player.carriedEntity.type = EntityType_none;			

			interact = false;
		}

	}


	for(s32 i = 0; i < game->boxes.count; ++i)
	{
		v4 colour = game->boxes.carriedEntities[i].type == EntityType_none ? colour_bright_red : colour_bright_green;
		FS_DEBUG_NPC(debug_draw_circle_xy(game->boxes.transforms[i].position + v3{0,0,0.6}, 1, colour));
		FS_DEBUG_NPC(debug_draw_circle_xy(game->boxes.transforms[i].position + v3{0,0,0.6}, 1.5, colour_bright_cyan));
	}
}
#endif