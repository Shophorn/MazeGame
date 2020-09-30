struct Train
{
	Transform3D transform;
	v3 			targetPosition;	
	s32 		pathPositionIndex;
}

void update_train(Train & train, CollisionsSystem3D & collisionSystem)
{



	constexpr f32 pathPositions [] = 
	{
		{},
		{},
	};

	f32 speed 		= 50;

	v3 toTarget 	= train.targetPosition - train.transform.position;
	f32 sqrLength	= sqr_v3_length(toTarget);

	if (sqrLength > 0)
	{

	}
	else
	{

	}
};