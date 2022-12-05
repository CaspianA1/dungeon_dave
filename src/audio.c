#include "audio.h"
#include "utils/failure.h" // For `FAIL`
#include "utils/debug_macro_utils.h" // TODO: remove

//////////

typedef struct {
	const ALuint al_buffer;
	const ALchar* const path;
	const byte flags;
} AudioClip;

//////////

/* This converts the bytes data referred to
by `audio_data`, and updates `data_length` too.
Note that both have previously valid values upon function entry,
and that conversion will not happen if it isn't needed. */
static void convert_audio_data(
	bool convert_to_mono,
	const SDL_AudioFormat new_format,
	const ALchar* const path,
	const SDL_AudioSpec* const orig_spec,
	Uint8** audio_data, Uint32* const data_length) {

	////////// Checking if conversion is really needed

	const Uint8 orig_num_channels = orig_spec -> channels;
	const Uint8 new_num_channels = convert_to_mono ? 1 : orig_num_channels;
	const SDL_AudioFormat orig_format = orig_spec -> format;

	if (orig_format == new_format && orig_num_channels == new_num_channels) return;
	
	////////// Making a stream

	const int sample_rate = orig_spec -> freq;

	SDL_AudioStream* const stream = SDL_NewAudioStream(
		orig_format, orig_num_channels, sample_rate,
		new_format, new_num_channels, sample_rate);

	if (stream == NULL) goto error;

	////////// Writing the data into the stream, and flushing the buffer for the conversion process

	if (SDL_AudioStreamPut(stream, *audio_data, (int) *data_length) == -1) goto error;
	SDL_AudioStreamFlush(stream); // TODO: only flush at the end (then, converting can happen asynchronously)

	////////// Writing the converted bytes to the original audio buffer

	const int num_bytes_for_converted_audio = SDL_AudioStreamAvailable(stream);

	// TODO: is there a specific function for reallocing data alloced by `SDL_LoadWAV`?
	*audio_data = SDL_realloc(*audio_data, (size_t) num_bytes_for_converted_audio);
	*data_length = (Uint32) num_bytes_for_converted_audio;

	if (SDL_AudioStreamGet(stream, *audio_data, num_bytes_for_converted_audio) == -1) goto error;
	SDL_FreeAudioStream(stream);

	return;

	////////// Error handling

	error: FAIL(CreateAudioBuffer,
		"Could not create an audio buffer for '%s' "
		"because the audio could not be converted "
		"into a supported format: %s", path, SDL_GetError());
}

static ALuint init_audio_buffer(const ALchar* const path, const byte flags) {
	////////// Loading the original WAV data in

	SDL_AudioSpec audio_spec;
	Uint8* wav_buffer;
	Uint32 wav_length;

	if (SDL_LoadWAV(path, &audio_spec, &wav_buffer, &wav_length) == NULL)
		FAIL(OpenFile, "Could not load '%s': %s", path, SDL_GetError());

	////////// Converting the WAV data, if needed

	// OpenAL only supports unsigned 8-bit and signed 16-bit
	const SDL_AudioFormat
		supported_sdl_8_bit_format = AUDIO_U8,
		supported_sdl_16_bit_format = AUDIO_S16;

	const bool wav_file_is_stereo = (audio_spec.channels == 2);
	const bool convert_to_mono = wav_file_is_stereo && (flags & AudioIsPositional);

	const Uint8 orig_num_bits_for_format = SDL_AUDIO_BITSIZE(audio_spec.format);
	Uint8 converted_num_bits_for_format;

	/* Note that the conversion function will only convert if
	conversion is actually needed. Otherwise, it will just return. */
	switch (orig_num_bits_for_format) {
		case 8: // Converting if needed since OpenAL doesn't support signed 8-bit
			convert_audio_data(convert_to_mono, supported_sdl_8_bit_format,
				path, &audio_spec, &wav_buffer, &wav_length);

			converted_num_bits_for_format = 8;
			break;

		case 16: // Converting if needed since OpenAL doesn't support unsigned 16-bit
			convert_audio_data(convert_to_mono, supported_sdl_16_bit_format,
				path, &audio_spec, &wav_buffer, &wav_length);

			converted_num_bits_for_format = 16;
			break;

		case 32: // Converting to 16-bit b/c OpenAL doesn't support 32-bit
			convert_audio_data(convert_to_mono, supported_sdl_16_bit_format,
				path, &audio_spec, &wav_buffer, &wav_length);

			converted_num_bits_for_format = 16;
			break;

		default:
			FAIL(CreateAudioBuffer, "Could not use '%s', because %hhu-bit "
				"WAV formats are not supported.", path, orig_num_bits_for_format);
	}

	////////// Finding the AL format

	ALenum mono_format, stereo_format;

	if (converted_num_bits_for_format == 8) {
		mono_format = AL_FORMAT_MONO8;
		stereo_format = AL_FORMAT_STEREO8;
	}
	else {
		mono_format = AL_FORMAT_MONO16;
		stereo_format = AL_FORMAT_STEREO16;
	}

	const ALenum al_format = (convert_to_mono || !wav_file_is_stereo) ? mono_format : stereo_format;

	////////// Making an audio buffer, and then freeing the wav buffer

	ALuint al_buffer;
	alGenBuffers(1, &al_buffer);
	alBufferData(al_buffer, al_format, wav_buffer, (ALsizei) wav_length, audio_spec.freq);

	SDL_FreeWAV(wav_buffer);

	return al_buffer;
}

//////////

