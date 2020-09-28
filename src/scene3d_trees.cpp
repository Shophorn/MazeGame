// /*
// Leo Tamminen

// Trees and leaves. And L-Systems.
// */

struct Leaves
{
	s32 			capacity;
	s32 			count;

	v3 				position;
	quaternion 		rotation;

	s32 			colourIndex;

	// Todo(Leo): maybe put these into a struct
	v3 * 			localPositions;
	quaternion * 	localRotations;
	f32 * 			localScales;

	f32 * 			swayPositions;
	v3 * 			swayAxes;

	MaterialHandle	material;
};
 
internal void flush_leaves(Leaves & leaves)
{
	leaves.count = 0;
}

internal Leaves make_leaves(MemoryArena & allocator, s32 capacity)
{
	Leaves leaves 			= {};

	leaves.capacity 		= capacity;
	leaves.localPositions 	= push_memory<v3>(allocator, leaves.capacity, ALLOC_GARBAGE);
	leaves.localRotations 	= push_memory<quaternion>(allocator, leaves.capacity, ALLOC_GARBAGE);
	leaves.localScales 		= push_memory<f32>(allocator, leaves.capacity, ALLOC_GARBAGE);
	leaves.swayPositions 	= push_memory<f32>(allocator, leaves.capacity, ALLOC_GARBAGE);
	leaves.swayAxes 		= push_memory<v3>(allocator, leaves.capacity, ALLOC_GARBAGE);

	return leaves;
}

internal void draw_leaves(Leaves & leaves, f32 elapsedTime, v2 leafScale = {1,1}, v3 colour = {})
{
	s32 drawCount = min_f32(leaves.capacity, leaves.count);

	m44 * leafTransforms = push_memory<m44>(*global_transientMemory, drawCount, ALLOC_GARBAGE);
	for (s32 i = 0; i < drawCount; ++i)
	{
		v3 position 			= leaves.position + rotate_v3(leaves.rotation, leaves.localPositions[i]);

		constexpr f32 swayRange = 0.5;
		leaves.swayPositions[i] = mod_f32(leaves.swayPositions[i] + elapsedTime, swayRange * 4);
		f32 sway 				= mathfun_pingpong_f32(leaves.swayPositions[i], swayRange * 2) - swayRange;
		quaternion rotation 	= axis_angle_quaternion(leaves.swayAxes[i], sway);
		rotation 				= leaves.localRotations[i] * leaves.rotation * rotation;

		v3 scale 	= make_uniform_v3(leaves.localScales[i]);
		scale.x 	*= leafScale.x;
		scale.y 	*= leafScale.y;

		leafTransforms[i] 		= transform_matrix(	position, rotation,	scale);
	}

	graphics_draw_leaves(platformGraphics, drawCount, leafTransforms, leaves.colourIndex, colour, leaves.material);
}


struct Waters
{
	s32 			capacity;
	s32 			count;
	Transform3D * 	transforms;
	f32	* 			levels;
};

// struct Module
// {
// 	char 	letter;
// 	float 	age;
// 	float 	parameter;
// };

// enum TreeType
// {
// 	TREE_TYPE_1,
// 	TREE_TYPE_2,
// 	TREE_TYPE_CRYSTAL,

// 	TREE_TYPE_COUNT,
// };

// struct TimedLSystem
// {
// 	s32 			wordCapacity;
// 	s32 			wordCount;
// 	Module * 		aWord;
// 	Module * 		bWord;

// 	f32 			timeSinceLastUpdate;

// 	v3 				position;
// 	quaternion 		rotation;

// 	TreeType		type;

// 	f32 totalAge;
// 	f32 maxAge;

// 	// Dynamic mesh
// 	MemoryView<u16> 	indices;
// 	MemoryView<Vertex> 	vertices;

// 	MeshHandle			seedMesh;
// 	MaterialHandle 		seedMaterial;
// };

// // Note(Leo): We can use this instead of separate lambda in each function
// struct AddProductionFunctor
// {
// 	MemoryView<Module> * word;

// 	void operator () (s32 count, Module * production)
// 	{
// 		Assert(word->count + count <= word->capacity);
// 		Assert(false && "obsolete");
// 		// copy_structs(word->memory + word->count, production, count);
// 		word->count += count;
// 	}

// 	void operator () (Module module)
// 	{
// 		this->operator()(1, &module);
// 	}

// 	template<int Count>
// 	void operator() (Module (&production)[Count])
// 	{
// 		this->operator()(Count, production);
// 	}
// };


// internal void advance_lsystem_time(TimedLSystem & lSystem, f32 timePassed)
// {
// 	f32 waterDistanceThreshold = 1;

// 	lSystem.totalAge += timePassed;

// 	Module * aWord 				= lSystem.aWord;
// 	MemoryView<Module> bWord 	= {lSystem.wordCapacity, 0, lSystem.bWord};

