/*=============================================================================
Leo Tamminen

Mesh Loader

Todo(Leo):
	- use reference variables for rapidjson things
=============================================================================*/

template <typename T> internal T const * 
get_buffer_start(GltfFile const & file, u32 accessor)
{
	s32 viewIndex = file.json["accessors"][accessor]["bufferView"].GetInt();
	s32 offset = file.json["bufferViews"][viewIndex]["byteOffset"].GetInt();

	T const * start = reinterpret_cast<T const *> (file.binary() + offset);
	return start;
}

s32 find_bone_by_node(JsonArray & bones, int nodeIndex)
{
	for (int boneIndex = 0; boneIndex < bones.Size(); ++boneIndex)
	{
		if (nodeIndex == bones[boneIndex].GetInt())
			return boneIndex;
	}

	return -1;
}

s32 index_by_name(JsonArray & nodes, char const * name)
{
	for (s32 i = 0; i < nodes.Size(); ++i)
	{
		if (nodes[i].HasMember("name") && cstring_equals(nodes[i]["name"].GetString(), name))
		{
			return i;
		}
	}
	return -1;
}

internal Animation
load_animation_glb(MemoryArena & allocator, GltfFile const & file, char const * animationName)
{
	DEBUG_ASSERT(file.json.HasMember("animations"), "No Animations found");

	auto animArray 		= file.json["animations"].GetArray();
	s32 animationIndex 	= index_by_name(animArray, animationName);

	if(animationIndex < 0)
	{
		auto log = logDebug();
		log << "Animation not found. Requested: " << animationName << ", available:\n";
		for (auto const & anim : animArray)
		{
			log << "\t" << anim["name"].GetString() << "\n";
		}
	}
	Assert(animationIndex >= 0);
	JsonValue const & namedAnimation = animArray[animationIndex];

	auto jsonChannels 	= animArray[animationIndex]["channels"].GetArray();
	auto samplers 		= animArray[animationIndex]["samplers"].GetArray();

	auto accessors 		= file.json["accessors"].GetArray();

	Animation result = {};
	result.channels = allocate_array<AnimationChannel>(allocator, jsonChannels.Size());

	float minTime = math::highest_value<float>;
	float maxTime = math::lowest_value<float>;

	for (auto const & jsonChannel : jsonChannels)
	{
		/* Note(Leo): input refers to keyframe times and output to actual values,
		i.e. the input and output of interpolation function for animation. */
		int samplerIndex 	= jsonChannel["sampler"].GetInt();
		int inputAccessor 	= samplers[samplerIndex]["input"].GetInt();
		int outputAccessor 	= samplers[samplerIndex]["output"].GetInt();

		minTime = math::min(minTime, accessors[inputAccessor]["min"][0].GetFloat());
		maxTime = math::max(maxTime, accessors[inputAccessor]["max"][0].GetFloat());


		// ------------------------------------------------------------------------------

		InterpolationMode 	interpolationMode;
		ChannelType 		channelType;
		{
			char const * interpolationModeStr = samplers[samplerIndex]["interpolation"].GetString();

			if(cstring_equals(interpolationModeStr, "STEP"))				interpolationMode = INTERPOLATION_MODE_STEP;
			else if (cstring_equals(interpolationModeStr, "LINEAR"))		interpolationMode = INTERPOLATION_MODE_LINEAR;
			else if (cstring_equals(interpolationModeStr,"CUBICSPLINE"))	interpolationMode = INTERPOLATION_MODE_CUBICSPLINE;
			else
				Assert(false);
		
			char const * path 	= jsonChannel["target"]["path"].GetString();

			if (cstring_equals(path, "translation"))	channelType = ANIMATION_CHANNEL_TRANSLATION;
			else if(cstring_equals(path, "rotation"))	channelType = ANIMATION_CHANNEL_ROTATION;
			else if (cstring_equals(path, "scale"))		channelType = ANIMATION_CHANNEL_SCALE;
			else
				Assert(false);
		}

		// ------------------------------------------------------------------------------

		bool32 supportedChannel = 	(channelType == ANIMATION_CHANNEL_TRANSLATION)
									|| (channelType == ANIMATION_CHANNEL_ROTATION);


		if (supportedChannel == false)
		{
			continue;
		}

		AnimationChannel & channel 	= *result.channels.push_return_pointer({});
		channel.type 				= channelType;
		channel.interpolationMode 	= interpolationMode;


		{
			/* Note(Leo): Bones (or joints) are somewhat cumbersomely behind different list, so we need to remap those.
			Probably the best idea would be to have skeleton reference here too. */
			Assert(file.json.HasMember("skins") && file.json["skins"].Size() == 1 && file.json["skins"][0].HasMember("joints"));

			auto bonesArray 	= file.json["skins"][0]["joints"].GetArray();
			s32 targetNode 		= jsonChannel["target"]["node"].GetInt();
			channel.targetIndex = find_bone_by_node(bonesArray, targetNode);
		
			Assert(channel.targetIndex >= 0);
		}
		
		// ------------------------------------------------------------------------------

		int keyframeCount = accessors[inputAccessor]["count"].GetInt();

		float const * keyframeStart = get_buffer_start<float>(file, inputAccessor);
		float const * keyframeEnd 	= keyframeStart + keyframeCount;
		channel.times 				= allocate_array<float>(allocator, keyframeStart, keyframeEnd);

		/* Note(Leo): CUBICSPLINE interpolation has three values in total: inTangent,
		splineVertex(aka actual keyframe value) and outTangent. */
		s32 valueCount = channel.interpolationMode == INTERPOLATION_MODE_CUBICSPLINE
						? keyframeCount * 3
						: keyframeCount;

		switch(channel.type)
		{
			case ANIMATION_CHANNEL_TRANSLATION:
			{
				logDebug(1) << "translation channel for bone " << channel.targetIndex;

				v3 const * start 		= get_buffer_start<v3>(file, outputAccessor);
				v3 const * end 			= start + valueCount;
				channel.translations	= allocate_array<v3>(allocator, start, end);
			} break;

			case ANIMATION_CHANNEL_ROTATION:
			{
				logDebug(1) << "rotation channel for bone " << channel.targetIndex;
		
				quaternion const * start 	= get_buffer_start<quaternion>(file, outputAccessor);
				quaternion const * end 		= start + valueCount;
				channel.rotations 			= allocate_array<quaternion>(allocator, start, end);

				/* Note(Leo): For some reason, we get inverted rotations from blender produced gltf-files,
				so we need to invert them here. I have not tested glTF files from other sources.

				If interpolation mode is CUBICSPLINE, we also have tangent quaternions, that are not
				unit length, so we must use proper inversion method. */
				for (auto & rotation : channel.rotations)
				{
					rotation = rotation.inverse_non_unit();
				}

			} break;

			default:
				logDebug(1) << FILE_ADDRESS << "Invalid or unimplemented animation channel: '" << channel.type << "' for bone " << channel.targetIndex;
				continue;

		}
	}
	result.duration = maxTime - minTime;

	logDebug(1) << "Animation loaded, duration: " << result.duration << "\n";

	DEBUG_ASSERT(minTime == 0, "Probably need to implement support animations that do not start at 0");

	return result;
}

