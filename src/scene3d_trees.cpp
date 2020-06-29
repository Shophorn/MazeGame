/*
Leo Tamminen

Trees and leaves.
*/

struct Leaves
{
	s32 			capacity;
	s32 			count;

	v3 				position;
	quaternion 		rotation;

	// Todo(Leo): maybe put these into a struct
	v3 * 			localPositions;
	quaternion * 	localRotations;
	f32 * 			localScales;

	f32 * 			swayPositions;
	v3 * 			swayAxes;
};
 
internal Leaves make_leaves(MemoryArena & allocator, s32 capacity)
{
	Leaves leaves 			= {};

	leaves.capacity 		= capacity;
	leaves.localPositions 	= push_memory<v3>(allocator, leaves.capacity, ALLOC_NO_CLEAR);
	leaves.localRotations 	= push_memory<quaternion>(allocator, leaves.capacity, ALLOC_NO_CLEAR);
	leaves.localScales 		= push_memory<f32>(allocator, leaves.capacity, ALLOC_NO_CLEAR);
	leaves.swayPositions 	= push_memory<f32>(allocator, leaves.capacity, ALLOC_NO_CLEAR);
	leaves.swayAxes 		= push_memory<v3>(allocator, leaves.capacity, ALLOC_NO_CLEAR);

	return leaves;
}

internal void draw_leaves(Leaves & leaves, f32 elapsedTime)
{
	s32 drawCount = min_f32(leaves.capacity, leaves.count);

	m44 * leafTransforms = push_memory<m44>(*global_transientMemory, drawCount, ALLOC_NO_CLEAR);
	for (s32 i = 0; i < drawCount; ++i)
	{
		v3 position 			= leaves.position + rotate_v3(leaves.rotation, leaves.localPositions[i]);

		constexpr f32 swayRange = 0.5;
		leaves.swayPositions[i] = mod_f32(leaves.swayPositions[i] + elapsedTime, swayRange * 4);
		f32 sway 				= mathfun_pingpong_f32(leaves.swayPositions[i], swayRange * 2) - swayRange;
		quaternion rotation 	= axis_angle_quaternion(leaves.swayAxes[i], sway);
		rotation 				= leaves.localRotations[i] * leaves.rotation * rotation;

		v3 scale 				= make_uniform_v3(leaves.localScales[i]);

		leafTransforms[i] 		= transform_matrix(	position, rotation,	scale);
	}

	platformApi->draw_leaves(platformGraphics, drawCount, leafTransforms);
}

struct TimedModule
{
	char 	letter;
	float 	age;
	float 	parameter;
};

struct Waters
{
	s32 			capacity;
	s32 			count;
	Transform3D * 	transforms;
	f32	* 			levels;
};


struct TimedLSystem
{
	s32 			wordCapacity;
	s32 			wordCount;
	TimedModule * 	aWord;
	TimedModule * 	bWord;

	f32 			timeSinceLastUpdate;

	v3 			position;
	quaternion 	rotation;

	f32 totalAge;
	f32 maxAge;

	// Dynamic mesh
	MemoryView<u16> 	indices;
	MemoryView<Vertex> 	vertices;

	void(*advance)		(TimedLSystem & lSystem, Waters & waters, f32 timePassed);
	void(*update_mesh)	(TimedLSystem & lSystem, Leaves & leaves);
};


