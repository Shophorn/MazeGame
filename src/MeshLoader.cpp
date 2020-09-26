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

internal Array<Transform3D>
load_all_transforms_glb(MemoryArena & allocator, GltfFile const & file, char const * nodeName, Array<BoxCollider> * outColliders)
{
	auto nodes = file.json["nodes"].GetArray();

	s32 transformCount = 0;
	s32 colliderCount = 0;

	for (auto const & node : nodes)
	{
		if (cstring_begins_with(node["name"].GetString(), nodeName))
		{
			transformCount += 1;
		}
		else if (cstring_begins_with(node["name"].GetString(), "BoxCollider"))
		{
			colliderCount += 1;
		}
	}

	Array<Transform3D> transforms = allocate_array<Transform3D>(allocator, transformCount, ALLOC_NO_CLEAR);
	Array<BoxCollider> colliders = allocate_array<BoxCollider>(allocator, colliderCount, ALLOC_NO_CLEAR);
	Array<Transform3D> colliderFinalTransforms = allocate_array<Transform3D>(allocator, colliderCount, ALLOC_NO_CLEAR);

	for (auto const & node : nodes)
	{
		if (cstring_begins_with(node["name"].GetString(), nodeName))
		{
			Transform3D transform;

			if(node.HasMember("translation"))
			{
				auto translationArray = node["translation"].GetArray();
				transform.position.x = translationArray[0].GetFloat();
				transform.position.y = translationArray[1].GetFloat();
				transform.position.z = translationArray[2].GetFloat();
			}
			else
			{
				transform.position = {0, 0, 0};
			}

			if (node.HasMember("rotation"))
			{
				auto rotationArray = node["rotation"].GetArray();
				transform.rotation.x = rotationArray[0].GetFloat();
				transform.rotation.y = rotationArray[1].GetFloat();
				transform.rotation.z = rotationArray[2].GetFloat();
				transform.rotation.w = rotationArray[3].GetFloat();

				transform.rotation = inverse_quaternion(transform.rotation);
			}
			else
			{
				transform.rotation = identity_quaternion;
			}

			transform.scale = {1,1,1};

			transforms.push(transform);


			if(node.HasMember("children"))
			{
				auto const & childArray = node["children"].GetArray();

				s32 childCount = childArray.Size();
				m44 parentTransform = transform_matrix(transform);
				quaternion parentRotation = transform.rotation;

				for (s32 i = 0; i < childCount; ++i)
				{
					auto const & child = nodes[childArray[i].GetInt()].GetObject();
					Assert(child.HasMember("name"));

					if (cstring_begins_with(child["name"].GetString(), "BoxCollider"))
					{
						v3 center 					= {0,0,0};
						v3 extents 					= {0,0,0};
						quaternion localRotation 	= {0,0,0,1};

						if (child.HasMember("translation"))
						{
							auto translationArray = child["translation"].GetArray();
							center.x = translationArray[0].GetFloat();
							center.y = translationArray[1].GetFloat();
							center.z = translationArray[2].GetFloat();
						}

						if (child.HasMember("rotation"))
						{
							auto rotationArray = child["rotation"].GetArray();
							localRotation.x = rotationArray[0].GetFloat();
							localRotation.y = rotationArray[1].GetFloat();
							localRotation.z = rotationArray[2].GetFloat();
							localRotation.w = rotationArray[3].GetFloat();

							localRotation = inverse_quaternion(localRotation);
						}

						if (child.HasMember("scale"))
						{
							auto scaleArray = child["scale"].GetArray();
							extents.x = scaleArray[0].GetFloat();
							extents.y = scaleArray[1].GetFloat();
							extents.z = scaleArray[2].GetFloat();
						}

						colliders.push({ 	.extents = extents,
											.center = center,
											.orientation = localRotation,
											.transform = &transforms.last()});

						// v3 finalPosition 			= multiply_point(parentTransform, center);
						// quaternion finalRotation 	= parentRotation * rotation;
						// // v3 finalScale 				= extents;

						// m44 colliderTransform = 

					}
				}
			}

		}
	}

	*outColliders = std::move(colliders);
	return transforms;
}