internal AnimatedSkeleton
load_skeleton_glb(MemoryArena & allocator, GltfFile const & file, char const * modelName)
{
	auto nodes = file.json["nodes"].GetArray();

	s32 nodeIndex = index_by_name(nodes, modelName);
	Assert(nodeIndex >= 0);

	Assert(nodes[nodeIndex].HasMember("skin"));
	Assert(file.json.HasMember("skins"));

	// Note(Leo): We currently support only 1 skin per file, so that we can easily load correct animation.
	Assert(file.json["skins"].Size() == 1);
	Assert(file.json["skins"][0].HasMember("joints"));

	auto skin 		= file.json["skins"][0].GetObject();
	auto jsonBones 	= skin["joints"].GetArray();
	u32 boneCount 	= jsonBones.Size();
	m44 const * inverseBindMatrices = get_buffer_start<m44>(file, skin["inverseBindMatrices"].GetInt());

	AnimatedSkeleton skeleton = {};
	skeleton.bones = allocate_array<AnimatedBone>(allocator, boneCount, ALLOC_FILL | ALLOC_NO_CLEAR);
	
	for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		u32 nodeIndex 		= jsonBones[boneIndex].GetInt();
		auto const & node 	= nodes[nodeIndex].GetObject();

		u32 parent = -1;

		for (int parentBoneIndex = 0; parentBoneIndex < boneCount; ++parentBoneIndex)
		{
			u32 parentNodeIndex 	= jsonBones[parentBoneIndex].GetInt();
			auto const & parentNode = nodes[parentNodeIndex].GetObject();

			if (parentNode.HasMember("children"))
			{
				auto childArray = parentNode["children"].GetArray();
				for(auto const & childNodeIndex : childArray)
				{
					u32 childBoneIndex = find_bone_by_node(jsonBones, childNodeIndex.GetInt()); 
					if (childBoneIndex == boneIndex)
					{
						parent = parentBoneIndex;
					}
				}
			}
		}

		auto boneSpaceTransform = Transform3D::identity();
		if(node.HasMember("translation"))
		{
			auto translationArray = node["translation"].GetArray();
			boneSpaceTransform.position.x = translationArray[0].GetFloat();
			boneSpaceTransform.position.y = translationArray[1].GetFloat();
			boneSpaceTransform.position.z = translationArray[2].GetFloat();
		}

		if (node.HasMember("rotation"))
		{
			auto rotationArray = node["rotation"].GetArray();
			boneSpaceTransform.rotation.x = rotationArray[0].GetFloat();
			boneSpaceTransform.rotation.y = rotationArray[1].GetFloat();
			boneSpaceTransform.rotation.z = rotationArray[2].GetFloat();
			boneSpaceTransform.rotation.w = rotationArray[3].GetFloat();
 
			boneSpaceTransform.rotation = boneSpaceTransform.rotation.inverse();
		}

		m44 inverseBindMatrix = inverseBindMatrices[boneIndex];
		skeleton.bones[boneIndex] = make_bone(boneSpaceTransform, inverseBindMatrix, parent);

		// Note(Leo): Name is not essential...
		skeleton.bones[boneIndex].name = node.HasMember("name") ? node["name"].GetString() : "nameless bone";
	}

	return skeleton;
}