// 	auto add_production = AddProductionFunctor{&bWord};

// 	constexpr float growSpeed 	= 0.5f;
// 	constexpr float angle 		= π / 8;

// 	for (int i = 0; i < lSystem.wordCount; ++i)
// 	{
// 		aWord[i].age += timePassed * growSpeed;
		
// 		switch(aWord[i].letter)
// 		{
// 			case 'X':
// 			{
// 				constexpr float terminalAge = 1.0f;
				
// 				if (aWord[i].age > terminalAge)
// 				{
// 					Module production [] =
// 					{
// 						{'F', terminalAge, aWord[i].parameter * terminalAge},
// 						{'<', 0, 5 * π/7},
// 						{'['},
// 							{'&', 0, angle},
// 							{'X', 0, random_range(0.6f, 1.0f)},
// 							{'L', 0, random_range(0.25f, 0.4f)},
// 						{']'},
// 						{'<', 0, 6 * π/7},
// 						{'['},
// 							{'&', 0, angle},
// 							{'X', 0, random_range(0.6f, 1.0f)},
// 							{'L', 0, random_range(0.25f, 0.4f)},
// 						{']'},
// 						// count here = 13

// 						{'<', 0, 4 * π/7},
// 						{'['},
// 							{'&', 0, angle},
// 							{'X', 0, random_range(0.6f, 1.0f)},
// 							{'L', 0, random_range(0.25f, 0.4f)},
// 						{']'},
// 						// count here = 20
// 					};

// 					// Note(Leo) Important: these values must be manually copied from above array
// 					s32 count = random_choice() ? 13 : 20; 

// 					add_production(count, production);
// 				}
// 				else
// 				{
// 					Module production [] =
// 					{
// 						aWord[i],
// 						{'L', 0, random_range(0.25f, 0.4f)},
// 					};
// 					add_production(array_count(production), production);
// 				}
// 			} break;

// 			case 'L':
// 				break;

// 			default:
// 				add_production(aWord[i]);
// 		}
// 	}

// 	swap(lSystem.aWord, lSystem.bWord);
// 	lSystem.wordCount = bWord.count;
// }

// internal void update_lsystem_mesh(TimedLSystem & lSystem, Leaves & leaves)
// {
// 	constexpr v3 lSystemRight 	= right_v3;
// 	constexpr v3 lSystemForward = up_v3;
// 	constexpr v3 lSystemUp 		= -forward_v3;

// 	leaves.count = 0;

// 	struct TurtleState
// 	{
// 		v3 			position;
// 		quaternion 	orientation;
// 	};

// 	int stateIndex 				= 0;
// 	TurtleState states[1000] 	= {};
 
// 	states[0].position 		= {};
// 	states[0].orientation 	= identity_quaternion;

// 	leaves.count = 0;

// 	int wordCount 		= lSystem.wordCount;
// 	Module * word 	= lSystem.aWord;

// 	lSystem.vertices.count = 0;
// 	lSystem.indices.count = 0;

// 	float widthGroth = 0.03f;
// 	constexpr float terminalAge = 1.0f;

// 	for (int i = 0; i < wordCount; ++i)
// 	{
// 		TurtleState & state = states[stateIndex];

// 		switch(word[i].letter)
// 		{
// 			case 'F':
// 			{
// 				v3 forward 				= rotate_v3(state.orientation, lSystemForward);
// 				u16 vertexOffsetBefore 	= lSystem.vertices.count;
// 				f32 width 				= word[i].age * widthGroth;

// 				m44 transform = transform_matrix(state.position, state.orientation, {width, width, 1});

// 				Assert(memory_view_available(lSystem.vertices) >= 8);
// 				s32 & v = lSystem.vertices.count;

// 				lSystem.vertices.memory[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,0}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,0}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,0}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,0}};

// 				state.position 	+= forward * word[i].parameter;

// 				// Note(Leo): Draw upper end slightly thinner, so it matches the following 'X' module
// 				width 			= (word[i].age - terminalAge) * widthGroth;
// 				transform 		= transform_matrix(state.position, state.orientation, {width, width, 1});

// 				lSystem.vertices.memory[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,1}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,1}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,1}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,1}};

// 				constexpr u16 indices [] = 
// 				{
// 					0, 1, 4, 4, 1, 5,
// 					1, 2, 5, 5, 2, 6,
// 					2, 3, 6, 6, 3, 7,
// 					3, 0, 7, 7, 0, 4,
// 				};
// 				Assert(memory_view_available(lSystem.indices) >= array_count(indices));

// 				for(u16 index : indices)
// 				{
// 					lSystem.indices.memory[lSystem.indices.count++] = index + vertexOffsetBefore;
// 				}

// 			} break;

// 			case 'X':
// 			{
// 				Assert(memory_view_available(lSystem.vertices) >= 8);