internal Animation
load_animation_glb(MemoryArena & allocator, GltfFile const & file, char const * animationName)
{
	AssertMsg(file.json.HasMember("animations"), "No Animations found");

	auto animArray 		= file.json["animations"].GetArray();
	s32 animationIndex 	= index_by_name(animArray, animationName);

	if(animationIndex < 0)
	{
		s32 logMessageCapacity = 2048;
		String logMessage = push_temp_string(logMessageCapacity);
		string_append_format(logMessage, logMessageCapacity, "Animation not found. Requested: ", animationName, ", available: \n");

		// auto log = log_debug();
		// log << "Animation not found. Requested: " << animationName << ", available:\n";
		for (auto const & anim : animArray)
		{
			string_append_format(logMessage, logMessageCapacity, "\t", anim["name"].GetString(), "\n");
			// log << "\t" << anim["name"].GetString() << "\n";
		}
	}
	Assert(animationIndex >= 0);
	JsonValue const & namedAnimation = animArray[animationIndex];

	auto jsonChannels 	= animArray[animationIndex]["channels"].GetArray();
	auto samplers 		= animArray[animationIndex]["samplers"].GetArray();

	auto accessors 		= file.json["accessors"].GetArray();

	s32 translationChannelCount = 0;
	s32 rotationChannelCount = 0;

	for (auto const & jsonChannel : jsonChannels)
	{
		char const * path 	= jsonChannel["target"]["path"].GetString();

		if (cstring_equals(path, "translation"))	{ translationChannelCount += 1; }
		else if(cstring_equals(path, "rotation")) { rotationChannelCount += 1; }
		else if (cstring_equals(path, "scale"))	 {}
		else
			Assert(false);
	}

	Animation result = {};
	result.translationChannels 	= push_array_2<TranslationChannel>(allocator, translationChannelCount, ALLOC_CLEAR);
	result.rotationChannels 	= push_array_2<RotationChannel>(allocator, rotationChannelCount, ALLOC_CLEAR);

	float minTime = highest_f32;
	float maxTime = lowest_f32;

	for (auto const & jsonChannel : jsonChannels)
	{
		/* Note(Leo): input refers to keyframe times and output to actual values,
		i.e. the input and output of interpolation function for animation. */
		int samplerIndex 	= jsonChannel["sampler"].GetInt();
		int inputAccessor 	= samplers[samplerIndex]["input"].GetInt();
		int outputAccessor 	= samplers[samplerIndex]["output"].GetInt();

		minTime = min_f32(minTime, accessors[inputAccessor]["min"][0].GetFloat());
		maxTime = max_f32(maxTime, accessors[inputAccessor]["max"][0].GetFloat());


		// ------------------------------------------------------------------------------

		enum { 	ANIMATION_CHANNEL_TRANSLATION,
				ANIMATION_CHANNEL_ROTATION,
				ANIMATION_CHANNEL_SCALE,
		} channelType;

		InterpolationMode 	interpolationMode;
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

		s32 targetIndex = -1;
		{
			/* Note(Leo): Bones (or joints) are somewhat cumbersomely behind different list, so we need to remap those.
			Probably the best idea would be to have skeleton reference here too. */
			Assert(file.json.HasMember("skins") && file.json["skins"].Size() == 1 && file.json["skins"][0].HasMember("joints"));

			auto bonesArray 	= file.json["skins"][0]["joints"].GetArray();
			s32 targetNode 		= jsonChannel["target"]["node"].GetInt();
			targetIndex = find_bone_by_node(bonesArray, targetNode);
		
			Assert(targetIndex >= 0);
		}
		
		// ------------------------------------------------------------------------------

		int keyframeCount = accessors[inputAccessor]["count"].GetInt();

		float const * timesStart = get_buffer_start<float>(file, inputAccessor);
		// float const * timesEnd 	= timesStart + keyframeCount;
		Array2<f32> times = push_array_2<float>(allocator, keyframeCount, 0);//timesStart, timesEnd);
		array_2_fill_from_memory(times, keyframeCount, timesStart);

		/* Note(Leo): CUBICSPLINE interpolation has three values in total: inTangent,
		splineVertex(aka actual keyframe value) and outTangent. */
		s32 valueCount = interpolationMode == INTERPOLATION_MODE_CUBICSPLINE
						? keyframeCount * 3
						: keyframeCount;

		switch(channelType)
		{
			case ANIMATION_CHANNEL_TRANSLATION:
			{
				v3 const * start 		= get_buffer_start<v3>(file, outputAccessor);
				v3 const * end 			= start + valueCount;

				TranslationChannel channel 	= {};
				channel.times 				= times;

				// Note(Leo): We do not yet support cubicspline interpolation, so we do not need the data for it, convert to linear				
				if (interpolationMode == INTERPOLATION_MODE_CUBICSPLINE)
				{
					interpolationMode = INTERPOLATION_MODE_LINEAR;
					
					channel.translations = push_array_2<v3>(allocator, keyframeCount, 0);
					array_2_fill_uninitialized(channel.translations);

					for (s32 keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
					{
						s32 valueIndex = keyframeIndex * 3 + 1;
						channel.translations[keyframeIndex] = start[valueIndex];
					}					
				}
				else
				{
					channel.translations = push_array_2<v3>(allocator, keyframeCount, 0);
					array_2_fill_from_memory(channel.translations, keyframeCount, start);

				}
			  	
			  	channel.interpolationMode = interpolationMode;
				channel.targetIndex 		= targetIndex;

				result.translationChannels.push(channel);
			} break;

			case ANIMATION_CHANNEL_ROTATION:
			{
				quaternion const * start 	= get_buffer_start<quaternion>(file, outputAccessor);
				quaternion const * end 		= start + valueCount;

				RotationChannel channel 	= {};
				channel.times 				= std::move(times);

				// Note(Leo): We do not yet support cubicspline interpolation, so we do not need the data for it, convert to linear				
				if (interpolationMode == INTERPOLATION_MODE_CUBICSPLINE)
				{
					interpolationMode = INTERPOLATION_MODE_LINEAR;
					
					channel.rotations = push_array_2<quaternion>(allocator, keyframeCount, 0);
					array_2_fill_uninitialized(channel.rotations);

					for (s32 keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
					{
						s32 valueIndex = keyframeIndex * 3 + 1;
						channel.rotations[keyframeIndex] = start[valueIndex];
					}
				}
				else
				{
					channel.rotations = push_array_2<quaternion>(allocator, keyframeCount, 0);
					array_2_fill_from_memory(channel.rotations, keyframeCount, start);
				}
				
				channel.interpolationMode 	= interpolationMode;
				channel.targetIndex 		= targetIndex;


				/* Note(Leo): For some reason, we get inverted rotations from blender produced gltf-files,
				so we need to invert them here. I have not tested glTF files from other sources.

				If interpolation mode is CUBICSPLINE, we also have tangent quaternions, that are not
				unit length, so we must use proper inversion method for those. */
				for (auto & rotation : channel.rotations)
				{
					rotation = inverse_non_unit_quaternion(rotation);
				}

				result.rotationChannels.push(channel);
			} break;

			default:
				log_debug(1, FILE_ADDRESS, "Invalid or unimplemented animation channel: '", channelType, "' for bone ", targetIndex);
				continue;

		}
	}
	result.duration = maxTime - minTime;

	log_animation(1, "Animation loaded, duration: ", result.duration);

	AssertMsg(minTime == 0, "Probably need to implement support animations that do not start at 0");

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
	skeleton.bones = push_array_2<AnimatedBone>(allocator, boneCount, ALLOC_CLEAR);
	
	for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		u32 nodeIndex 		= jsonBones[boneIndex].GetInt();
		auto const & node 	= nodes[nodeIndex].GetObject();

		s32	 parent = -1;

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

		if (parent < 0)
		{
			/* Note(Leo): Only accept one bone with no parent, and we recognize it by name now

			This is a hack, but it works well enough. Bones that are not root,
			but have no parent are likely to be ik rig bones or similar non deforming bones. 
			We have made notion to asset workflow to remove these bones before exporting,
			this will catch those if forgotten. */
			Assert(node.HasMember("name"));
			if (cstring_equals("Root", node["name"].GetString()) == false)
			{
				parent = 0;
			}
		}

		auto boneSpaceTransform = identity_transform;
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
 
			boneSpaceTransform.rotation = inverse_quaternion(boneSpaceTransform.rotation);
		}

		m44 inverseBindMatrix = inverseBindMatrices[boneIndex];
		skeleton.bones.push(make_bone(boneSpaceTransform, inverseBindMatrix, parent));
	}

	/* Todo(Leo): Check that parent always comes before bone itself. Currently we have no mechanism
	to fix the situation, so we just abort.*/
	Assert(skeleton.bones[0].parent < 0);

	for (s32 i = 1; i < skeleton.bones.count; ++i)
	{
		Assert((skeleton.bones[i].parent < i) && skeleton.bones[i].parent >= 0);
	}

	return skeleton;
}

