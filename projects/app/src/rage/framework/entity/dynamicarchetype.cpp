#include "dynamicarchetype.h"

rage::fwDynamicArchetypeComponent::fwDynamicArchetypeComponent()
{
	ExpressionName = atHashString::Null();
	AiAvoidancePoint = S_MAX;
	PhysicsRefCount = 0;
	AutoStartAnim = false;
	HasPhysicsInDrawable = false;

	ClipDictionarySlot = -1;
	BlendShapeFileSlot = -1;
	ExpressionDictionarySlot = -1;
	PoseMatcherFileSlot = -1;
	PoseMatcherProneSlot = -1;
	CreatureMetadataSlot = -1;
}
