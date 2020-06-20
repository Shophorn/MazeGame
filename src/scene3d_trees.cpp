/*
Leo Tamminen

Trees and leaves.
*/

struct Leaves
{
	s32 	count;
	v3 		position;
	f32 	radius;
	f32 	leafScale;
	s32 	processCount;

	// Todo(Leo): maybe put these into a struct
	v3 * 			localPositions;
	f32 * 			rotations;
	f32 * 			directions;
	v3 * 			axes;
	f32 * 			scales;
	quaternion * 	baseRotations;
};

struct Trees
{
	static constexpr f32 growthRate 		= 0.1f;
	static constexpr f32 waterAcceptDistance 	= 0.4f;

	s32 capacity;
	s32 count;

	Transform3D * transforms;

	// Todo(Leo): maybe put these into a struct
	f32 * growths;
	f32 * waterinesss;

	// f32 vitalities

	Leaves * leaves;

	ModelHandle model;
};

internal Leaves make_leaves(MemoryArena & allocator, s32 count)
{
	Leaves leaves = {};

	leaves.count 		= count;
	// leaves.position = leavesPositions[leavesIndex];
	leaves.radius 		= 10;
	leaves.leafScale 	= 1;

	leaves.localPositions 	= push_memory<v3>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.rotations 		= push_memory<f32>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.directions 		= push_memory<f32>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.axes 			= push_memory<v3>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.scales 			= push_memory<f32>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.baseRotations 	= push_memory<quaternion>(allocator, leaves.count, ALLOC_NO_CLEAR);

	/* Note(Leo): Initialize leaves in order biggest to smallest, so we can easily LOD them
	by limiting number of processed leaves. We do not currently have any such system, but
	this costs zero, so we do it.

	This to my best understanding represent uniform distribution, but it can be made to
	use other distributions as well. */
	f32 biggestScale 	= 1.0f;
	f32 smallestScale 	= 0.5f;
	f32 scaleStep 		= (biggestScale - smallestScale) / leaves.count;
	f32 scale 			= biggestScale;

	Assert(scaleStep > 0 && "Scale step is too small to make any change");

	for (s32 i = 0; i < leaves.count; ++i)
	{
		f32 horizontalAngle	= RandomRange(0, 2 * pi);
		f32 verticalAngle 	= RandomRange(-pi / 2, pi / 2);

		// Todo(Add more weight towards outer limit)
		f32 distance 	= RandomRange(0.6, 1);

		f32 sineHorizontal = sine(horizontalAngle);
		f32 cosineHorizontal = cosine(horizontalAngle);

		f32 x = cosineHorizontal;
		f32 y = sineHorizontal;

		v3 localPosition =
		{
			x * cosine(verticalAngle) * distance,
			y * cosine(verticalAngle) * distance,
			sine(verticalAngle) * distance
		};

		leaves.localPositions[i] = localPosition;

		// Todo(Leo): update names: rotations is current rotation values, directions is just which direction we are
		// rotation, counter clockwise or normal clockwise
		leaves.rotations[i] = RandomRange(-1, 1);
		leaves.directions[i] = Sign(RandomValue());


		// Note(Leo): leaf geometry is defined so that it points along positive y axis, so we rotate accordingly with -pi/2
		quaternion awayFromCenterRotation = axis_angle_quaternion(up_v3, horizontalAngle - pi / 2);

		v3 localYAxis 				= multiply_direction(rotation_matrix(awayFromCenterRotation), {0,1,0});
		v3 localXAxis 				= cross_v3(localYAxis, up_v3);
		f32 localXRotation 			= -pi / 4 + RandomRange(-pi / 12, pi / 12);

		// quaternion 
		leaves.baseRotations[i] 	= awayFromCenterRotation * axis_angle_quaternion(localXAxis, localXRotation);
		leaves.axes[i] 				= multiply_direction(rotation_matrix(leaves.baseRotations[i]), {0,1,0});

		leaves.scales[i] 	= scale;
		scale 				-= scaleStep;
	}

	return leaves;
}

