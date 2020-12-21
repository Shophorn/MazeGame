	enum BoxState : s32
	{
		BoxState_closed,
		BoxState_opening,
		BoxState_open,
		BoxState_closing,
	};
	
	struct Boxes
	{
		s64 				count;
		Transform3D * 		transforms;
		Transform3D * 		coverLocalTransforms;
		BoxState * 			states;
		f32 *				openStates;
		EntityReference * 	carriedEntities;
	};