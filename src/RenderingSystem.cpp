/*=============================================================================
Leo Tamminen
shophorn @ internet
=============================================================================*/
struct Renderer
{
	Transform3D * 	transform;
	ModelHandle 	model;
	bool32 			castShadows = true;
};

struct AnimatedRenderer
{
	// References to external resources
	Transform3D	* 		transform;
	AnimatedSkeleton * 	skeleton;
	ModelHandle 		model;

	// Memory where animated bone matrices are stores
	Array<m44> bones;

	// Options
	bool32 castShadows = true;
};

AnimatedRenderer make_animated_renderer (	Transform3D * transform,
											AnimatedSkeleton * skeleton,
											ModelHandle model,
											MemoryArena & allocator)
{
	auto result = AnimatedRenderer {
			.transform 	= transform,
			.skeleton 	= skeleton,
			.model 		= model,
			.bones 		= allocate_array<m44>(allocator, skeleton->bones.count(), ALLOC_FILL | ALLOC_NO_CLEAR)
		};	
	return result;		
}

void update_animated_renderer(AnimatedRenderer & renderer)
{
	/* Note(Leo): Vertex shader where actual deforming happens, needs to know
	transform from bind position aka default position (i.e. the original position
	of vertex or bone in model space) to final deformed position in model space.

	We compute the deformed matrix by applying the whole hierarchy of matrices,
	and adding it to the inverse bind matrix which is the inverse transform from origin
	to the bind pose. */
	
	AnimatedSkeleton const & skeleton 	= *renderer.skeleton; 
	u32 boneCount 						= skeleton.bones.count();

	// Compute current pose in model space
	for (int i = 0; i < boneCount; ++i)
	{
		m44 matrix = get_matrix(skeleton.bones[i].boneSpaceTransform);

		if (skeleton.bones[i].is_root() == false)
		{
			// Note(Leo): this is sanity check for future, we don't currently check this elsewhere
			s32 parentIndex = skeleton.bones[i].parent;
			Assert(parentIndex < i);

			m44 parentMatrix = renderer.bones[parentIndex];
			matrix = parentMatrix * matrix;
		}
		renderer.bones[i] = matrix;
	}

	// Compute deform transformation (from bind position to pose position) in model space
	for (int i = 0; i < boneCount; ++i)
	{
		renderer.bones[i] = renderer.bones[i] * skeleton.bones[i].inverseBindMatrix;
	}
}