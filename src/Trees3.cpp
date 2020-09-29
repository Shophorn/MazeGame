/*
Leo Tamminen
Trees version 3

TODO(Leo):
	interpolate beziers with even distances

	- apex branching
	- branch max age
	- simple pruning


	SELF PRUNING:
	Compute incoming light for each branch section, and add them up from child to parent branch.
	Compute a threshold value based on branch section and its cumulative childrens' sizes.
	Any branch that does not meet the threshold is pruned

DONE:
	- leaves from buds age separately from tree growth
*/

// Todo(Leo): This is maybe stupid(as in unnecessary) that this is functor
struct GetWaterFunc
{
	Waters & 	waters;
	s32 		carriedItemIndex;
	bool32 		waterIsCarried;

	f32 operator()(v3 position, f32 requestedAmount)
	{
		s32 closestIndex;
		f32 closestDistance = highest_f32;

		for (s32 i = 0; i < waters.count; ++i)
		{
			if (carriedItemIndex == i && waterIsCarried)
			{
				continue;
			}

			f32 distance = magnitude_v3(waters.transforms[i].position - position);
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closestIndex 	= i;
			}
		}

		f32 amount 				= 0;
		f32 distanceThreshold	= 2;

		if (closestDistance < distanceThreshold)
		{
			waters.levels[closestIndex] -= requestedAmount;
			if (waters.levels[closestIndex] < 0)
			{
				waters.count -= 1;
				waters.transforms[closestIndex] 	= waters.transforms[waters.count];
				waters.levels[closestIndex] 		= waters.levels[waters.count];
			}

			amount = requestedAmount;
		}

		return amount;
	};
};


struct Tree3Node
{
	v3 			position;
	quaternion 	rotation;
	f32 		radius;
};

struct Tree3Branch
{
	s64 startNodeIndex;
	s64 endNodeIndex;
	f32 startNodeDistance;
	
	s64 parentBranchIndex;
	s32 childBranchCount;

	f32 		nextBudPosition;
	quaternion 	nextBudRotation;
	s32 		budIndex;

	bool32	dontGrowLength;
};

struct Tree3Bud
{
	bool32 		hasLeaf;
	s64 		firstLeafIndex;
	f32 		age;

	f32 		distanceFromStartNode;
	s64 		parentBranchIndex;

	// Updated with branch section stuff
	v3 			position;
	quaternion 	rotation;
	f32 		size;
};

struct Tree3Settings
{
	f32 maxHeight 					= 10;
	f32 growSpeedScale 				= 1;
	f32 areaGrowthSpeed 			= 0.05;
	f32 lengthGrowthSpeed 			= 0.3;
	f32 maxHeightToWidthRatio 		= 20;
	
	f32 tangentScale 				= 5;
	
	f32 apexBranchingProbability 	= 0.1;

	f32 budInterval 				= 0.5;
	f32 budIntervalRandomness 		= 0.1;
	f32 budTerminalDistanceFromApex = 1.0;
	f32 budAngle 					= 2.06;
	f32 budAngleRandomness 			= 0.1;
	
	f32 leafMaturationTime 	= 3.0;
	s32 leafColourIndex 	= 0;
	s32 leafCountPerBud 	= 3;
	v2 leafSize 			= {1,1};
	v3 leafColour           = colour_gray.rgb;

	f32 seedDrawSizeThreshold 	= 1;

	f32 waterDrainSpeed 		= 1;
	f32 waterUsageSpeed 		= 0.5;
	f32 waterReservoirCapacity 	= 10;


	static constexpr auto serializedProperties = make_property_list
	(	
		#define SERIALIZE(name) serialize_property(#name, &Tree3Settings::name)

		SERIALIZE(maxHeight),
		SERIALIZE(growSpeedScale),
		SERIALIZE(tangentScale),
		SERIALIZE(areaGrowthSpeed),
		SERIALIZE(lengthGrowthSpeed),
		SERIALIZE(maxHeightToWidthRatio),
		SERIALIZE(budInterval),
		SERIALIZE(budIntervalRandomness),
		SERIALIZE(budTerminalDistanceFromApex),
		SERIALIZE(budAngle),
		SERIALIZE(budAngleRandomness),
		SERIALIZE(leafMaturationTime),
		SERIALIZE(apexBranchingProbability),
		SERIALIZE(leafColourIndex),
		SERIALIZE(leafCountPerBud),
		SERIALIZE(leafSize),
		SERIALIZE(leafColour),
		SERIALIZE(seedDrawSizeThreshold),
		SERIALIZE(waterDrainSpeed),
		SERIALIZE(waterUsageSpeed),
		SERIALIZE(waterReservoirCapacity)

		#undef SERIALIZE
	);
};

