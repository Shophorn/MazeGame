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