internal void advance_lsystem_time(TimedLSystem & lSystem, Waters & waters, f32 timePassed)
{
	f32 waterDistanceThreshold = 1;

	lSystem.totalAge += timePassed;

	TimedModule * aWord 			= lSystem.aWord;
	MemoryView<TimedModule> bWord 	= {lSystem.wordCapacity, 0, lSystem.bWord};

	constexpr float growSpeed 	= 0.5f;
	// constexpr float growSpeed 	= 0.0f;
	constexpr float terminalAge = 1.0f;
	constexpr float angle 		= π / 8;

	for (int i = 0; i < lSystem.wordCount; ++i)
	{
		aWord[i].age += timePassed * growSpeed;
		
		switch(aWord[i].letter)
		{
			case 'X':
			{
				if (aWord[i].age > terminalAge)
				{
					TimedModule production [] =
					{
						{'F', terminalAge, aWord[i].parameter * terminalAge},
						{'<', 0, 5 * π/7},
						{'['},
							{'&', 0, angle},
							{'X', 0, random_range(0.6f, 1.0f)},
							{'L', 0, random_range(0.25f, 0.4f)},
						{']'},
						{'<', 0, 6 * π/7},
						{'['},
							{'&', 0, angle},
							{'X', 0, random_range(0.6f, 1.0f)},
							{'L', 0, random_range(0.25f, 0.4f)},
						{']'},
						// count here = 13

						{'<', 0, 4 * π/7},
						{'['},
							{'&', 0, angle},
							{'X', 0, random_range(0.6f, 1.0f)},
							{'L', 0, random_range(0.25f, 0.4f)},
						{']'},
						// count here = 20
					};

					Assert(memory_view_available(bWord) >= array_count(production));

					// Note(Leo) Important: these values must be manually copied from above array
					s32 count = random_choice() ? 13 : 20; 
					copy_structs(bWord.data + bWord.count, production, count);
					bWord.count += count;
				}
				else
				{
					TimedModule production [] =
					{
						aWord[i],
						{'L', 0, random_range(0.25f, 0.4f)},
					};

					Assert(memory_view_available(bWord) >= array_count(production));
					
					copy_structs(bWord.data + bWord.count, production, array_count(production));
					bWord.count += array_count(production);
				}
			} break;

			case 'L':
				break;

			default:
				Assert(memory_view_available(bWord) > 0);
				bWord.data[bWord.count++] = aWord[i];
		}
	}

	swap(lSystem.aWord, lSystem.bWord);
	lSystem.wordCount = bWord.count;
}