struct Tree3
{
	Array2<Tree3Node> 	nodes;
	Array2<Tree3Bud> 	buds;
	Array2<Tree3Branch> branches;

	Leaves 		leaves;
	DynamicMesh mesh;

	MeshHandle 		seedMesh;
	MaterialHandle 	seedMaterial;

	f32 waterReservoir = 0;

	v3 position;
	// Todo(Leo): this is not used yet, make it so that it is
	// quaternion rotation;

	bool32 drawSeed = true;
	bool32 enabled;
	bool32 drawGizmos;
	bool32 breakOnUpdate;

	static bool32 globalEnabled;

	bool32 resourceLimitReached 	= false;
	f32 resourceLimitThresholdValue = 0.8;

	static constexpr f32 fruitMaturationTime = 5; 
	bool32 	hasFruit;
	f32 	fruitAge;
	v3 		fruitPosition;

	s32 			typeIndex;
	Tree3Settings * settings;


	// Todo(Leo): I would like not to include this here, but we do need connection to falling system etc.
	Game * game;
};
bool32 Tree3::globalEnabled = true;


internal void tree_3_add_branch(Tree3 & tree, s64 parentBranchIndex, v3 position, quaternion rotation, f32 distanceFromParentStartNode)
{
	s32 newBranchIndex 				= tree.branches.count++;
	Tree3Branch & newBranch 		= tree.branches[newBranchIndex];

	newBranch.startNodeIndex 			= tree.nodes.count++;
	newBranch.endNodeIndex 				= tree.nodes.count++;
	newBranch.startNodeDistance 		= distanceFromParentStartNode;
	newBranch.parentBranchIndex 		= parentBranchIndex;
	newBranch.childBranchCount 			= 0;
	newBranch.nextBudPosition 			= tree.settings->budInterval;
	newBranch.nextBudRotation 			= rotation;
	newBranch.budIndex 					= tree.buds.count++;

	constexpr f32 nodeStartSize = 0.01;

	tree.nodes[newBranch.startNodeIndex] = 
	{
		.position 	= position,
		.rotation 	= rotation,
		.radius 	= nodeStartSize,
	};

	tree.nodes[newBranch.endNodeIndex] = 
	{
		.position 	= position,
		.rotation 	= rotation,
		.radius 	= nodeStartSize / 2,
	};

	// Todo(Leo): reset new bud here
	Tree3Bud & newBud 		= tree.buds[newBranch.budIndex];
	newBud.hasLeaf	 		= true;
	newBud.firstLeafIndex 	= tree.leaves.count;
	tree.leaves.count 		+= tree.settings->leafCountPerBud;
	newBud.age 				= 0;

	newBud.distanceFromStartNode 	= 0;
	newBud.parentBranchIndex = newBranchIndex;

	// // Updated with branch section stuff
	// v3 			position;
	// quaternion 	rotation;
	// f32 		size;

	tree.branches[parentBranchIndex].childBranchCount += 1;
}