// 				v3 forward 				= rotate_v3(state.orientation, lSystemForward);
// 				u16 vertexOffsetBefore 	= lSystem.vertices.count;

// 				f32 width 				= word[i].age * widthGroth;
// 				m44 transform 			= transform_matrix(state.position, state.orientation, {width, width, 1});

// 				s32 & v = lSystem.vertices.count;

// 				lSystem.vertices.memory[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,0}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,0}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,0}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,0}};

// 				state.position 	+= forward * word[i].parameter * word[i].age;
				
// 				width 		= (word[i].age / 2) *widthGroth;
// 				transform 	= transform_matrix(state.position, state.orientation, {width, width, 1});

// 				lSystem.vertices.memory[v++] = {multiply_point(transform, {-0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5, -0.5, 0})), {1,1,1}, {0,1}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, { 0.5, -0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5, -0.5, 0})), {1,1,1}, {0.25,1}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, { 0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, { 0.5,  0.5, 0})), {1,1,1}, {0.5,1}};
// 				lSystem.vertices.memory[v++] = {multiply_point(transform, {-0.5,  0.5, 0}), normalize_v3(multiply_direction(transform, {-0.5,  0.5, 0})), {1,1,1}, {0.75,1}};

// 				constexpr u16 indices [] = 
// 				{
// 					0, 1, 4, 4, 1, 5,
// 					1, 2, 5, 5, 2, 6,
// 					2, 3, 6, 6, 3, 7,
// 					3, 0, 7, 7, 0, 4,
// 				};
// 				Assert(memory_view_available(lSystem.indices) >= array_count(indices));

// 				for(u16 index : indices)
// 				{
// 					lSystem.indices.memory[lSystem.indices.count++] = index + vertexOffsetBefore;
// 				}

// 			} break;

// 			case 'L':
// 			{
// 				Assert(leaves.count + 2 < leaves.capacity);

// 				{
// 					quaternion rotation = state.orientation; 
// 					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,lSystemRight), -π / 6);
// 					v3 position 		= state.position + rotate_v3(rotation, lSystemForward) * 0.05f;
					
// 					rotation 			= normalize_quaternion(rotation);

// 					int leafIndex 		= leaves.count;
// 					leaves.count += 1;

// 					leaves.localPositions[leafIndex]	= position;
// 					leaves.localRotations[leafIndex] 	= rotation;
// 					leaves.localScales[leafIndex] 		= word[i].parameter;
					
// 					// Note(Leo): Use normal forward here, since it refers to leaf's own space
// 					leaves.swayAxes[leafIndex]			= rotate_v3(rotation, forward_v3);
// 					leaves.swayPositions[leafIndex] 	= random_value() * 100;	
// 				}

// 				{
// 					quaternion rotation = state.orientation;
// 					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation, lSystemForward), π); 
// 					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,lSystemRight), -π / 6);
// 					v3 position 		= state.position + rotate_v3(rotation, lSystemForward) * 0.05f;

// 					rotation 			= normalize_quaternion(rotation);

// 					int leafIndex = leaves.count;
// 					leaves.count += 1;

// 					leaves.localPositions[leafIndex]	= position;
// 					leaves.localRotations[leafIndex] 	= rotation;
// 					leaves.localScales[leafIndex] 		= word[i].parameter;
					
// 					// Note(Leo): Use normal forward here, since it refers to leaf's own space
// 					leaves.swayAxes[leafIndex]			= rotate_v3(rotation, forward_v3);
// 					leaves.swayPositions[leafIndex]		= random_value() * 100;
// 				}

// 			} break;

// 			case '+':
// 			{
// 				v3 up = rotate_v3(state.orientation, lSystemUp);
// 				state.orientation = state.orientation * axis_angle_quaternion(up, word[i].parameter);
// 			} break;

// 			case '<':
// 			{
// 				v3 forward 			= rotate_v3(state.orientation, lSystemForward);
// 				state.orientation 	= state.orientation * axis_angle_quaternion(forward, word[i].parameter);
// 			} break;

// 			case '&':
// 			{	
// 				v3 right 			= rotate_v3(state.orientation, lSystemRight);
// 				state.orientation 	= state.orientation * axis_angle_quaternion(right, word[i].parameter);
// 			} break;

// 			case '[':
// 			{
// 				states[stateIndex + 1] = states[stateIndex];
// 				stateIndex += 1;
// 			} break;

// 			case ']':
// 			{
// 				stateIndex -= 1;
// 			} break;
// 		}

// 	}

// 	Assert(stateIndex == 0 && "All states have not been popped");

// 	if (lSystem.totalAge > 0 && leaves.count == 0)
// 	{
// 		return;
// 	}
// }

// internal TimedLSystem make_lsystem(MemoryArena & allocator)
// {
// 	// Note(Leo): These are good for now, but they can and should be measured more
// 	// precisely for each individual type of tree.
// 	// Note(Leo): Also max age should be considered
// 	s32 wordCapacity 		= 50'000;
// 	s32 indexCapacity 		= 45'000;
// 	s32 vertexCapacity		= 15'000;