internal void update_lsystem_mesh(TimedLSystem & lSystem, Leaves & leaves)
{
	constexpr v3 lSystemRight 	= right_v3;
	constexpr v3 lSystemForward = up_v3;
	constexpr v3 lSystemUp 		= -forward_v3;

	leaves.count = 0;

	struct TurtleState
	{
		v3 			position;
		quaternion 	orientation;
	};

	int stateIndex 				= 0;
	TurtleState states[1000] 	= {};
 
	states[0].position 		= {};
	states[0].orientation 	= identity_quaternion;

	leaves.count = 0;

	int wordCount 		= lSystem.wordCount;
	TimedModule * word 	= lSystem.aWord;

	lSystem.vertices.count = 0;
	lSystem.indices.count = 0;

	float widthGrowFactor = 0.03f;
	constexpr float terminalAge = 1.0f;

	for (int i = 0; i < wordCount; ++i)
	{
		TurtleState & state = states[stateIndex];

		switch(word[i].letter)
		{
			case 'F':
			{
				v3 forward 				= rotate_v3(state.orientation, lSystemForward);
				u16 vertexOffsetBefore 	= lSystem.vertices.count;
				f32 width 				= word[i].age * widthGrowFactor;

				m44 transform = transform_matrix(state.position, state.orientation, {width, width, 1});

				Assert(memory_view_available(lSystem.vertices) >= 8);
				s32 & v = lSystem.vertices.count;

				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,0}};

				state.position 	+= forward * word[i].parameter;

				// Note(Leo): Draw upper end slightly thinner, so it matches the following 'X' module
				width 			= (word[i].age - terminalAge) * widthGrowFactor;
				transform 		= transform_matrix(state.position, state.orientation, {width, width, 1});

				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,1}};

				constexpr u16 indices [] = 
				{
					0, 1, 4, 4, 1, 5,
					1, 2, 5, 5, 2, 6,
					2, 3, 6, 6, 3, 7,
					3, 0, 7, 7, 0, 4,
				};
				Assert(memory_view_available(lSystem.indices) >= array_count(indices));

				for(u16 index : indices)
				{
					lSystem.indices.data[lSystem.indices.count++] = index + vertexOffsetBefore;
				}

			} break;

			case 'X':
			{
				Assert(memory_view_available(lSystem.vertices) >= 8);

				v3 forward 				= rotate_v3(state.orientation, lSystemForward);
				u16 vertexOffsetBefore 	= lSystem.vertices.count;

				f32 width 				= word[i].age * widthGrowFactor;
				m44 transform 			= transform_matrix(state.position, state.orientation, {width, width, 1});

				s32 & v = lSystem.vertices.count;

				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,0}};

				state.position 	+= forward * word[i].parameter * word[i].age;
				
				width 		= (word[i].age / 2) *widthGrowFactor;
				transform 	= transform_matrix(state.position, state.orientation, {width, width, 1});

				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,1}};

				constexpr u16 indices [] = 
				{
					0, 1, 4, 4, 1, 5,
					1, 2, 5, 5, 2, 6,
					2, 3, 6, 6, 3, 7,
					3, 0, 7, 7, 0, 4,
				};
				Assert(memory_view_available(lSystem.indices) >= array_count(indices));

				for(u16 index : indices)
				{
					lSystem.indices.data[lSystem.indices.count++] = index + vertexOffsetBefore;
				}

			} break;

			case 'L':
			{
				Assert(leaves.count + 2 < leaves.capacity);

				{
					quaternion rotation = state.orientation; 
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,lSystemRight), -π / 6);
					v3 position 		= state.position + rotate_v3(rotation, lSystemForward) * 0.05f;
					
					rotation 			= normalize_quaternion(rotation);

					int leafIndex 		= leaves.count;
					leaves.count += 1;

					leaves.localPositions[leafIndex]	= position;
					leaves.localRotations[leafIndex] 	= rotation;
					leaves.localScales[leafIndex] 		= word[i].parameter;
					
					// Note(Leo): Use normal forward here, since it refers to leaf's own space
					leaves.swayAxes[leafIndex]			= rotate_v3(rotation, forward_v3);
					leaves.swayPositions[leafIndex] 	= random_value() * 100;	
				}

				{
					quaternion rotation = state.orientation;
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation, lSystemForward), π); 
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,lSystemRight), -π / 6);
					v3 position 		= state.position + rotate_v3(rotation, lSystemForward) * 0.05f;

					rotation 			= normalize_quaternion(rotation);

					int leafIndex = leaves.count;
					leaves.count += 1;

					leaves.localPositions[leafIndex]	= position;
					leaves.localRotations[leafIndex] 	= rotation;
					leaves.localScales[leafIndex] 		= word[i].parameter;
					
					// Note(Leo): Use normal forward here, since it refers to leaf's own space
					leaves.swayAxes[leafIndex]			= rotate_v3(rotation, forward_v3);
					leaves.swayPositions[leafIndex]		= random_value() * 100;
				}

			} break;

			case '+':
			{
				v3 up = rotate_v3(state.orientation, lSystemUp);
				state.orientation = state.orientation * axis_angle_quaternion(up, word[i].parameter);
			} break;

			case '<':
			{
				v3 forward 			= rotate_v3(state.orientation, lSystemForward);
				state.orientation 	= state.orientation * axis_angle_quaternion(forward, word[i].parameter);
			} break;

			case '&':
			{	
				v3 right 			= rotate_v3(state.orientation, lSystemRight);
				state.orientation 	= state.orientation * axis_angle_quaternion(right, word[i].parameter);
			} break;

			case '[':
			{
				states[stateIndex + 1] = states[stateIndex];
				stateIndex += 1;
			} break;

			case ']':
			{
				stateIndex -= 1;
			} break;
		}

	}

	Assert(stateIndex == 0 && "All states have not been popped");

	if (lSystem.totalAge > 0 && leaves.count == 0)
	{
		return;
	}
}

internal TimedLSystem make_lsystem(MemoryArena & allocator)
{
	s32 wordCapacity 	= 100'000;
	s32 indexCapacity 	= 75'000;
	s32 vertexCapacity	= 25'000;

	TimedLSystem lSystem 	= {};

	lSystem.wordCapacity 	= wordCapacity;
	lSystem.wordCount 		= 0;
	lSystem.aWord 			= push_memory<TimedModule>(allocator, lSystem.wordCapacity, ALLOC_NO_CLEAR);
	lSystem.bWord 			= push_memory<TimedModule>(allocator, lSystem.wordCapacity, ALLOC_NO_CLEAR);

	TimedModule axiom 		= {'X', 0, 0.7};
	lSystem.aWord[0] 		= axiom;
	lSystem.wordCount		= 1;

	lSystem.vertices 		= push_memory_view<Vertex>(allocator, vertexCapacity);
	lSystem.indices 		= push_memory_view<u16>(allocator, indexCapacity);

	lSystem.totalAge 		= 0;
	lSystem.maxAge 			= 15;

	return lSystem;
}