internal void grow_tree_3(Tree3 & tree, f32 elapsedTime, GetWaterFunc & get_water)
{
	elapsedTime *= tree.settings->growSpeedScale;

	f32 waterCapacityAvailable 	= tree.settings->waterReservoirCapacity - tree.waterReservoir; 
	f32 waterDrain 				= min_f32(waterCapacityAvailable, tree.settings->waterDrainSpeed * elapsedTime);
	tree.waterReservoir 		+= get_water(tree.position, waterDrain);
	
	tree.waterReservoir         -= tree.settings->waterUsageSpeed * elapsedTime;
	tree.waterReservoir 		= clamp_f32(tree.waterReservoir, 0, tree.settings->waterReservoirCapacity);


	f32 waterGrowthFactor;
	{
		waterGrowthFactor = clamp_f32(tree.waterReservoir / tree.settings->waterReservoirCapacity, 0, 1);
		waterGrowthFactor = pow_f32(waterGrowthFactor, 0.5);
		// waterGrowthFactor = waterGrowthFactor * waterGrowthFactor;
		// waterGrowthFactor *= -1;
		// waterGrowthFactor += 1;
	}

	s32 branchCountBefore = tree.branches.count;
	for (s32 branchIndex = 0; branchIndex < branchCountBefore; ++branchIndex)
	{
		Tree3Branch & branch 	= tree.branches[branchIndex];

		Tree3Node & startNode 	= tree.nodes[branch.startNodeIndex];
		Tree3Node & endNode 	= tree.nodes[branch.endNodeIndex];

		// GROW WIDTH
		{
			f32 maxRadius = highest_f32;

			if (branch.parentBranchIndex >= 0)
			{
				Tree3Branch & parentBranch = tree.branches[branch.parentBranchIndex];

				s32 parentBranchChildCount 		= parentBranch.childBranchCount;
				f32 parentBranchStartNodeRadius	= tree.nodes[parentBranch.startNodeIndex].radius;

				f32 parentArea 				= π * parentBranchStartNodeRadius * parentBranchStartNodeRadius;
				f32 areaDividedForChildren 	= parentArea / parentBranchChildCount;
				f32 maxRadiurPerChildren 	= square_root_f32(areaDividedForChildren / π);

				maxRadius = maxRadiurPerChildren;
			}

			if (startNode.radius < maxRadius)
			{
				f32 growthAmount 	= tree.settings->areaGrowthSpeed / π * elapsedTime * waterGrowthFactor;
				startNode.radius 	= square_root_f32(startNode.radius * startNode.radius + growthAmount);
			}

			if (branch.dontGrowLength && endNode.radius < maxRadius)
			{
				f32 growthAmount 	= tree.settings->areaGrowthSpeed / π * elapsedTime * waterGrowthFactor;
				endNode.radius 	= square_root_f32(endNode.radius * endNode.radius + growthAmount);
			}
		}

		if (branch.dontGrowLength == false)
		{
			f32 distanceFromStart 	= magnitude_v3(endNode.position - startNode.position);
			f32 distanceFromRoot 	= distanceFromStart + branch.startNodeDistance;

			f32 hFactor 			= clamp_f32(1 - (distanceFromRoot / tree.settings->maxHeight), 0, 1);
			f32 hwRatio 			= distanceFromStart / startNode.radius;
			f32 hwFactor 			= clamp_f32(1 - (hwRatio / tree.settings->maxHeightToWidthRatio), 0, 1);
			f32 growFactor  		= hFactor * hwFactor * tree.settings->lengthGrowthSpeed * elapsedTime;
			growFactor 				*= waterGrowthFactor;

			v3 direction 			= rotate_v3(endNode.rotation, v3_up);
			endNode.position 		+= direction * growFactor;
			
			f32 ratioToNextBud 		= (branch.nextBudPosition - distanceFromStart) / tree.settings->budInterval;

			if (ratioToNextBud < 0)
			{
				// Note(Leo): this is inverted..
				if (random_value() > tree.settings->apexBranchingProbability)
				{
					Assert(tree.buds.has_room_for(1));

					f32 budIntervalRandom 	= 1 + random_range(-tree.settings->budIntervalRandomness, tree.settings->budIntervalRandomness);
					branch.nextBudPosition 	= distanceFromStart + tree.settings->budInterval * budIntervalRandom;

					branch.budIndex = tree.buds.count++;

					tree.buds[branch.budIndex].hasLeaf 			= true;
					tree.buds[branch.budIndex].firstLeafIndex 	= tree.leaves.count;
					tree.leaves.count 							+= tree.settings->leafCountPerBud;

					tree.buds[branch.budIndex].age 				= 0;

					tree.buds[branch.budIndex].rotation  		= branch.nextBudRotation;

					tree.buds[branch.budIndex].distanceFromStartNode  	= distanceFromStart;
					tree.buds[branch.budIndex].parentBranchIndex 		= branchIndex;
					tree.buds[branch.budIndex].size 					= 1;

					v3 branchDirection 		= rotate_v3(endNode.rotation, v3_up);
					f32 axisRotationAngle 	= tree.settings->budAngle + random_range(-tree.settings->budAngleRandomness, tree.settings->budAngleRandomness);
					branch.nextBudRotation 	= branch.nextBudRotation * axis_angle_quaternion(branchDirection, axisRotationAngle);
				}
				else
				{
					s32 newApexBranchesCount = (random_value() < 0.6) ? 2 : 3;

					Assert(tree.branches.has_room_for(newApexBranchesCount));
					Assert(tree.buds.has_room_for(newApexBranchesCount));
					Assert(tree.nodes.has_room_for(newApexBranchesCount * 2));

					for(s32 newBranchIndex = 0; newBranchIndex < newApexBranchesCount; ++newBranchIndex)
					{
						f32 angle 			= newBranchIndex * 2 * π / newApexBranchesCount * (1 + random_range(-0.15, 0.15));
						v3 direction 		= rotate_v3(branch.nextBudRotation, v3_up);
						quaternion rotation = branch.nextBudRotation * axis_angle_quaternion(direction, angle);
						// quaternion rotation = endNode.rotation * axis_angle_quaternion(rotate_v3(endNode.rotation, v3_up), angle);
						v3 axis 			= rotate_v3(rotation, v3_forward);
						rotation 			= rotation * axis_angle_quaternion(axis, 0.5);

						tree_3_add_branch(tree, branchIndex, endNode.position, rotation, distanceFromRoot);
					}

					branch.dontGrowLength = true;
				}
			}
			
			// Note(Leo): these are updated each frame, as the apex bud moves with the tip of the branch
			tree.buds[branch.budIndex].position 	= endNode.position;

		}
	}

	if (tree.nodes[tree.branches[0].startNodeIndex].radius > tree.settings->seedDrawSizeThreshold)
	{
		tree.drawSeed = false;
	}

	for (s32 i = 0; i < tree.buds.count; ++i)
	{
		Tree3Bud & bud = tree.buds[i];

		bud.age += elapsedTime;

		Tree3Node const & branchEndNode = tree.nodes[tree.branches[bud.parentBranchIndex].endNodeIndex];
		f32 distanceFromApex 			= magnitude_v3(bud.position - branchEndNode.position);

		if (bud.hasLeaf && (distanceFromApex > tree.settings->budTerminalDistanceFromApex))
		{
			/// RESET LEAVES
			{			
				bud.hasLeaf = false;

				for(s32 leafIndex = 0; leafIndex < tree.settings->leafCountPerBud; ++leafIndex)
				{
					tree.leaves.localPositions[leafIndex + bud.firstLeafIndex] 	= {};
					tree.leaves.localRotations[leafIndex + bud.firstLeafIndex] 	= {};
					tree.leaves.localScales[leafIndex + bud.firstLeafIndex] 	= 0;
					tree.leaves.swayAxes[leafIndex + bud.firstLeafIndex] 		= {};
				}
			}

			v3 axis 			= rotate_v3(bud.rotation, v3_forward);
			quaternion rotation = bud.rotation * axis_angle_quaternion(axis, 0.25 * π);

			Assert(tree.nodes.has_room_for(2));
			Assert(tree.branches.has_room_for(1));
			Assert(tree.buds.has_room_for(1));

			tree_3_add_branch(tree, bud.parentBranchIndex, bud.position, rotation, bud.distanceFromStartNode);
		}

		if (bud.hasLeaf)
		{
			for(s32 leafIndex = 0; leafIndex < tree.settings->leafCountPerBud; ++leafIndex)
			{
				s32 l = leafIndex + bud.firstLeafIndex;
				tree.leaves.localPositions[l] 	= bud.position;

				f32 angle 						= leafIndex * 2 * π / tree.settings->leafCountPerBud;
				quaternion rotation 			= bud.rotation * axis_angle_quaternion(rotate_v3(bud.rotation, v3_up), angle);
				tree.leaves.localRotations[l] 	= rotation;

				tree.leaves.localScales[l] 		= clamp_f32(bud.age / tree.settings->leafMaturationTime, 0, 1) * bud.size;
				tree.leaves.swayAxes[l] 		= rotate_v3(rotation, v3_forward);
			}
		}
	}

	if (	used_percent(tree.nodes) > tree.resourceLimitThresholdValue
		or 	used_percent(tree.buds) > tree.resourceLimitThresholdValue
		or 	used_percent(tree.branches) > tree.resourceLimitThresholdValue)
	{
		tree.resourceLimitReached = true;
		log_debug(FILE_ADDRESS, "tree resource limit reached");
	}
}