internal void draw_leaves(Trees & trees)
{
	for (s32 leavesIndex = 0; leavesIndex < trees.count; ++leavesIndex)
	{
		Leaves & leaves = trees.leaves[leavesIndex]; 
		s32 drawCount 	= leaves.processCount;

		m44 * leafTransforms = push_memory<m44>(*global_transientMemory, drawCount, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < drawCount; ++i)
		{
			quaternion rotation = axis_angle_quaternion(leaves.axes[i], leaves.rotations[i]);
			rotation 			= leaves.baseRotations[i] * rotation;

			leafTransforms[i] 	= transform_matrix(	leaves.position + leaves.localPositions[i] * leaves.radius, 
													rotation,
													make_uniform_v3(leaves.scales[i] * leaves.leafScale));
		}

		platformApi->draw_meshes(platformGraphics, drawCount, leafTransforms, {-1}, {-1});
	}
}

internal void draw_leaves(Leaves & leaves)
{
	s32 drawCount = math::min(leaves.count, leaves.processCount);

	m44 * leafTransforms = push_memory<m44>(*global_transientMemory, drawCount, ALLOC_NO_CLEAR);
	for (s32 i = 0; i < drawCount; ++i)
	{
		quaternion rotation = axis_angle_quaternion(leaves.axes[i], leaves.rotations[i]);
		rotation 			= leaves.baseRotations[i] * rotation;

		leafTransforms[i] 	= transform_matrix(	leaves.position + leaves.localPositions[i] * leaves.radius, 
												rotation,
												make_uniform_v3(leaves.scales[i] * leaves.leafScale));
	}

	platformApi->draw_meshes(platformGraphics, drawCount, leafTransforms, {-1}, {-1});
}


internal void update_leaves(Trees & trees, f32 elapsedTime)
{
	/// UPDATE LEAVES
	for (s32 i = 0; i < trees.count; ++i)
	{
		constexpr f32 zPosition = 2.6;
		constexpr f32 growthRadiusMultiplier = 1;
		constexpr f32 growthScaleMin = 0.2; 
		constexpr f32 growthScaleMax = 1; 

		constexpr f32 maxGrowth = 20;

		f32 growth 		= trees.growths[i];
		Leaves & leaves = trees.leaves[i];

		leaves.position 	= trees.transforms[i].position + v3{0, 0, growth * zPosition};
		leaves.radius 		= growth * growthRadiusMultiplier;
		leaves.leafScale 	= 0.05f + math::clamp(growth / 5, 0.0f, 1.0f) * 0.95f;

		constexpr s32 minLeavesNumber 	= 2;
		f32 leafCountGrowthAmount 		= math::clamp (growth / 5, 0.0f, 1.0f);
		leaves.processCount 			= minLeavesNumber + leafCountGrowthAmount * (leaves.count - minLeavesNumber);

		for (s32 leafIndex = 0; leafIndex < leaves.processCount; ++leafIndex)
		{
			leaves.rotations[leafIndex] += leaves.directions[leafIndex] * elapsedTime;
			constexpr f32 rotationRange = 0.5;
			// Todo(Leo): make pingpong like functionality and remove direction from Leaves
			if(leaves.rotations[leafIndex] < -rotationRange)
			{
				leaves.directions[leafIndex] = 1;
			}
			else if (leaves.rotations[leafIndex] > rotationRange)
			{
				leaves.directions[leafIndex] = -1;
			}
		}
	}
}

internal void update_leaves(Leaves & leaves, f32 elapsedTime)
{
	for (s32 leafIndex = 0; leafIndex < leaves.processCount; ++leafIndex)
	{
		leaves.rotations[leafIndex] += leaves.directions[leafIndex] * elapsedTime;
		constexpr f32 rotationRange = 0.5;
		// Todo(Leo): make pingpong like functionality and remove direction from Leaves
		if(leaves.rotations[leafIndex] < -rotationRange)
		{
			leaves.directions[leafIndex] = 1;
		}
		else if (leaves.rotations[leafIndex] > rotationRange)
		{
			leaves.directions[leafIndex] = -1;
		}
	}
}

// Note(Leo): wild idea to emulate implicit this-pointer in regular member functions
#define IMPORT_TREES(treesInstance) s32 & count = (treesInstance).count;