AudioContext init_audio_context(void) {
	// TODO: find out if any of these can return null, and if so, fail
	ALCdevice* const device = alcOpenDevice(NULL);
	ALCcontext* const context = alcCreateContext(device, NULL);

	if (!alcMakeContextCurrent(context))
		FAIL(LoadOpenAL, "Could not load OpenAL (at `alcMakeContextCurrent`). Reason: %s", get_ALC_error(device));

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	//////////

	return (AudioContext) {
		device, context,
		init_list(1, AudioClip),
		init_list(1, PositionalAudioSource),
		init_list(1, NonPositionalAudioSource)
	};
}

void add_audio_clip_to_audio_context(AudioContext* const context, const ALchar* const path, const byte flags) {
	const ALuint buffer = init_audio_buffer(path, flags);
	push_ptr_to_list(&context -> clips, &(AudioClip) {buffer, path, flags});
}

////////// Adding sources to the context

static void add_audio_source_to_audio_context(AudioContext* const context,
	void* const source_struct, const bool is_positional) {

	const List* const clips = &context -> clips;

	const ALchar* const path = is_positional
		? ((PositionalAudioSource*) source_struct) -> metadata.path
		: ((NonPositionalAudioSource*) source_struct) -> path;

	//////////

	LIST_FOR_EACH(clips, AudioClip, clip_ref,
		const AudioClip clip = *clip_ref;
		const bool clip_is_positional = !!(clip.flags & AudioIsPositional);

		if (!strcmp(clip.path, path) && (clip_is_positional == is_positional)) {
			ALuint source;
			alGenSources(1, &source);
			alSourcei(source, AL_BUFFER, (ALint) clip.al_buffer);

			if (clip.flags & AudioLoops)
				alSourcei(source, AL_LOOPING, AL_TRUE);

			List* const dest_list = is_positional
				? &context -> positional_sources
				: &context -> nonpositional_sources;
			
			if (is_positional)
				((PositionalAudioSource*) source_struct) -> al_source = source;
			else
				((NonPositionalAudioSource*) source_struct) -> al_source = source;

			push_ptr_to_list(dest_list, source_struct);

			return;
		}
	);

	FAIL(CreateAudioSource, "Could not find a matching clip for %spositional "
		"audio source with path '%s'. Perhaps you specified the clip as %spositional?",
			is_positional ? "" : "non", path, is_positional ? "non" : ""
		);
}

void add_positional_audio_source_to_audio_context(
	AudioContext* const context, const PositionalAudioSourceMetadata* const metadata) {

	add_audio_source_to_audio_context(context, &(PositionalAudioSource) {.metadata = *metadata}, true);
}

void add_nonpositional_audio_source_to_audio_context(
	AudioContext* const context, const ALchar* const path) {

	// TODO: check for no duplicate nonpositional audio sources, and for no duplicate audio clips either
	add_audio_source_to_audio_context(context, &(NonPositionalAudioSource) {.path = path}, false);
}

//////////

static void apply_action_to_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path,
	const ALchar* const action_description, void (*const action) (const ALuint source)) {

	const List* const sources = &context -> nonpositional_sources;

	LIST_FOR_EACH(sources, NonPositionalAudioSource, source,
		if (!strcmp(source -> path, path)) {
			action(source -> al_source);
			return;
		}
	);
	FAIL(UseAudioSource, "Could not %s a nonpositional audio source with path '%s'", action_description, path);
}

void play_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path) {
	apply_action_to_nonpositional_audio_source(context, path, "play", alSourcePlay);
}

void stop_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path) {
	apply_action_to_nonpositional_audio_source(context, path, "stop", alSourceStop);
}

//////////

void update_audio_context(const AudioContext* const context, const Camera* const camera) {
	const ALfloat *const dir = camera -> dir, *const up = camera -> up;

	alListenerfv(AL_POSITION, camera -> pos);
	alListenerfv(AL_VELOCITY, camera -> velocity_world_space);
	alListenerfv(AL_ORIENTATION, (ALfloat[6]) {dir[0], dir[1], dir[2], up[0], up[1], up[2]});	

	const List* const positional_sources = &context -> positional_sources;

	LIST_FOR_EACH(positional_sources, PositionalAudioSource, source,
		const ALuint al_source = source -> al_source;
		const PositionalAudioSourceMetadata* const metadata = &source -> metadata;
		const void* const data = metadata -> data;

		//////////

		metadata -> updater(data, al_source);

		if (metadata -> activator(data)) {
			ALint source_state;
			alGetSourcei(al_source, AL_SOURCE_STATE, &source_state);

			// Stop if already active
			if (source_state == AL_PLAYING) alSourceStop(al_source);
			alSourcePlay(al_source);
		}
	);
}

void deinit_audio_context(const AudioContext* const context) {
	const List
		*const clips = &context -> clips,
		*const positional_sources = &context -> positional_sources,
		*const nonpositional_sources = &context -> nonpositional_sources;

	LIST_FOR_EACH(positional_sources, PositionalAudioSource, source,
		const ALuint al_source = source -> al_source;
		alSourceStop(al_source);
		alDeleteSources(1, &al_source);
	);

	LIST_FOR_EACH(nonpositional_sources, NonPositionalAudioSource, source,
		const ALuint al_source = source -> al_source;
		alSourceStop(al_source);
		alDeleteSources(1, &al_source);
	);

	LIST_FOR_EACH(clips, AudioClip, clip,
		alDeleteBuffers(1, &clip -> al_buffer);
	);

	deinit_list(*nonpositional_sources);
	deinit_list(*positional_sources);
	deinit_list(*clips);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(context -> context);
	alcCloseDevice(context -> device);
}