// 	TimedLSystem lSystem 	= {};

// 	lSystem.wordCapacity 	= wordCapacity;
// 	lSystem.wordCount 		= 0;
// 	lSystem.aWord 			= push_memory<Module>(allocator, lSystem.wordCapacity, ALLOC_GARBAGE);
// 	lSystem.bWord 			= push_memory<Module>(allocator, lSystem.wordCapacity, ALLOC_GARBAGE);

// 	Module axiom 		= {'X', 0, 0.7};
// 	lSystem.aWord[0] 		= axiom;
// 	lSystem.wordCount		= 1;

// 	lSystem.vertices 		= push_memory_view<Vertex>(allocator, vertexCapacity);
// 	lSystem.indices 		= push_memory_view<u16>(allocator, indexCapacity);

// 	lSystem.totalAge 		= 0;
// 	lSystem.maxAge 			= 15;

// 	return lSystem;
// }

// /*

// A: apex
// L: leaf
// S: stem
// //B: also stem, but no leaves

// A 		-> SLA 
// S 		-> S
// L(t) : t < 1 -> L(t + n)
// L(t) : t > 1 : chance(0.1) -> [B]
// L(t) : t > 1 : chance(1.0) -> -

// 0: A
// 1: SLA
// 2: SLSLA
// 3: SLSLSLA

// -------------------------------------------------

// A: apex
// B: branch
// F: forward, trunk

// A(t) : t < 1 -> A(t + n)L
// A(t) : t > 1 -> F(1)[B(0)L]A(0)L
// B(t) -> B(t + n)L
// F(t) -> F(t + n)
// L -> -

// 0: 		A(0)
// 0.99: 	A(0.99)L
// 1: 		F(1)[B(0)L]A(0)L
// 1.5: 	F(1.5)[B(0.5)]A(0.5)L
// 2:		F(2)[B(1)L]F(1)[B(0)L]A(0.5)L 


// -------------------------------------------------

// A(n, t): 	apex
// B(n, t):	secondary apex for non-lasting branch
// L(t): 		leaf


// A(n,t) 	: n > 0 -> FL(t - dt)A(n - 1, t - dt)
// 		: t < 0 -> [A(5,2)][A(5,2)]

// B(n,t) 	: n > 0 -> FL(t - dt)B(n - 1, t - dt)


// L(t)	chance(0.5) : t < 0 -> [B(5,2)]

// 0: 	A(5,2.0)
// 1: 	FL(1.8)A(4,1.8)
// 2: 	FL(1.6)FL(1.6)A(3,1.6)
// 3: 	FL(1.4)FL(1.4)FL(1.4)A(2,1.4)
// 4: 	FL(1.2)FL(1.2)FL(1.2)FL(1.2)A(1,1.2)
// 5: 	FL(1.0)FL(1.0)FL(1.0)FL(1.0)FL(1.0)A(0,1.0)
// 6: 	FL(0.8)FL(0.8)FL(0.8)FL(0.8)FL(0.8)A(0,0.8)
// 7: 	FL(0.6)FL(0.6)FL(0.6)FL(0.6)FL(0.6)A(0,0.6)
// 8: 	FL(0.4)FL(0.4)FL(0.4)FL(0.4)FL(0.4)A(0,0.4)
// 9: 	FL(0.2)FL(0.2)FL(0.2)FL(0.2)FL(0.2)A(0,0.2)
// 10: F[B(5,2.0)L(0)F[B(5,2.0)L(0)F[B(5,2.0)L(0)F[B(5,2.0)L(0)F[B(5,2.0)L(0)[A(4,2.0)][A(4,2.0)]


// */

// namespace lsystem_tree2_alphabet
// {
// 	enum : u8
// 	{
// 		PUSH_STATE,
// 		POP_STATE,
// 		ROLL,
// 		PITCH,
// 		YAW,
// 		ORIENT_UP,
// 		ORIENT_DOWN,

// 		LEAF,
// 		STEM 	= 'F',
// 		APEX 	= 'A',
// 		BUD 	= 'B',
// 		APEX_2,
// 		STEM_2,
// 		BUD_2,
// 	};
// }

// internal void advance_lsystem_time_tree2(TimedLSystem & lSystem, f32 timePassed)
// {
// 	using namespace lsystem_tree2_alphabet;

// 	auto push 			= []() {return Module {PUSH_STATE};};
// 	auto pop 			= []() {return Module {POP_STATE};};
// 	auto roll 			= [](f32 angle) {return Module {ROLL, 0, angle};};
// 	auto pitch 			= [](f32 angle) {return Module {PITCH, 0, angle};};
// 	auto yaw 			= [](f32 angle) {return Module {YAW, 0, angle};};
// 	auto orient_up 		= [](f32 amount) {return Module {ORIENT_UP, 0, amount};};
// 	auto orient_down 	= [](f32 amount) {return Module {ORIENT_DOWN, 0, amount};};

