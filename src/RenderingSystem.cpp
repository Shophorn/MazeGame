/*
Leo Tamminen
shophorn @ internet

Some excuse for a rendering system
*/

	// AnimatedRenderer 	animatedRenderer;
void skeleton_animator_get_bone_transform_matrices(SkeletonAnimator const & animator, m44 * boneTransformMatrices)
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

	boneTransformMatrices[0] = transform_matrix(boneSpaceTransforms[0]);
	for (s32 i = 1; i < skeleton.boneCount; ++i)
	{
		s32 parentIndex 			= skeleton.bones[i].parent;
		boneTransformMatrices[i] 	= boneTransformMatrices[parentIndex] * transform_matrix(boneSpaceTransforms[i]);
	}

	for (s32 i = 0; i < skeleton.boneCount; ++i)
	{
		boneTransformMatrices[i] = boneTransformMatrices[i] * skeleton.bones[i].inverseBindMatrix;
	}
}