internal Trees allocate_trees(MemoryArena & allocator, s32 capacity)
{
	Trees trees 		= {};
	trees.count 		= 0;
	trees.capacity  	= capacity;
	trees.transforms 	= push_memory<Transform3D>(allocator, capacity, 0);
	trees.growths 		= push_memory<f32>(allocator, capacity, 0);
	trees.waterinesss 	= push_memory<f32>(allocator, capacity, 0);
	
	trees.leaves 		= push_memory<Leaves>(allocator, capacity, ALLOC_NO_CLEAR);
	for (s32 i = 0; i < capacity; ++i)
	{
		trees.leaves[i] = make_leaves(allocator, 4000);
	}

	return trees;
}

// Note(Leo): return tree index
internal s32 add_new_tree(Trees & trees, v3 position)
{
	// IMPORT_TREES(trees);

	Assert(trees.count <= trees.capacity);

	s32 index = trees.count;

	trees.transforms[index].position 	= position;
	trees.transforms[index].rotation 	= identity_quaternion;
	trees.transforms[index].scale 		= {0,0,0};

	// Note(Leo): we assume that tree was created with a drop of water
	trees.waterinesss[index] 	= 0.5f;
	trees.growths[index] 		= 0;
	// trees.leavesIndices[index] 	= 0;

	trees.count += 1;
	return index;
}

internal void grow_trees(Trees & trees, f32 maxGrowth, f32 elapsedTime)
{
	for (s32 i = 0; i < trees.count; ++i)
	{
		if (trees.waterinesss[i] > 0)
		{
			f32 growth 				= trees.growthRate * elapsedTime;
			trees.growths[i] 		= math::min(maxGrowth, trees.growths[i] + growth);
			trees.waterinesss[i] 	-= growth;
	
			trees.transforms[i].scale = make_uniform_v3(trees.growths[i]);
		}
	}

	for (s32 i = 0; i < trees.count; ++i)
	{
		debug_draw_circle_xy(	trees.transforms[i].position + v3{0,0,0.5f},
								trees.growths[i] * trees.waterAcceptDistance,
								colour_aqua_blue, DEBUG_NPC);
	}
}


struct TimedModule
{
	char 	letter;
	float 	age;
	float 	parameter;
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

	s32 	linePointsCapacity;
	s32 	linePointsCount;
	v3 * 	linePoints;
};

internal void advance_lsystem_time(TimedLSystem & lSystem, f32 timePassed)
{
	TimedModule * aWord = lSystem.aWord;
	TimedModule * bWord = lSystem.bWord;

	int aWordLength = lSystem.wordCount;
	int bWordLength = 0;

	constexpr float growSpeed = 0.05f;
	constexpr float terminalAge = 1.0f;
	constexpr float angle = π / 8;

	for (int i = 0; i < aWordLength; ++i)
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
						{']'},
						{'<', 0, 6 * π/7},
						{'['},
							{'&', 0, angle},
							{'X', 0, random_range(0.6f, 1.0f)},
						{']'},
						// count here = 11

						{'<', 0, 4 * π/7},
						{'['},
							{'&', 0, angle},
							{'X', 0, random_range(0.6f, 1.0f)},
						{']'},
						// count here = 16
					};

					Assert(bWordLength + array_count(production) <= lSystem.wordCapacity && "LSystem out of memory");
					copy_memory(bWord + bWordLength, production, sizeof(production));

					s32 count = random_choice() ? 11 : 16; 

					bWordLength += count;
				}
				else
				{
					TimedModule production [] =
					{
						aWord[i],
						{'L'},
					};

					Assert(bWordLength + array_count(production) <= lSystem.wordCapacity && "LSystem out of memory");
					
					copy_memory(bWord + bWordLength, production, sizeof(production));
					bWordLength += array_count(production);
				}
			} break;

			case 'L':
				break;

			default:
				Assert(bWordLength + 1 <= lSystem.wordCapacity && "LSystem out of memory");
				bWord[bWordLength++] = aWord[i];
		}
	}

	swap(lSystem.aWord, lSystem.bWord);
	lSystem.wordCount = bWordLength;
}