// 	auto leaf = []() {return Module {LEAF, 0, random_range(0.25f, 0.4f)};}; 

// 	f32 waterDistanceThreshold = 1;

// 	lSystem.totalAge += timePassed;

// 	Module * aWord 				= lSystem.aWord;
// 	MemoryView<Module> bWord 	= {lSystem.wordCapacity, 0, lSystem.bWord};

// 	constexpr f32 growSpeed 		= 0.5f;
// 	constexpr f32 terminalAge 		= 0.4f;
// 	constexpr f32 angle 			= π / 8;

// 	auto add_production = AddProductionFunctor{&bWord};

// 	for (int i = 0; i < lSystem.wordCount; ++i)
// 	{
// 		aWord[i].age += timePassed * growSpeed;
		
// 		switch(aWord[i].letter)
// 		{
// 			case APEX:
// 			{
// 				if (aWord[i].age > terminalAge * random_range(0.8, 1.0))
// 				{
// 					if(aWord[i].parameter < 5)
// 					{
// 						Module production [] =
// 						{
// 							orient_up(random_range(0.05, 0.25)),
// 							{STEM, terminalAge, 0},
// 							roll(5 * π/7),
// 							{BUD},
// 							{APEX, 0, aWord[i].parameter + 1},
// 							leaf(),
// 						};
// 						add_production(production);
// 					}
// 					// Branching time
// 					else
// 					{ 
// 						if (random_value() < 0.6)
// 						{
// 							Module production [] =
// 							{
// 								push(),
// 									pitch(π/5),
// 									{APEX, 0, 0},
// 									leaf(),
// 								pop(),

// 								pitch(-π/5),
// 								{APEX, 0, 0},
// 								leaf(),
// 							};
// 							add_production(production);
// 						}
// 						else
// 						{
// 							Module production []=
// 							{
// 								{APEX, 0, 0},
// 								leaf(),
// 							};
// 							add_production(production);
// 						}
// 					}
// 				}
// 				else
// 				{
// 					Module production [] =
// 					{
// 						aWord[i],
// 						leaf(),
// 					};
// 					add_production(production);
// 				}
// 			} break;

// 			case BUD:
// 			{
// 				constexpr f32 terminalAge = 4;

// 				if(aWord[i].age < terminalAge)
// 				{
// 					Module production [] =
// 					{
// 						aWord[i],
// 						leaf(),
// 					};
// 					add_production(production);
// 				}
// 				else
// 				{
// 					Module production [] =
// 					{
// 						push(),
// 							pitch(2 * π / 5),
// 							{APEX_2, 0, 0},
// 						pop(),
// 					};
// 					add_production(production);
// 				}
// 			} break;

// 			case BUD_2:
// 			{
// 				constexpr f32 terminalAge = 4;

// 				if (aWord[i].age < terminalAge)
// 				{
// 					Module production [] = 
// 					{
// 						aWord[i],
// 						leaf()
// 					};
// 					add_production(production);
// 				}

// 			} break;

// 			case APEX_2:
// 			{
// 				if(aWord[i].parameter < 5)
// 				{
// 					Module production [] =
// 					{
// 						orient_down(random_range(0.05, 0.25)),
// 						{STEM_2, terminalAge, terminalAge * 5},
// 						roll(5 * π/7),
// 						{BUD_2},
// 						{APEX_2, 0, aWord[i].parameter + 1},
// 						leaf(),
// 					};
// 					add_production(production);
// 				}
// 			} break;

// 			case STEM_2:
// 			{	
// 				aWord[i].parameter -= growSpeed * timePassed;
// 				if (aWord[i].parameter > 0)
// 				{
// 					add_production(aWord[i]);

// 				}
// 			} break;

// 			case LEAF:
// 				break;

// 			default:
// 				add_production(aWord[i]);
// 				break;
// 		}
// 	}

// 	swap(lSystem.aWord, lSystem.bWord);
// 	lSystem.wordCount = bWord.count;
// }

// internal void update_lsystem_mesh_tree2(TimedLSystem & lSystem, Leaves & leaves)
// {
// 	struct TurtleState
// 	{
// 		v3 			position;
// 		quaternion 	orientation;

// 		f32 	textureHeight;

// 		bool 	newBranchBottom;
// 		u16 	currentVertexIndices [5];
// 	};

// 	auto transform_vertex = [](m44 const & transform, v3 position, v3 normal, v2 texcoord) -> Vertex
// 	{
// 		Vertex vertex =
// 		{
// 			.position 	= multiply_point(transform, position),
// 			.texCoord 	= texcoord,
// 			.color 		= {1,1,1},
// 		};

// 		return vertex;
// 	};

