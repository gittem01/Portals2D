#include <ThinDrawer.h>

void AnimStruct::linearFunc(float xProg, float magnitude, float from, float* value)
{
	*value = from + xProg * magnitude;
}

AnimGroup::AnimGroup(int count)
{
	this->count = count;
	this->lastFreePointer = (AnimStruct*)malloc(sizeof(AnimStruct) * count);
	this->pointerHeader = this->lastFreePointer;
}

AnimGroup::~AnimGroup()
{
	free(pointerHeader);
	
	if (toNull)
	{
		*toNull = nullptr;
	}

	if (prev)
	{
		prev->next = next;
	}
	if (next)
	{
		next->prev = prev;
	}
}

void AnimGroup::updateStructWithPointer(float* valuePtr, float scale, float from, float to)
{
	AnimStruct* animStruct = nullptr;
	for (animStruct = animStructs; animStruct; animStruct = animStruct->next)
	{
		if (animStruct->value == valuePtr)
		{
			break;
		}
	}

	if (animStruct)
	{
		animStruct->scale = scale;
		animStruct->xProg = 0.0f;
		animStruct->from = from;
		animStruct->magnitude = to - from;
	}
}

AnimStruct* AnimGroup::removeStruct(AnimStruct* animStruct)
{
	AnimStruct* retStruct;
	if (!animStruct->chainedAnimations)
	{
		retStruct = animStruct->next;
		if (animStruct->prev == nullptr)
		{
			animStructs = animStruct->next;
		}

		if (animStruct->prev) animStruct->prev->next = animStruct->next;
		if (animStruct->next) animStruct->next->prev = animStruct->prev;
	}
	else
	{
		retStruct = animStruct->chainedAnimations;
		if (animStruct->prev == nullptr)
		{
			animStructs = animStruct->chainedAnimations;
		}

		if (animStruct->prev) animStruct->prev->next = animStruct->chainedAnimations;
		if (animStruct->next) animStruct->next->prev = animStruct->lastChainedAimation;

		animStruct->chainedAnimations->prev = animStruct->prev;
		animStruct->lastChainedAimation->next = animStruct->next;
	}

	return retStruct;
}

AnimStruct* AnimGroup::createAnimation(float* value, float scale, float from, float to)
{
	AnimStruct* animStruct = lastFreePointer++;
	animStruct->prev = nullptr;
	animStruct->next = animStructs;
	if (animStructs)
	{
		animStructs->prev = animStruct;
	}
	animStructs = animStruct;

	animStruct->value = value;
	animStruct->scale = scale;
	animStruct->xProg = 0.0f;
	animStruct->from = from;
	animStruct->magnitude = to - from;

	animStruct->chainedAnimations = nullptr;
	animStruct->lastChainedAimation = nullptr;

	return animStruct;
}

AnimStruct* AnimGroup::createChainedAnimation(AnimStruct* animStructTo, float* value, float scale, float from, float to)
{
	AnimStruct* animStruct = lastFreePointer++;
	animStructTo->lastChainedAimation = animStruct;

	if (animStructTo->chainedAnimations)
	{
		animStructTo->chainedAnimations->next = animStruct;
		animStruct->prev = animStructTo->chainedAnimations;
	}
	else
	{
		animStructTo->chainedAnimations = animStruct;
		animStruct->prev = nullptr;
	}
	
	animStruct->next = nullptr;

	animStruct->value = value;
	animStruct->scale = scale;
	animStruct->xProg = 0.0f;
	animStruct->from = from;
	animStruct->magnitude = to - from;

	animStruct->chainedAnimations = nullptr;
	animStruct->lastChainedAimation = nullptr;

	return animStruct;
}
