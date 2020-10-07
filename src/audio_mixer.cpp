/*
Leo Tamminen

Audio mixing and playing things

Study:
	http://www.sengpielaudio.com/calculator-SoundAndDistance.htm
*/

struct AudioAsset
{
	s32 sampleCount;
	f32 * leftChannel;
	f32 * rightChannel;
};

struct AudioClip
{
	AudioAsset const * asset;
	f32 sampleIndex;

	f32 pitch = 1;
	f32 volume = 1;

	v3 position;
};

// AudioAsset load_audio_clip(MemoryArena & allocator, char const * filename)
// {
// 	AudioFile<f32> file;
// 	file.load(filename);

// 	AudioAsset clip 	= {};
// 	clip.sampleCount 	= file.getNumSamplesPerChannel();
// 	clip.leftChannel 	= push_memory<f32>(allocator, clip.sampleCount, ALLOC_GARBAGE);
// 	clip.rightChannel 	= push_memory<f32>(allocator, clip.sampleCount, ALLOC_GARBAGE);

// 	memory_copy(clip.leftChannel, file.samples[0].data(), sizeof(f32) * clip.sampleCount);
// 	memory_copy(clip.rightChannel, file.samples[1].data(), sizeof(f32) * clip.sampleCount);

// 	return clip;
// }

// internal PlatformStereoSoundSample get_next_sample_looping(AudioClip & clip)
// {
// 	PlatformStereoSoundSample sample = {
// 		.left = clip.asset->file.samples[0][clip.sampleIndex],
// 		.right = clip.asset->file.samples[1][clip.sampleIndex]
// 	};

// 	// clip.sampleIndex = (clip.sampleIndex + 1) % clip.asset->sampleCount;
// 	clip.sampleIndex += clip.pitch;
// 	clip.sampleIndex = mod_f32(clip.sampleIndex, clip.asset->sampleCount);

// 	return sample;
// }

internal bool32 get_next_sample(AudioClip & clip, PlatformStereoSoundSample & outSample)
{
	s32 previousIndex 	= floor_f32(clip.sampleIndex);
	s32 nextIndex 		= previousIndex + 1;
	f32 lerpTime 		= clip.sampleIndex - previousIndex;

	PlatformStereoSoundSample previous = 
	{
		.left = clip.asset->leftChannel[previousIndex],
		.right = clip.asset->rightChannel[previousIndex]
	};

	PlatformStereoSoundSample next = 
	{
		.left = clip.asset->leftChannel[nextIndex],
		.right = clip.asset->rightChannel[nextIndex]
	};

	outSample = 
	{
		.left = clip.volume * f32_lerp(previous.left, next.left, lerpTime),
		.right = clip.volume * f32_lerp(previous.right, next.right, lerpTime)
	};

	clip.sampleIndex 	+= clip.pitch;
	bool32 result 		= clip.sampleIndex < clip.asset->sampleCount;
	clip.sampleIndex 	= mod_f32(clip.sampleIndex, clip.asset->sampleCount);

	return result;
}
