struct PhysicsEntity
{
	EntityReference entity;
	f32 			velocity;
};

struct PhysicsWorld
{
	Array<PhysicsEntity> entities;
};

internal void initialize_physics_world(PhysicsWorld & physicsWorld, MemoryArena & allocator)
{
	physicsWorld 			= {};
	physicsWorld.entities 	= push_array<PhysicsEntity>(allocator, 1000, ALLOC_GARBAGE);
}

internal void physics_world_push_entity(PhysicsWorld & physicsWorld, EntityReference entity)
{
	Assert(physicsWorld.entities.count < physicsWorld.entities.capacity);
	physicsWorld.entities[physicsWorld.entities.count++] = {entity, 0};
}

internal void physics_world_remove_entity(PhysicsWorld & physicsWorld, EntityReference entity)
{
	for (s32 i = 0; i < physicsWorld.entities.count; ++i)
	{
		if (physicsWorld.entities[i].entity == entity)
		{
			array_unordered_remove(physicsWorld.entities, i);
		}
	}
}

internal void update_physics_world(PhysicsWorld & physics, Game * game, f32 elapsedTime)
{
	for (s32 i = 0; i < physics.entities.count; ++i)
	{
		// bool cancelFall = physics.entities[i].entity == game->playerCarriedEntity;
		// if (cancelFall)
		// {
		// 	array_unordered_remove(physics.entities, i);
		// 	i -= 1;

		// 	continue;
		// }

		v3 * position 	= entity_get_position(game, physics.entities[i].entity);
		Assert(position != nullptr && "That entity has not position");

		// f32 groundHeight = get_terrain_height(game->collisionSystem, position->xy);
		f32 groundHeight = get_terrain_height(game_get_collision_system(game), position->xy);

		// Todo(Leo): this is in reality dependent on frame rate, this works for development capped 30 ms frame time
		// but is lower when frametime is lower
		constexpr f32 physicsSleepThreshold = 0.2;

		f32 & velocity 	= physics.entities[i].velocity;
		if (velocity < 0 && position->z < groundHeight)
		{
			position->z = groundHeight;

			{
				// log_debug(FILE_ADDRESS, velocity);
				f32 maxVelocityForAudio = 10;
				f32 velocityForAudio 	= f32_clamp(-velocity, 0, maxVelocityForAudio);
				f32 volume 				= velocityForAudio / maxVelocityForAudio;


				// Todo(Leo): add this back
				// game->audioClipsOnPlay.push({game->stepSFX, 0, random_range(0.5, 1.5), volume, *position});
			}


			// todo (Leo): move to physics properties per game object type
			f32 bounceFactor 	= 0.5;
			velocity 			= -velocity * bounceFactor;

			if (velocity < physicsSleepThreshold)
			{
				array_unordered_remove(physics.entities, i);
				i -= 1;
			}

		}
		else
		{
			velocity 		+= elapsedTime * physics_gravity_acceleration;
			position->z 	+= elapsedTime * velocity;
		}
	}
}