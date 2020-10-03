/*
Leo Tamminen
shophorn @ internet

Some excuse for a rendering system
*/
struct AnimatedRenderer
{
	AnimatedSkeleton * 	skeleton;
	ModelHandle 		model;

	// Options
	bool32 castShadows = true;
};

AnimatedRenderer make_animated_renderer (	Transform3D * transform,
											AnimatedSkeleton * skeleton,
											ModelHandle model)
{
	auto result = AnimatedRenderer {
			// .transform 	= transform,
			.skeleton 	= skeleton,
			.model 		= model,
		};	
	return result;		
}

void update_animated_renderer(m44 * boneTransformMatrices, SkeletonAnimator const & animator)
{
	/* Note(Leo): Vertex shader where actual deforming happens, needs to know
	transform from bind position aka default position (i.e. the original position
	of vertex or bone in model space) to final deformed position in model space.

	We compute the deformed matrix by applying the whole hierarchy of matrices,
	and adding it to the inverse bind matrix which is the inverse transform from origin
	to the bind pose.

	Todo(Leo): Try optimizing this by first zipping matrices to aos containing all m44s needed.

	Note(Leo): set bones[0] separately, so we don't need if statement inside loop.
	We require proper order of bones from source file by asserting it in loader function, 
	proper order meaning that parent always comes before children in array, so that we can
	use same array to store "own" transforms and check parent transform, that has been
	previously set in loop. */

	auto const & boneSpaceTransforms 	= animator.boneBoneSpaceTransforms;
	auto const & skeleton 				= *animator.skeleton;

	s32 boneCount = skeleton.bonesCount;

	boneTransformMatrices[0] = transform_matrix(boneSpaceTransforms[0]);
	for (s32 i = 1; i < boneCount; ++i)
	{
		s32 parentIndex 			= skeleton.bones[i].parent;
		boneTransformMatrices[i] 	= boneTransformMatrices[parentIndex] * transform_matrix(boneSpaceTransforms[i]);
	}

	for (s32 i = 0; i < boneCount; ++i)
	{
		boneTransformMatrices[i] = boneTransformMatrices[i] * skeleton.bones[i].inverseBindMatrix;
	}
}