internal MeshAssetData
load_mesh_glb(MemoryArena & allocator, GltfFile const & file, char const * modelName)
{
	auto nodes = file.json["nodes"].GetArray();
	
	s32 nodeIndex = index_by_name(nodes, modelName);
	// Assert(nodeIndex >= 0);

	if (nodeIndex < 0)
	{
		log_debug(0, "Asset not found! modelName = ", modelName);
		Assert(false);
	}

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

	auto attribObject		= meshPrimitive["attributes"].GetObject();

	s32 positionAccessor 	= attribObject["POSITION"].GetInt();
	s32 normalAccessor 		= attribObject["NORMAL"].GetInt();
	s32 texcoordAccessor 	= attribObject["TEXCOORD_0"].GetInt();

	s32 tangentAccessor = -1;
	if (attribObject.HasMember("TANGENT"))
	{
		tangentAccessor = attribObject["TANGENT"].GetInt();		
	}

	JsonArray const & accessors = file.json["accessors"].GetArray();

	u32 positionCount 	= accessors[positionAccessor]["count"].GetInt();
	u32 normalCount 	= accessors[normalAccessor]["count"].GetInt();
	u32 texcoordCount 	= accessors[texcoordAccessor]["count"].GetInt();

	Assert(positionCount == normalCount && positionCount == texcoordCount);

	u32 vertexCount = positionCount;

	// -----------------------------------------------------------------------------
	// auto vertices = allocate_array<Vertex>(allocator, vertexCount, ALLOC_FILL | ALLOC_NO_CLEAR);
	Vertex * vertices = push_memory<Vertex>(allocator, vertexCount, 0);

	v3 const * positions 	= get_buffer_start<v3>(file, positionAccessor);
	v3 const * normals 		= get_buffer_start<v3>(file, normalAccessor);
	v2 const * texcoords 	= get_buffer_start<v2>(file, texcoordAccessor);

	Assert(vertexCount < max_value_u16 && "We need more indices!");

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
	u16 * indices 			= push_and_copy_memory(allocator, indexCount, indexStart, 0);

	// ----------------------------------------------------------------------------------

	// Todo(Leo): We have a function for this, use that
	s32 triangleCount = indexCount / 3;
	v3 * vertexTangents = push_memory<v3>(*global_transientMemory, vertexCount, 0);
	
	for (s32 i = 0; i < triangleCount; ++i)
	{
		s32 index0 = indices[i * 3];
		s32 index1 = indices[i * 3 + 1];
		s32 index2 = indices[i * 3 + 2];

		v3 p0 = vertices[index0].position;
		v3 p1 = vertices[index1].position;
		v3 p2 = vertices[index2].position;

		v3 p01 = p1 - p0;
		v3 p02 = p2 - p0;

		v2 uv0 = vertices[index0].texCoord;
		v2 uv1 = vertices[index1].texCoord;
		v2 uv2 = vertices[index2].texCoord;

		v2 uv01 = uv1 - uv0;
		v2 uv02 = uv2 - uv0;

		f32 r = 1.0f / (uv01.x * uv02.y - uv01.y * uv02.x);

		v3 tangent 	= (p01 * uv02.y - p02 * uv01.y) * r;
		tangent 	= normalize_v3(tangent);

		vertexTangents[index0] += tangent;
		vertexTangents[index1] += tangent;
		vertexTangents[index2] += tangent;
	}

	for (s32 i = 0; i < vertexCount; ++i)
	{
		vertices[i].tangent = normalize_v3(vertexTangents[i]);
	}


	// ----------------------------------------------------------------------------------

	MeshAssetData result 	= {};
	result.vertexCount		= vertexCount;
	result.vertices 		= vertices;

	result.indexCount 	= indexCount;
	result.indices 		= indices;
	return result;
}