internal void build_tree_3_mesh(Tree3 & tree)
{
	DynamicMesh & mesh 	= tree.mesh;
	Leaves & leaves 	= tree.leaves;

	flush_dynamic_mesh(mesh);

	/*
	DONE:
	number of vertices in a loop to variable,
		->later make it depend on size of node and somehow combine where it changes
	interpolate positions
	interpolate with bezier curves by fixed t-values

	TODO:
	platform graphics based dynamic mesh, so no need to copy each frame
	*/


	// Todo(Leo): make these dynamic based on radius of branch
	constexpr s32 verticesInLoop = 6;
	s32 vertexLoopsInNodeSection = 4;
	f32 loopTStep = 1.0f / vertexLoopsInNodeSection;
	s32 bottomSphereLoops 	= 2;

	local_persist v3 baseVertexPositions[verticesInLoop] = {};
	local_persist bool32 arraysInitialized = false;
	if (arraysInitialized == false)
	{
		arraysInitialized = true;
		f32 angleStep = 2 * π / verticesInLoop;

		for (s32 i = 0; i < verticesInLoop; ++i)
		{
			baseVertexPositions[i].xy 	= rotate_v2({1 ,0}, i * angleStep);
		}
	}


	/* DOCUMENT (Leo):
	1. generate first vertices outside of the loop
	2. generate other vertices interpolating between nodes
		always assuming(correctly) that there exists a vertex loop from previous node section
		
		Each section interpolates from t > 0 to t == 1.

	*/

	for (s32 branchIndex = 0; branchIndex < tree.branches.count; ++branchIndex)
	// for (s32 branchIndex = 0; branchIndex < 1; ++branchIndex)
	{
		Tree3Node const & startNode = tree.nodes[tree.branches[branchIndex].startNodeIndex];
		Tree3Node const & endNode 	= tree.nodes[tree.branches[branchIndex].endNodeIndex];

		/// BOTTOM DOME
		{
			Assert(mesh.vertices.has_room_for(bottomSphereLoops * verticesInLoop + 1));
			Assert(mesh.indices.has_room_for((2 * bottomSphereLoops * verticesInLoop - 1) * 3));

			v3 nodePosition 		= startNode.position;
			quaternion nodeRotation = startNode.rotation;
			f32 radius 				= startNode.radius;

			v3 downDirection 		= rotate_v3(nodeRotation, -v3_up);

			s32 bottomVertexIndex = mesh.vertices.count++;
			mesh.vertices[bottomVertexIndex] = { .position = nodePosition + downDirection * radius };

			f32 fullAngle 			= 0.5f * π;
			f32 angleStep 			= fullAngle / bottomSphereLoops;

			for (s32 loopIndex = 0; loopIndex < bottomSphereLoops; ++loopIndex)
			{
				f32 angle 	= fullAngle - (loopIndex + 1) * angleStep;
				f32 sin 	= sine(angle);
				f32 cos 	= cosine(angle);

				s32 verticesStartCount = mesh.vertices.count;

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					v3 pos = rotate_v3(nodeRotation, baseVertexPositions[i]) * radius * cos;
					mesh.vertices[mesh.vertices.count++] = {.position = pos + nodePosition + downDirection * sin * radius};
				}

				if (loopIndex == 0)
				{
					for(s32 i = 0; i < verticesInLoop; ++i)
					{
						mesh.indices[mesh.indices.count++] = bottomVertexIndex;
						mesh.indices[mesh.indices.count++] = verticesStartCount + ((i + 1) % verticesInLoop);
						mesh.indices[mesh.indices.count++] = verticesStartCount + i;
					}
				}
				else
				{
					for (s32 i = 0; i < verticesInLoop; ++i)
					{
						mesh.indices[mesh.indices.count++] = i + verticesStartCount - verticesInLoop;
						mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount - verticesInLoop;
						mesh.indices[mesh.indices.count++] = i + verticesStartCount;

						mesh.indices[mesh.indices.count++] = i + verticesStartCount;
						mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount - verticesInLoop;
						mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount;
					}
				}
			}
		}

		{
			// Todo(Leo): expose in tree editor once we have that
			// Note(Leo): this can be tweaked for artistic purposes
			f32 tangentScale 			= tree.settings->tangentScale;

			f32 maxTangentLength 		= magnitude_v3(startNode.position - endNode.position) / 3;
			f32 previousTangentLength 	= min_f32(maxTangentLength, startNode.radius * tangentScale);
			f32 nextTangentLength 		= min_f32(maxTangentLength, endNode.radius * tangentScale);

			v3 bezier0 = startNode.position;
			v3 bezier1 = startNode.position + previousTangentLength * rotate_v3(startNode.rotation, v3_up);
			v3 bezier2 = endNode.position - nextTangentLength * rotate_v3(endNode.rotation, v3_up);
			v3 bezier3 = endNode.position;

			auto bezier_lerp_v3 = [&](f32 t) -> v3
			{
				v3 b01 = lerp_v3(bezier0, bezier1, t);
				v3 b12 = lerp_v3(bezier1, bezier2, t);
				v3 b23 = lerp_v3(bezier2, bezier3, t);

				v3 b012 = lerp_v3(b01, b12, t);
				v3 b123 = lerp_v3(b12, b23, t);

				v3 b0123 = lerp_v3(b012, b123, t);

				return b0123;
			};

			Assert(mesh.vertices.has_room_for(vertexLoopsInNodeSection * verticesInLoop));
			Assert(mesh.indices.has_room_for(vertexLoopsInNodeSection * verticesInLoop * 6));

			for (s32 loopIndex = 0; loopIndex < vertexLoopsInNodeSection; ++loopIndex)
			{
				f32 t 				= (loopIndex + 1) * loopTStep;
				v3 position 		= bezier_lerp_v3(t);
				f32 radius 			= lerp_f32(startNode.radius, endNode.radius, t);

				// Todo(Leo): maybe bezier slerp this too, but let's not bother with that right now
				quaternion rotation = slerp_quaternion(startNode.rotation, endNode.rotation, t);

				s32 verticesStartCount = mesh.vertices.count;

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					v3 vertexPosition = rotate_v3(rotation, baseVertexPositions[i] * radius) + position;
					mesh.vertices[mesh.vertices.count++] = {.position = vertexPosition};
				}

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					mesh.indices[mesh.indices.count++] = ((i + 0) % verticesInLoop) + verticesStartCount - verticesInLoop; 
					mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount - verticesInLoop; 
					mesh.indices[mesh.indices.count++] = ((i + 0) % verticesInLoop) + verticesStartCount; 

					mesh.indices[mesh.indices.count++] = ((i + 0) % verticesInLoop) + verticesStartCount; 
					mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount - verticesInLoop; 
					mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount; 
				}
			}

		}
	
		/// TOP DOME
		{
			s32 topDomeLoops = 3;

			Assert(mesh.vertices.has_room_for(topDomeLoops * verticesInLoop + 1));
			// Assert(mesh.indices.has_room_for(((2 * topDomeLoops + 1) * verticesInLoop - 1) * 3));

			v3 position 		= endNode.position;
			quaternion rotation = endNode.rotation;
			f32 radius 			= endNode.radius;

			v3 upDirection = rotate_v3(rotation, v3_up);


			f32 fullAngle 			= 0.5f * π;
			f32 angleStep 			= fullAngle / (topDomeLoops + 1);

			for (s32 loopIndex = 0; loopIndex < topDomeLoops; ++loopIndex)
			{
				f32 angle 	= (loopIndex + 1) * angleStep;
				f32 sin 	= sine(angle);
				f32 cos 	= cosine(angle);

				s32 baseVertexIndex = mesh.vertices.count - verticesInLoop;

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					v3 vertexPosition = rotate_v3(rotation, baseVertexPositions[i]) * radius * cos;
					vertexPosition += sin * upDirection * radius + position;
					mesh.vertices[mesh.vertices.count++] = { .position = vertexPosition };
				}

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					mesh.indices[mesh.indices.count++] = baseVertexIndex + i;
					mesh.indices[mesh.indices.count++] = baseVertexIndex + ((i + 1) % verticesInLoop);
					mesh.indices[mesh.indices.count++] = baseVertexIndex + i + verticesInLoop;

					mesh.indices[mesh.indices.count++] = baseVertexIndex + i + verticesInLoop;
					mesh.indices[mesh.indices.count++] = baseVertexIndex + ((i + 1) % verticesInLoop);
					mesh.indices[mesh.indices.count++] = baseVertexIndex + ((i + 1) % verticesInLoop) + verticesInLoop;
				}
			}

			{
				s32 baseVertexIndex 			= mesh.vertices.count - verticesInLoop;
				v3 topVertexPosition 			= position + upDirection * radius;
				s32 topVertexIndex 				= mesh.vertices.count++;
				mesh.vertices[topVertexIndex] 	= { .position = topVertexPosition };

				for (s32 v = 0; v < verticesInLoop; ++v)
				{
					mesh.indices[mesh.indices.count++] = baseVertexIndex + v;
					mesh.indices[mesh.indices.count++] = baseVertexIndex + ((v + 1) % verticesInLoop);
					mesh.indices[mesh.indices.count++] = topVertexIndex;
				}
			}
		}

	}

	mesh_generate_normals(mesh.vertices.count, mesh.vertices.memory, mesh.indices.count, mesh.indices.memory);
	// mesh_generate_tangents(mesh.vertexCount, mesh.vertices, mesh.indexCount, mesh.indices);


	if (	used_percent(mesh.vertices) > tree.resourceLimitThresholdValue 
		or 	used_percent(mesh.indices) > tree.resourceLimitThresholdValue)
	{
		tree.resourceLimitReached = true;
		log_debug(FILE_ADDRESS, "tree resource limit reached");
	}
}