/*

A: apex
B: branch
F: forward, trunk

A(t) : t < 1 -> A(t + n)L
A(t) : t > 1 -> F(1)[B(0)L]A(0)L
B(t) -> B(t + n)L
F(t) -> F(t + n)
L -> -

0: 		A(0)
0.99: 	A(0.99)L
1: 		F(1)[B(0)L]A(0)L
1.5: 	F(1.5)[B(0.5)]A(0.5)L
2:		F(2)[B(1)L]F(1)[B(0)L]A(0.5)L 

*/

internal void advance_lsystem_time_tree2(TimedLSystem & lSystem, Waters & waters, f32 timePassed)
{
	f32 waterDistanceThreshold = 1;

	lSystem.totalAge += timePassed;

	TimedModule * aWord 			= lSystem.aWord;
	MemoryView<TimedModule> bWord 	= {lSystem.wordCapacity, 0, lSystem.bWord};

	constexpr float growSpeed 	= 0.5f;
	// constexpr float growSpeed 	= 0.0f;
	constexpr float terminalAge = 1.0f;
	constexpr float angle 		= π / 8;

	for (int i = 0; i < lSystem.wordCount; ++i)
	{
		aWord[i].age += timePassed * growSpeed;
		
		switch(aWord[i].letter)
		{
			case 'A':
			{
				if(aWord[i].age > terminalAge)
				{
					TimedModule production [] = 
					{
						{'F', terminalAge, aWord[i].parameter},
						{'['},
							{'&', 0, 2*π/5},
							{'B', 0, aWord[i].parameter},
							{'L', 0, random_range(0.25f, 0.4)},
						{']'},
						{'<', 0, 2.391 },
						{'A', 0, aWord[i].parameter},
						{'L', 0, random_range(0.25f, 0.4)}
					};

					s32 count = array_count(production);
					Assert(memory_view_available(bWord) >= count);
					copy_structs(bWord.data + bWord.count, production, count);
					bWord.count += count;
				}
				else
				{
					TimedModule production [] =
					{
						aWord[i],
						{'L', 0, random_range(0.25f, 0.4f)},
					};

					Assert(memory_view_available(bWord) >= array_count(production));
					copy_structs(bWord.data + bWord.count, production, array_count(production));
					bWord.count += array_count(production);
				}
			} break;

			case 'B':
				if (aWord[i].age < terminalAge * 3)
				{
					Assert(memory_view_available(bWord) >= 2);
					bWord.data[bWord.count++] = aWord[i];
					bWord.data[bWord.count++] = {'L', 0, random_range(0.25f, 0.4f)};

				}
				break;				

			case 'L':
				break;

			default:
				Assert(memory_view_available(bWord) >= 1);
				bWord.data[bWord.count++] = aWord[i];
				break;
		}
	}

	swap(lSystem.aWord, lSystem.bWord);
	lSystem.wordCount = bWord.count;
}