// 	auto add_vertices = [transform_vertex](TimedLSystem & lSystem, TurtleState & state, m44 transform)
// 	{	
// 		constexpr s32 vertexCount = 4;
// 		Assert(memory_view_available(lSystem.vertices) >= vertexCount);

// 		s32 & v 	= lSystem.vertices.count;
// 		f32 angle 	= 2 * π / 5;

// 		static const v3 positions [] =
// 		{
// 			{cosine(angle * 0), sine(angle * 0), 0},
// 			{cosine(angle * 1), sine(angle * 1), 0},
// 			{cosine(angle * 2), sine(angle * 2), 0},
// 			{cosine(angle * 3), sine(angle * 3), 0},
// 			{cosine(angle * 4), sine(angle * 4), 0},
// 		};

// 		for (s32 i = 0; i < array_count(positions); ++i)
// 		{
// 			state.currentVertexIndices[i] 	= v;
// 			lSystem.vertices.memory[v++] 		= transform_vertex(transform, positions[i], positions[i], {i * (1.0f / array_count(positions)), state.textureHeight});
// 		}
// 	};

// 	auto add_triangles = [](TimedLSystem & lSystem, u16 (&bottomVertexIndices)[5], u16 (&topVertexIndices)[5])
// 	{
// 		constexpr s32 triangleIndexCount = 6 * 5;
// 		Assert(memory_view_available(lSystem.indices) >= triangleIndexCount);

// 		s32 & i = lSystem.indices.count;

// 		lSystem.indices.memory[i++] = bottomVertexIndices[0],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[1],
// 		lSystem.indices.memory[i++] = topVertexIndices[0],
// 		lSystem.indices.memory[i++] = topVertexIndices[0],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[1],
// 		lSystem.indices.memory[i++] = topVertexIndices[1],

// 		lSystem.indices.memory[i++] = bottomVertexIndices[1],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[2],
// 		lSystem.indices.memory[i++] = topVertexIndices[1],
// 		lSystem.indices.memory[i++] = topVertexIndices[1],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[2],
// 		lSystem.indices.memory[i++] = topVertexIndices[2],
		
// 		lSystem.indices.memory[i++] = bottomVertexIndices[2],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[3],
// 		lSystem.indices.memory[i++] = topVertexIndices[2],
// 		lSystem.indices.memory[i++] = topVertexIndices[2],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[3],
// 		lSystem.indices.memory[i++] = topVertexIndices[3],
		
// 		lSystem.indices.memory[i++] = bottomVertexIndices[3],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[4],
// 		lSystem.indices.memory[i++] = topVertexIndices[3],
// 		lSystem.indices.memory[i++] = topVertexIndices[3],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[4],
// 		lSystem.indices.memory[i++] = topVertexIndices[4];

// 		lSystem.indices.memory[i++] = bottomVertexIndices[4],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[0],
// 		lSystem.indices.memory[i++] = topVertexIndices[4],
// 		lSystem.indices.memory[i++] = topVertexIndices[4],
// 		lSystem.indices.memory[i++] = bottomVertexIndices[0],
// 		lSystem.indices.memory[i++] = topVertexIndices[0];
// 	};


// 	using namespace lsystem_tree2_alphabet;

// 	constexpr v3 lSystemRight 	= right_v3;
// 	constexpr v3 lSystemForward = up_v3;
// 	constexpr v3 lSystemUp 		= -forward_v3;

// 	leaves.count = 0;


// 	int stateIndex 				= 0;
// 	TurtleState states[1000] 	= {};
 
// 	states[0].position 		= {};
// 	states[0].orientation 	= identity_quaternion;

// 	states[0].newBranchBottom = true;

// 	leaves.count = 0;

// 	int wordCount 		= lSystem.wordCount;
// 	Module * word 	= lSystem.aWord;

// 	lSystem.vertices.count = 0;
// 	lSystem.indices.count = 0;

// 	constexpr float widthGroth = 0.02f;
// 	constexpr float heightGrowth = 1.0f;
// 	constexpr float terminalAge = 0.3f;


// 	for (int i = 0; i < wordCount; ++i)
// 	{
// 		TurtleState & state = states[stateIndex];

// 		switch(word[i].letter)
// 		{
// 			case STEM:
// 			case STEM_2:
// 			{
// 				v3 forward 				= rotate_v3(state.orientation, lSystemForward);
// 				u16 vertexOffsetBefore 	= lSystem.vertices.count;
// 				f32 width 				= word[i].age * widthGroth;

// 				m44 transform = transform_matrix(state.position, state.orientation, {width, width, 1});

// 				if (state.newBranchBottom)
// 				{
// 					state.newBranchBottom = false;
// 					add_vertices(lSystem, state, transform);
// 				}

// 				state.position 	+= forward * heightGrowth * terminalAge;// * word[i].parameter;
// 				state.textureHeight += heightGrowth * terminalAge / 2;