internal void reset_test_tree3(Tree3 & tree)
{
	// Note(Leo): arrays are initialized to 0
	clear_array_2(tree.nodes);
	clear_array_2(tree.buds);
	clear_array_2(tree.branches);

	flush_leaves(tree.leaves);
	flush_dynamic_mesh(tree.mesh);

	tree_3_add_branch(tree, -1, {0,0,0}, identity_quaternion, 0);

	tree.resourceLimitReached 	= false;
	tree.drawSeed 				= true;
	tree.waterReservoir 		= 0;
	tree.enabled 				= true;

	build_tree_3_mesh(tree);
}

internal void initialize_test_tree_3(MemoryArena & allocator, Tree3 & tree, v3 position, Tree3Settings * settings)
{
	tree = {};

	tree.nodes 		= push_array_2<Tree3Node>(allocator, 1000, ALLOC_ZERO_MEMORY);
	tree.buds 		= push_array_2<Tree3Bud>(allocator, 1000, ALLOC_ZERO_MEMORY);
	tree.branches 	= push_array_2<Tree3Branch>(allocator, 1000, ALLOC_ZERO_MEMORY);
	tree.leaves 	= make_leaves(allocator, 2000);
	tree.mesh 		= push_dynamic_mesh(allocator, 10000, 40000);

	tree.position = position;
	tree.settings = settings;

	reset_test_tree3(tree);
}