internal void update_lsystem_mesh_tree2(TimedLSystem & lSystem, Leaves & leaves)
{
	constexpr v3 lSystemRight 	= right_v3;
	constexpr v3 lSystemForward = up_v3;
	constexpr v3 lSystemUp 		= -forward_v3;

	leaves.count = 0;

	struct TurtleState
	{
		v3 			position;
		quaternion 	orientation;
	};

	int stateIndex 				= 0;
	TurtleState states[1000] 	= {};
 
	states[0].position 		= {};
	states[0].orientation 	= identity_quaternion;

	leaves.count = 0;

	int wordCount 		= lSystem.wordCount;
	TimedModule * word 	= lSystem.aWord;

	lSystem.vertices.count = 0;
	lSystem.indices.count = 0;

	float widthGrowFactor = 0.03f;
	constexpr float terminalAge = 1.0f;

	for (int i = 0; i < wordCount; ++i)
	{
		TurtleState & state = states[stateIndex];

		switch(word[i].letter)
		{
			case 'F':
			{
				v3 forward 				= rotate_v3(state.orientation, lSystemForward);
				u16 vertexOffsetBefore 	= lSystem.vertices.count;
				f32 width 				= word[i].age * widthGrowFactor;

				m44 transform = transform_matrix(state.position, state.orientation, {width, width, 1});

				Assert(memory_view_available(lSystem.vertices) >= 8);
				s32 & v = lSystem.vertices.count;

				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,0}};

				state.position 	+= forward * word[i].parameter;

				// Note(Leo): Draw upper end slightly thinner, so it matches the following 'X' module
				width 			= (word[i].age - terminalAge) * widthGrowFactor;
				transform 		= transform_matrix(state.position, state.orientation, {width, width, 1});

				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,1}};

				constexpr u16 indices [] = 
				{
					0, 1, 4, 4, 1, 5,
					1, 2, 5, 5, 2, 6,
					2, 3, 6, 6, 3, 7,
					3, 0, 7, 7, 0, 4,
				};
				Assert(memory_view_available(lSystem.indices) >= array_count(indices));

				for(u16 index : indices)
				{
					lSystem.indices.data[lSystem.indices.count++] = index + vertexOffsetBefore;
				}

			} break;

			case 'A':
			case 'B':
			{
				Assert(memory_view_available(lSystem.vertices) >= 8);

				v3 forward 				= rotate_v3(state.orientation, lSystemForward);
				u16 vertexOffsetBefore 	= lSystem.vertices.count;

				f32 width 				= word[i].age * widthGrowFactor;
				m44 transform 			= transform_matrix(state.position, state.orientation, {width, width, 1});

				s32 & v = lSystem.vertices.count;

				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,0}};
				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,0}};

				state.position 	+= forward * word[i].parameter * word[i].age;
				
				width 		= (word[i].age / 2) *widthGrowFactor;
				transform 	= transform_matrix(state.position, state.orientation, {width, width, 1});

				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,1}};
				lSystem.vertices.data[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,1}};

				constexpr u16 indices [] = 
				{
					0, 1, 4, 4, 1, 5,
					1, 2, 5, 5, 2, 6,
					2, 3, 6, 6, 3, 7,
					3, 0, 7, 7, 0, 4,
				};
				Assert(memory_view_available(lSystem.indices) >= array_count(indices));

				for(u16 index : indices)
				{
					lSystem.indices.data[lSystem.indices.count++] = index + vertexOffsetBefore;
				}

			} break;

			case 'L':
			{
				Assert(leaves.count + 2 < leaves.capacity);

				{
					quaternion rotation = state.orientation; 
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,lSystemRight), -π / 6);
					v3 position 		= state.position + rotate_v3(rotation, lSystemForward) * 0.05f;
					
					rotation 			= normalize_quaternion(rotation);

					int leafIndex 		= leaves.count;
					leaves.count += 1;

					leaves.localPositions[leafIndex]	= position;
					leaves.localRotations[leafIndex] 	= rotation;
					leaves.localScales[leafIndex] 		= word[i].parameter;
					
					// Note(Leo): Use normal forward here, since it refers to leaf's own space
					leaves.swayAxes[leafIndex]			= rotate_v3(rotation, forward_v3);
					leaves.swayPositions[leafIndex] 	= random_value() * 100;	
				}

				{
					quaternion rotation = state.orientation;
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation, lSystemForward), π); 
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,lSystemRight), -π / 6);
					v3 position 		= state.position + rotate_v3(rotation, lSystemForward) * 0.05f;

					rotation 			= normalize_quaternion(rotation);

					int leafIndex = leaves.count;
					leaves.count += 1;

					leaves.localPositions[leafIndex]	= position;
					leaves.localRotations[leafIndex] 	= rotation;
					leaves.localScales[leafIndex] 		= word[i].parameter;
					
					// Note(Leo): Use normal forward here, since it refers to leaf's own space
					leaves.swayAxes[leafIndex]			= rotate_v3(rotation, forward_v3);
					leaves.swayPositions[leafIndex]		= random_value() * 100;
				}

			} break;

			case '+':
			{
				v3 up = rotate_v3(state.orientation, lSystemUp);
				state.orientation = state.orientation * axis_angle_quaternion(up, word[i].parameter);
			} break;

			case '<':
			{
				v3 forward 			= rotate_v3(state.orientation, lSystemForward);
				state.orientation 	= state.orientation * axis_angle_quaternion(forward, word[i].parameter);
			} break;

			case '&':
			{	
				v3 right 			= rotate_v3(state.orientation, lSystemRight);
				state.orientation 	= state.orientation * axis_angle_quaternion(right, word[i].parameter);
			} break;

			case '[':
			{
				states[stateIndex + 1] = states[stateIndex];
				stateIndex += 1;
			} break;

			case ']':
			{
				stateIndex -= 1;
			} break;
		}

	}

	Assert(stateIndex == 0 && "All states have not been popped");

	if (lSystem.totalAge > 0 && leaves.count == 0)
	{
		return;
	}
}