internal MeshAssetData
load_mesh_glb(MemoryArena & allocator, char const * filename, char const * modelName)
{
	GltfFile file = read_gltf_file(*global_transientMemory, filename);
	return load_mesh_glb(allocator, file, modelName);
}

namespace mesh_ops
{
	internal void
	transform(MeshAssetData * mesh, const m44 & transform)
	{
		int vertexCount = mesh->vertexCount;
		for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			mesh->vertices[vertexIndex].position = multiply_point(transform, mesh->vertices[vertexIndex].position); 
		}
	}

	internal void
	transform_tex_coords(MeshAssetData * mesh, const v2 translation, const v2 scale)
	{
		int vertexCount = mesh->vertexCount;
		for(int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{

			mesh->vertices[vertexIndex].texCoord = scale_v2(mesh->vertices[vertexIndex].texCoord, scale);
			mesh->vertices[vertexIndex].texCoord += translation;
		}
	}
}

namespace mesh_primitives
{
	constexpr f32 radius = 0.5f;

	MeshAssetData create_quad(MemoryArena * allocator, bool32 flipIndices)
	{
		MeshAssetData result = {};
		result.indexType = IndexType::UInt16;

		result.vertexCount 	= 4;
		result.vertices 	= push_memory<Vertex>(*allocator, result.vertexCount, 0);

		// 					  position 		normal   	color    	uv
		result.vertices[0] = {0, 0, 0, 		0, 0, 1, 	1, 1, 1, 	0, 0};
		result.vertices[1] = {1, 0, 0, 		0, 0, 1, 	1, 1, 1, 	1, 0};
		result.vertices[2] = {0, 1, 0, 		0, 0, 1, 	1, 1, 1, 	0, 1};
		result.vertices[3] = {1, 1, 0, 		0, 0, 1, 	1, 1, 1, 	1, 1};

		int indexCount = 6;

		if (flipIndices)
		{
			// Note(Leo): These seem to backwardsm but it because currently our gui is viewed from behind.
			u16 indices [] 		= {0, 2, 1, 1, 2, 3};
			result.indexCount 	= array_count(indices);
			result.indices 		= push_and_copy_memory(*allocator, array_count(indices), indices, 0);
		}
		else
		{
			u16 indices [] 		= {0, 1, 2, 2, 1, 3};
			result.indexCount 	= array_count(indices);
			result.indices 		= push_and_copy_memory(*allocator, array_count(indices), indices, 0);

		}

		return result;
	}

	/*
	MeshAssetData cube = {
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