internal void tree_gui(Tree3 & tree)
{
	gui_toggle("Global Enable", &Tree3::globalEnabled);
	gui_toggle("Enable Updates", &tree.enabled);
	gui_toggle("Draw Gizmos", &tree.drawGizmos);
	gui_float_field("Grow Speed Scale", &tree.settings->growSpeedScale);

	if (gui_button("Reset tree"))
	{
		reset_test_tree3(tree);
	}

	gui_line();

	// gui_text("PROPERTIES");
	static bool32 showProperties = false;
	gui_toggle("PROPERTIES", &showProperties);
	if (showProperties)
	{
		gui_float_field("Area Grow Speed", &tree.settings->areaGrowthSpeed, {.min = 0});
		gui_float_field("Length Grow Speed", &tree.settings->lengthGrowthSpeed, {.min = 0});
		gui_float_field("Tangent Scale", &tree.settings->tangentScale, {.min = 0});
		gui_float_field("Max Height", &tree.settings->maxHeight, {.min = 0});
		gui_float_field("Max Height To Width Ratio", &tree.settings->maxHeightToWidthRatio, {.min = 0});

		gui_float_field("Bud Interval", &tree.settings->budInterval, {.min = 0});
		gui_float_slider("Bud Interval Randomness", &tree.settings->budIntervalRandomness, 0, 1);
		gui_float_field("Bud Angle", &tree.settings->budAngle, {.min = 0, .max = 2*π});
		gui_float_slider("Bud Angle Randomness", &tree.settings->budAngleRandomness, 0, 1);
		gui_float_field("Bud Terminal Distance From Apex", &tree.settings->budTerminalDistanceFromApex, {.min = 0});

		gui_float_slider("Apex Branching Probability", &tree.settings->apexBranchingProbability, 0, 1);
	}

	gui_line();

	// static bool32 showLeaves = false;
	// if (gui_block("LEAVES", &showLeaves))
	
	static bool32 showLeaves = false;
	gui_toggle("LEAVES", &showLeaves);
	if (showLeaves)
	{
		gui_float_field("Leaf Maturation Time", &tree.settings->leafMaturationTime, {.min = 0});
		gui_colour_rgb("Leaf Colour", &tree.settings->leafColour);


		gui_int_field("Leaf Count Per Bud", &tree.settings->leafCountPerBud, {.min = 0});
		gui_vector_2_field("Leaf Size", &tree.settings->leafSize, {.min = {0,0}});
	}
	gui_line();

	static bool32 showWater = false;
	gui_toggle("WATER", &showWater);
	if (showWater)
	{
		gui_float_field("Water Drain Speed", &tree.settings->waterDrainSpeed, {.min= 0});
		gui_float_field("Water Usage Speed", &tree.settings->waterUsageSpeed, {.min= 0});
		gui_float_field("Water Reservoir Capacity", &tree.settings->waterReservoirCapacity, {.min= 0});
	}


	gui_line();

	static bool32 showStats = false;
	gui_toggle("STATS", &showStats);
	if (showStats)
	{
		s32 budCount 				= tree.buds.count;
		s32 vertexCount 			= tree.mesh.vertices.count;
		s32 indexCount 				= tree.mesh.indices.count;
		s32 leafCount 				= tree.leaves.count;
		bool32 resourceLimitReached = tree.resourceLimitReached;

		gui_int_field("Buds", &budCount);
		gui_int_field("Vertices", &vertexCount);
		gui_int_field("Indices", &indexCount);
		gui_int_field("Leaves", &leafCount);
		gui_toggle("Resources reached", &resourceLimitReached);

		f32 waterReservoir = tree.waterReservoir;
		gui_float_field("Water", &waterReservoir);
	}

	gui_line();

	gui_toggle("Break On Update", &tree.breakOnUpdate);
}