// 				// Note(Leo): Draw upper end slightly thinner, so it matches the following 'X' module
// 				width 			= (word[i].age - terminalAge) * widthGroth;
// 				transform 		= transform_matrix(state.position, state.orientation, {width, width, 1});

// 				u16 lastVertexIndices [] = 
// 				{
// 					state.currentVertexIndices[0],
// 					state.currentVertexIndices[1],
// 					state.currentVertexIndices[2],
// 					state.currentVertexIndices[3],
// 					state.currentVertexIndices[4],
// 				};

// 				add_vertices(lSystem, state, transform);
// 				add_triangles(lSystem, lastVertexIndices, state.currentVertexIndices);

// 			} break;

// 			case APEX:
// 			case APEX_2:
// 			{
// 				Assert(memory_view_available(lSystem.vertices) >= 8);

// 				v3 forward 				= rotate_v3(state.orientation, lSystemForward);
// 				u16 vertexOffsetBefore 	= lSystem.vertices.count;

// 				f32 width 				= word[i].age * widthGroth;
// 				m44 transform 			= transform_matrix(state.position, state.orientation, {width, width, 1});

// 				s32 & v = lSystem.vertices.count;

// 				if (state.newBranchBottom)
// 				{
// 					state.newBranchBottom = false;
// 					add_vertices(lSystem, state, transform);
// 				}

// 				state.position 	+= forward * word[i].age * heightGrowth;
				
// 				width 		= (word[i].age / 2) *widthGroth;
// 				transform 	= transform_matrix(state.position, state.orientation, {width, width, 1});

// 				u16 lastVertexIndices [] = 
// 				{
// 					state.currentVertexIndices[0],
// 					state.currentVertexIndices[1],
// 					state.currentVertexIndices[2],
// 					state.currentVertexIndices[3],
// 					state.currentVertexIndices[4],
// 				};


// 				add_vertices(lSystem, state, transform);
// 				add_triangles(lSystem, lastVertexIndices, state.currentVertexIndices);

// 			} break;

// 			case LEAF:
// 			{
// 				Assert(leaves.count + 2 < leaves.capacity);

// 				// v3 forward 	= rotate_v3(state.orientation, lSystemForward);
// 				// v3 right 	= rotate_v3(state.orientation, lSystemRight);

// 				// f32 angle 	= signed_angle_v3(forward, lSystemForward, right);

// 				// quaternion rotationToUpwards = axis_angle_quaternion(right, angle);
			
// 				{

// 					quaternion rotation = identity_quaternion; 
// 					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,lSystemRight), -π / 6);
// 					rotation 			= normalize_quaternion(rotation);

// 					v3 position 		= state.position + rotate_v3(rotation, lSystemForward) * 0.05f;

// 					int leafIndex 		= leaves.count;
// 					leaves.count += 1;

// 					leaves.localPositions[leafIndex]	= position;
// 					leaves.localRotations[leafIndex] 	= rotation;
// 					leaves.localScales[leafIndex] 		= word[i].parameter;
					
// 					// Note(Leo): Use normal forward here, since it refers to leaf's own space
// 					leaves.swayAxes[leafIndex]			= rotate_v3(rotation, forward_v3);
// 					leaves.swayPositions[leafIndex] 	= random_value() * 100;	
// 				}

// 				{
// 					quaternion rotation = identity_quaternion;
// 					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation, lSystemForward), π); 
// 					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,lSystemRight), -π / 6);
// 					rotation 			= normalize_quaternion(rotation);

// 					v3 position 		= state.position + rotate_v3(rotation, lSystemForward) * 0.05f;

// 					int leafIndex = leaves.count;
// 					leaves.count += 1;

// 					leaves.localPositions[leafIndex]	= position;
// 					leaves.localRotations[leafIndex] 	= rotation;
// 					leaves.localScales[leafIndex] 		= word[i].parameter;
					
// 					// Note(Leo): Use normal forward here, since it refers to leaf's own space
// 					leaves.swayAxes[leafIndex]			= rotate_v3(rotation, forward_v3);
// 					leaves.swayPositions[leafIndex]		= random_value() * 100;
// 				}

// 			} break;

// 			case YAW:
// 			{
// 				v3 up = rotate_v3(state.orientation, lSystemUp);
// 				state.orientation = state.orientation * axis_angle_quaternion(up, word[i].parameter);
// 			} break;

// 			case ROLL:
// 			{
// 				v3 forward 			= rotate_v3(state.orientation, lSystemForward);
// 				state.orientation 	= state.orientation * axis_angle_quaternion(forward, word[i].parameter);
// 			} break;

// 			case PITCH:
// 			{	
// 				v3 right 			= rotate_v3(state.orientation, lSystemRight);
// 				state.orientation 	= state.orientation * axis_angle_quaternion(right, word[i].parameter);
// 			} break;