internal MeshAsset
load_mesh_glb(MemoryArena & allocator, GltfFile const & file, char const * modelName)
{
	logDebug() << "modelName = " << modelName; 

	auto nodes = file.json["nodes"].GetArray();
	
	s32 nodeIndex = index_by_name(nodes, modelName);
	// Assert(nodeIndex >= 0);
	DEBUG_ASSERT(nodeIndex >= 0, CStringBuilder("modelName = ") + modelName);

	Assert(file.json.HasMember("meshes"));
	Assert(nodes[nodeIndex].HasMember("mesh"));

	u32 meshIndex 	= nodes[nodeIndex]["mesh"].GetInt();
	auto meshObject	= file.json["meshes"][meshIndex].GetObject();

	/* Note(Leo): glTF primitives may contain following fields:
		- attributes,
		- indices,
		- material,
		- mode,
		- targets,
		- extensions,
		- extras

		We only care about attributes, indices and mode,
		Currently we support only single primitive, since we do not have any submesh systems.
	*/

	Assert (meshObject["primitives"].Size() == 1);

	auto meshPrimitive = meshObject["primitives"][0].GetObject();

	Assert (meshPrimitive.HasMember("attributes"));
	Assert (meshPrimitive.HasMember("indices"));
	Assert (meshPrimitive.HasMember("mode") == false || meshPrimitive["mode"].GetInt() == glTF::PRIMITIVE_TYPE_TRIANGLES);


	auto attribObject	= meshPrimitive["attributes"].GetObject();

	s32 positionAccessor 	= attribObject["POSITION"].GetInt();
	s32 normalAccessor 		= attribObject["NORMAL"].GetInt();
	s32 texcoordAccessor 	= attribObject["TEXCOORD_0"].GetInt();


	JsonArray const & accessors = file.json["accessors"].GetArray();

	u32 positionCount 	= accessors[positionAccessor]["count"].GetInt();
	u32 normalCount 	= accessors[normalAccessor]["count"].GetInt();
	u32 texcoordCount 	= accessors[texcoordAccessor]["count"].GetInt();

	Assert(positionCount == normalCount && positionCount == texcoordCount);

	u32 vertexCount = positionCount;

	// -----------------------------------------------------------------------------
	auto vertices = allocate_array<Vertex>(allocator, vertexCount, ALLOC_FILL | ALLOC_NO_CLEAR);

	v3 const * positions 	= get_buffer_start<v3>(file, positionAccessor);
	v3 const * normals 		= get_buffer_start<v3>(file, normalAccessor);
	v2 const * texcoords 	= get_buffer_start<v2>(file, texcoordAccessor);

	Assert(vertexCount < math::highest_value<u16> && "We need more indices!");

	for (u16 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
	{
		vertices[vertexIndex].position 	= positions[vertexIndex];
		vertices[vertexIndex].normal 	= normals[vertexIndex];
		vertices[vertexIndex].texCoord 	= texcoords[vertexIndex];
	}

	bool hasSkin = attribObject.HasMember("JOINTS_0") && attribObject.HasMember("WEIGHTS_0");

	if (hasSkin)
	{
		s32 bonesAccessorIndex 		= attribObject["JOINTS_0"].GetInt();
		s32 boneWeightsAccessorIndex = attribObject["WEIGHTS_0"].GetInt();

		// Note(Leo): only UNSIGNED_SHORT supported currently
		auto componentType = (glTF::ComponentType)accessors[bonesAccessorIndex]["componentType"].GetInt();
		Assert(componentType == glTF::COMPONENT_TYPE_UNSIGNED_SHORT);

		u16 const * bones = get_buffer_start<u16>(file, bonesAccessorIndex);
		f32 const * weights = get_buffer_start<f32>(file, boneWeightsAccessorIndex);

		for (u16 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			// Note(Leo): joints are stored as UINT16 in file, but we use UINT32, hence the intermediary buffer
			u16 bonesFromBuffer [4];
			copy_memory(bonesFromBuffer, bones + (vertexIndex * 4), sizeof(bonesFromBuffer));
			for(s32 boneIndexIndex = 0; boneIndexIndex < 4; ++boneIndexIndex)
			{
				vertices[vertexIndex].boneIndices[boneIndexIndex] = static_cast<u32>(bonesFromBuffer[boneIndexIndex]);
			}

			copy_memory(&vertices[vertexIndex].boneWeights, weights + (vertexIndex * 4), sizeof(vertices[vertexIndex].boneWeights));
		} 
	}

	// ----------------------------------------------------------------------------------

	s32 indexAccessor 	= meshPrimitive["indices"].GetInt();
	s32 indexCount 		= accessors[indexAccessor]["count"].GetInt();

	u16 const * indexStart 	= get_buffer_start<u16>(file, indexAccessor);
	u16 const * indexEnd 	= indexStart + indexCount;
	Array<u16> indices 		= allocate_array<u16>(allocator, indexStart, indexEnd);

	// ----------------------------------------------------------------------------------

	MeshAsset result = make_mesh_asset(std::move(vertices), std::move(indices));
	return result;
}

namespace mesh_ops
{
	internal void
	transform(MeshAsset * mesh, const m44 & transform)
	{
		int vertexCount = mesh->vertices.count();
		for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			mesh->vertices[vertexIndex].position = multiply_point(transform, mesh->vertices[vertexIndex].position); 
		}
	}

	internal void
	transform_tex_coords(MeshAsset * mesh, const v2 translation, const v2 scale)
	{
		int vertexCount = mesh->vertices.count();
		for(int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{

			mesh->vertices[vertexIndex].texCoord = vector::scale(mesh->vertices[vertexIndex].texCoord, scale);
			mesh->vertices[vertexIndex].texCoord += translation;
		}
	}
}

namespace mesh_primitives
{
	constexpr f32 radius = 0.5f;

	MeshAsset create_quad(MemoryArena * allocator, bool32 flipIndices)
	{
		MeshAsset result = {};
		result.indexType = IndexType::UInt16;

		int vertexCount = 4;
		result.vertices = allocate_array<Vertex>(*allocator, vertexCount, ALLOC_FILL | ALLOC_NO_CLEAR);

		// result.vertices[0] = Vertex{.position = {0,0,0}, .normal = {0,0,1}, .color = {1,1,1}, .texCoord = {0,0}};
		// result.vertices[0] = Vertex{.position = {1,0,0}, .normal = {0,0,1}, .color = {1,1,1}, .texCoord = {1,0}};
		// result.vertices[0] = Vertex{.position = {0,1,0}, .normal = {0,0,1}, .color = {1,1,1}, .texCoord = {0,1}};
		// result.vertices[0] = Vertex{.position = {1,1,0}, .normal = {0,0,1}, .color = {1,1,1}, .texCoord = {1,1}};
		// 					  position 		normal   	color    	uv
		result.vertices[0] = {0, 0, 0, 		0, 0, 1, 	1, 1, 1, 	0, 0};
		result.vertices[1] = {1, 0, 0, 		0, 0, 1, 	1, 1, 1, 	1, 0};
		result.vertices[2] = {0, 1, 0, 		0, 0, 1, 	1, 1, 1, 	0, 1};
		result.vertices[3] = {1, 1, 0, 		0, 0, 1, 	1, 1, 1, 	1, 1};

		int indexCount = 6;

		if (flipIndices)
		{
			// Note(Leo): These seem to backwardsm but it because currently our gui is viewed from behind.
			result.indices = allocate_array<u16>(*allocator, {0, 2, 1, 1, 2, 3});
		}
		else
		{
			result.indices = allocate_array<u16>(*allocator, {0, 1, 2, 2, 1, 3});
		}

		return result;
	}

	/*
	MeshAsset cube = {
		/// VERTICES
		{
			/// LEFT face
			{-radius, -radius,  radius, -1, 0, 0, 1, 1, 1, 0, 0},
			{-radius, -radius, -radius, -1, 0, 0, 1, 1, 1, 1, 0},
			{-radius,  radius,  radius, -1, 0, 0, 1, 1, 1, 0, 1},
			{-radius,  radius, -radius, -1, 0, 0, 1, 1, 1, 1, 1},

			/// RIGHT face
			{radius, -radius, -radius, 1, 0, 0, 1, 1, 1, 0, 0},
			{radius, -radius,  radius, 1, 0, 0, 1, 1, 1, 1, 0},
			{radius,  radius, -radius, 1, 0, 0, 1, 1, 1, 0, 1},
			{radius,  radius,  radius, 1, 0, 0, 1, 1, 1, 1, 1},
	
			/// BACK face
			{-radius, -radius,  radius, 0, -1, 0, 1, 1, 1, 0, 0},
			{ radius, -radius,  radius, 0, -1, 0, 1, 1, 1, 1, 0},
			{-radius, -radius, -radius, 0, -1, 0, 1, 1, 1, 0, 1},
			{ radius, -radius, -radius, 0, -1, 0, 1, 1, 1, 1, 1},

			/// FORWARD face
			{-radius, radius, -radius, 0, 1, 0, 1, 1, 1, 0, 0},
			{ radius, radius, -radius, 0, 1, 0, 1, 1, 1, 1, 0},
			{-radius, radius,  radius, 0, 1, 0, 1, 1, 1, 0, 1},
			{ radius, radius,  radius, 0, 1, 0, 1, 1, 1, 1, 1},

			/// BOTTOM face
			{-radius, -radius, -radius, 0, 0, -1, 1, 1, 1, 0, 0},
			{ radius, -radius, -radius, 0, 0, -1, 1, 1, 1, 1, 0},
			{-radius,  radius, -radius, 0, 0, -1, 1, 1, 1, 0, 1},
			{ radius,  radius, -radius, 0, 0, -1, 1, 1, 1, 1, 1},

			/// UP face
			{ radius, -radius, radius, 0, 0, 1, 1, 1, 1, 0, 0},
			{-radius, -radius, radius, 0, 0, 1, 1, 1, 1, 1, 0},
			{ radius,  radius, radius, 0, 0, 1, 1, 1, 1, 0, 1},
			{-radius,  radius, radius, 0, 0, 1, 1, 1, 1, 1, 1}
		},

		/// INDICES
		{
			/// LEFT face
			0, 2, 1, 1, 2, 3,

			/// RIGHT face
			4, 6, 5, 5, 6, 7,
	
			/// BOTTOM face
			8, 10, 9, 9, 10, 11,

			/// TOP face
			12, 14, 13, 13, 14, 15,

			/// BACK face
			16, 18, 17, 17, 18, 19,

			/// FRONT face
			20, 22, 21, 21, 22, 23
		},
		IndexType::UInt16
	};
	*/
}