internal void update_tree_3(Tree3 & tree, f32 elapsedTime, GetWaterFunc & get_water)
{
	if (tree.breakOnUpdate)
	{
		tree.breakOnUpdate = false;
	}

	if (Tree3::globalEnabled && tree.enabled && !tree.resourceLimitReached)
	{
		grow_tree_3(tree, elapsedTime, get_water);
		build_tree_3_mesh(tree);
	}

	if (tree.resourceLimitReached)
	{
		if (tree.hasFruit == false)
		{
			tree.hasFruit 		= true;
			tree.fruitPosition 	= {3,3,3};
			tree.fruitAge 		= 0;
		}

		tree.fruitAge += elapsedTime;

		if (tree.fruitAge > tree.fruitMaturationTime)
		{
			game_spawn_tree(*tree.game, tree.fruitPosition + tree.position, tree.typeIndex);

			s32 budIndex = random_range(0, tree.buds.count);
			v3 fruitPosition = tree.buds[budIndex].position;

			tree.fruitPosition 	= fruitPosition;
			tree.fruitAge 		= 0;
		}
	}

	if (tree.drawGizmos)
	{
		for (auto const & branch : tree.branches)
		{
			FS_DEBUG_ALWAYS(debug_draw_circle_xy(tree.nodes[branch.endNodeIndex].position + tree.position, 0.6, colour_bright_yellow));
		}

		// for (auto const & bud : tree.buds)
		// {
		// 	if (bud.hasLeaf)
		// 	{
		// 		FS_DEBUG_ALWAYS(debug_draw_circle_xy(bud.position + tree.position, 0.5, colour_bright_green));
		// 	}
		// 	else
		// 	{
		// 		FS_DEBUG_ALWAYS(debug_draw_circle_xy(bud.position + tree.position, 0.5, colour_bright_red));
		// 	}
		// }
	}	
}