/*
struct LSystem1
{
	Module axiom [1] = {{'A', 0.0625f}};

	void generate(Module predecessor, int & wordLength, Module * word)
	{
		float angle = pi / 8;
		float scale = 0.0625f;

		switch(predecessor.letter)
		{
			case 'A':
			{
				Module production [] = 
				{
					{'['},
						{'&', angle},
						{'F', scale},
						{'L'},
						{'A'},
					{']'},
					{'<', 5 * angle},
					{'['},
						{'&', angle},
						{'F', scale},
						{'L'},
						{'A'},
					{']'},
					{'<', 7 * angle},
					{'['},
						{'&', angle},
						{'F', scale},
						{'L'},
						{'A'},
					{']'},
				};
				copy_memory(word + wordLength, production, sizeof(production));
				wordLength += array_count(production);
			} break;
		
			case 'F':
			{
				Module production [] = 
				{
					{'S'},
					{'<', 5 * angle},
					{'F', scale},
				};
				copy_memory(word + wordLength, production, sizeof(production));
				wordLength += array_count(production);
			} break;

			case 'S':
			{
				Module production [] =
				{
					{'F', scale},
					{'L'},
				};
				copy_memory(word + wordLength, production, sizeof(production));
				wordLength += array_count(production);
			} break;

			case 'L':
			{
				Module production [] =
				{
					{'['},
						{'^', 2 * angle},
						{'f', scale},
					{']'},
				};
				copy_memory(word + wordLength, production, sizeof(production));
				wordLength += array_count(production);
			} break;

			default:
				word[wordLength] = predecessor;
				wordLength += 1;
				break;
		}
	}
};

struct LSystem2
{
	static constexpr float r1 = 0.9f;
	static constexpr float r2 = 0.6f;
	static constexpr float a0 = π / 4;
	static constexpr float a2 = π / 4;
	static constexpr float d = to_radians(137.5);
	static constexpr float wr = 0.707;

	Module axiom [1] = {{'A', 1.0f}};

	void generate(Module predecessor, int & wordLength, Module * word)
	{
		float length = predecessor.argument;

		switch(predecessor.letter)
		{
			case 'A':
			{
				Module production [] =
				{
					{'F', length},
					{'['},
						{'&', a0},
						{'B', length * r2},
					{']'},
					{'<', d},
					{'A', length * r1}
				};
				copy_memory(word + wordLength, production, sizeof(production));
				wordLength += array_count(production);
			} break;
		
			case 'B':
			{
				Module production [] =
				{
					{'F', length},
					{'['},
						{'+', -a2},
						{'$'},
						{'C', length * r2},
					{']'},
					{'C', length * r1},
				};


				copy_memory(word + wordLength, production, sizeof(production));
				wordLength += array_count(production);
			} break;

			case 'C':
			{
				Module production [] =
				{
					{'F', length},
					{'['},
						{'+', a2},
						{'$'},
						{'B', length * r2},
					{']'},
					{'B', length * r1}
				};
				copy_memory(word + wordLength, production, sizeof(production));
				wordLength += array_count(production);
			} break;

			default:
				word[wordLength] = predecessor;
				wordLength += 1;
				break;
		}
	}
};
*/