internal void update_lsystem_mesh(TimedLSystem & lSystem, Leaves & leaves)
{
	leaves.processCount = 0;

	struct TurtleState
	{
		v3 			position;
		quaternion 	orientation;
	};

	int stateIndex 				= 0;
	TurtleState states[1000] 	= {};

	states[0].position 		= lSystem.position;
	states[0].orientation 	= lSystem.rotation;

	// Leaves overall position must be 0, since all individual leaf positions are in lSystem space
	leaves.position 	= {0,0,0};
	leaves.processCount = 0;
	leaves.radius 		= 1;
	leaves.leafScale 	= 1;

	int wordCount 		= lSystem.wordCount;
	TimedModule * word 	= lSystem.aWord;

	// Note(Leo): Wrap this stuff to a stack, so we get easiness for free
	Stack<v3> linePoints = {lSystem.linePointsCapacity, 0, lSystem.linePoints};

	for (int i = 0; i < wordCount; ++i)
	{
		TurtleState & state = states[stateIndex];

		switch(word[i].letter)
		{
			case 'F':
			{
				v3 forward 		= rotate_v3(state.orientation, forward_v3);
				linePoints.push(state.position);
				state.position 	+= forward * word[i].parameter;
				linePoints.push(state.position);

			} break;

			case 'X':
			{
				v3 forward 		= rotate_v3(state.orientation, forward_v3);
				linePoints.push(state.position);
				state.position 	+= forward * word[i].parameter * word[i].age;
				linePoints.push(state.position);
			} break;

			case 'L':
			{
				if (leaves.processCount >= leaves.count)
					break;

				{
					quaternion rotation = state.orientation; 
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,right_v3), π / 3);
					v3 position 		= state.position + rotate_v3(rotation, forward_v3) * 0.05f;
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation,right_v3), π / 3);
					rotation 			= rotation * axis_angle_quaternion(rotate_v3(rotation, forward_v3), π);

					int leafIndex 		= leaves.processCount;
					leaves.processCount += 1;

					leaves.localPositions[leafIndex]	= position;
					leaves.baseRotations[leafIndex] 	= rotation;
					leaves.scales[leafIndex] 			= 0.2f;
					leaves.axes[leafIndex]				= rotate_v3(rotation, forward_v3);
				}

				{
					quaternion rotation = state.orientation;
					rotation = rotation * axis_angle_quaternion(rotate_v3(rotation, forward_v3), π); 
					rotation = rotation * axis_angle_quaternion(rotate_v3(rotation,right_v3), π / 3);
					v3 position = state.position + rotate_v3(rotation, forward_v3) * 0.05f;
					rotation = rotation * axis_angle_quaternion(rotate_v3(rotation,right_v3), π / 3);
					rotation = rotation * axis_angle_quaternion(rotate_v3(rotation, forward_v3), π);


					int leafIndex = leaves.processCount;
					leaves.processCount += 1;

					leaves.localPositions[leafIndex]	= position;
					leaves.baseRotations[leafIndex] 	= rotation;
					leaves.scales[leafIndex] 			= 0.2f;
					leaves.axes[leafIndex]				= rotate_v3(rotation, forward_v3);
				}

			} break;

			case '+':
			{
				v3 up = rotate_v3(state.orientation, up_v3);
				state.orientation = state.orientation * axis_angle_quaternion(up, word[i].parameter);
			} break;

			case '<':
			{
				v3 forward 			= rotate_v3(state.orientation, forward_v3);
				state.orientation 	= state.orientation * axis_angle_quaternion(forward, word[i].parameter);
			} break;

			case '&':
			{	
				v3 right 			= rotate_v3(state.orientation, right_v3);
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

	lSystem.linePointsCount = linePoints.count;
	Assert(stateIndex == 0 && "All states have not been popped");
}

internal TimedLSystem make_lsystem(MemoryArena & allocator, s32 linePointsCapacity, s32 wordCapacity)
{
	TimedLSystem lSystem = {};

	lSystem.wordCapacity = wordCapacity;
	lSystem.wordCount 	= 0;
	lSystem.aWord 		= push_memory<TimedModule>(allocator, lSystem.wordCapacity, ALLOC_NO_CLEAR);
	lSystem.bWord 		= push_memory<TimedModule>(allocator, lSystem.wordCapacity, ALLOC_NO_CLEAR);

	lSystem.linePointsCapacity 	= linePointsCapacity;
	lSystem.linePointsCount 	= 0;
	lSystem.linePoints 			= push_memory<v3>(allocator, lSystem.linePointsCapacity, ALLOC_NO_CLEAR);

	TimedModule axiom 	= {'X', 0, 0.7};
	lSystem.aWord[0] 	= axiom;
	lSystem.wordCount	= 1;

	return lSystem;
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