// 			case PUSH_STATE:
// 			{
// 				states[stateIndex + 1] = states[stateIndex];
// 				states[stateIndex + 1].newBranchBottom = true;
// 				stateIndex += 1;
// 			} break;

// 			case POP_STATE:
// 			{
// 				stateIndex -= 1;
// 			} break;

// 			case ORIENT_UP:
// 			{
// 				state.orientation = interpolate_quaternion (state.orientation, identity_quaternion, word[i].parameter);
// 				state.orientation = normalize_quaternion(state.orientation);
// 			} break;

// 			case ORIENT_DOWN:
// 			{
// 				state.orientation = interpolate_quaternion (state.orientation, axis_angle_quaternion(lSystemRight, π), 0.2);
// 				state.orientation = normalize_quaternion(state.orientation);
// 			} break;
// 		}

// 	}

// 	Assert(stateIndex == 0 && "All states have not been popped");

// 	if (lSystem.totalAge > 0 && leaves.count == 0)
// 	{
// 		return;
// 	}
// }

// /*
// struct LSystem1
// {
// 	Module axiom [1] = {{'A', 0.0625f}};

// 	void generate(Module predecessor, int & wordLength, Module * word)
// 	{
// 		float angle = pi / 8;
// 		float scale = 0.0625f;

// 		switch(predecessor.letter)
// 		{
// 			case 'A':
// 			{
// 				Module production [] = 
// 				{
// 					{'['},
// 						{'&', angle},
// 						{'F', scale},
// 						{'L'},
// 						{'A'},
// 					{']'},
// 					{'<', 5 * angle},
// 					{'['},
// 						{'&', angle},
// 						{'F', scale},
// 						{'L'},
// 						{'A'},
// 					{']'},
// 					{'<', 7 * angle},
// 					{'['},
// 						{'&', angle},
// 						{'F', scale},
// 						{'L'},
// 						{'A'},
// 					{']'},
// 				};
// 				memory_copy(word + wordLength, production, sizeof(production));
// 				wordLength += array_count(production);
// 			} break;
		
// 			case 'F':
// 			{
// 				Module production [] = 
// 				{
// 					{'S'},
// 					{'<', 5 * angle},
// 					{'F', scale},
// 				};
// 				memory_copy(word + wordLength, production, sizeof(production));
// 				wordLength += array_count(production);
// 			} break;

// 			case 'S':
// 			{
// 				Module production [] =
// 				{
// 					{'F', scale},
// 					{'L'},
// 				};
// 				memory_copy(word + wordLength, production, sizeof(production));
// 				wordLength += array_count(production);
// 			} break;

// 			case 'L':
// 			{
// 				Module production [] =
// 				{
// 					{'['},
// 						{'^', 2 * angle},
// 						{'f', scale},
// 					{']'},
// 				};
// 				memory_copy(word + wordLength, production, sizeof(production));
// 				wordLength += array_count(production);
// 			} break;

// 			default:
// 				word[wordLength] = predecessor;
// 				wordLength += 1;
// 				break;
// 		}
// 	}
// };

// struct LSystem2
// {
// 	static constexpr float r1 = 0.9f;
// 	static constexpr float r2 = 0.6f;
// 	static constexpr float a0 = π / 4;
// 	static constexpr float a2 = π / 4;
// 	static constexpr float d = to_radians(137.5);
// 	static constexpr float wr = 0.707;

// 	Module axiom [1] = {{'A', 1.0f}};

// 	void generate(Module predecessor, int & wordLength, Module * word)
// 	{
// 		float length = predecessor.argument;

// 		switch(predecessor.letter)
// 		{
// 			case 'A':
// 			{
// 				Module production [] =
// 				{
// 					{'F', length},
// 					{'['},
// 						{'&', a0},
// 						{'B', length * r2},
// 					{']'},
// 					{'<', d},
// 					{'A', length * r1}
// 				};
// 				memory_copy(word + wordLength, production, sizeof(production));
// 				wordLength += array_count(production);
// 			} break;
		
// 			case 'B':
// 			{
// 				Module production [] =
// 				{
// 					{'F', length},
// 					{'['},
// 						{'+', -a2},
// 						{'$'},
// 						{'C', length * r2},
// 					{']'},
// 					{'C', length * r1},
// 				};


// 				memory_copy(word + wordLength, production, sizeof(production));
// 				wordLength += array_count(production);
// 			} break;

// 			case 'C':
// 			{
// 				Module production [] =
// 				{
// 					{'F', length},
// 					{'['},
// 						{'+', a2},
// 						{'$'},
// 						{'B', length * r2},
// 					{']'},
// 					{'B', length * r1}
// 				};
// 				memory_copy(word + wordLength, production, sizeof(production));
// 				wordLength += array_count(production);
// 			} break;

// 			default:
// 				word[wordLength] = predecessor;
// 				wordLength += 1;
// 				break;
// 		}
// 	}
// };
// */