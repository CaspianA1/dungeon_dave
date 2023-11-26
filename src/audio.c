#include "audio.h"
#include "utils/failure.h" // For `FAIL`
#include "utils/safe_io.h" // For `get_temp_asset_path`
#include "utils/debug_macro_utils.h" // TODO: remove

/* TODO:
- Allow for an option to set the game's master volume
- Tying audio to billboards will be trickier later on, but for now, tie sounds to when animated billboards' cycles start
- Maybe use sound handles, instead of strings, as audio keys (not as much use for `Dict`, then)
- Add a function `clear_audio_context` for between levels

- Fix this OpenAL error that happens at startup when there's no title screen:
[ALSOFT] (WW) Error generated on context 0x7fa46001e200, code 0xa003, "Listener velocity out of range"
[ALSOFT] (WW) Error generated on context 0x7fa46001e200, code 0xa003, "Value out of range"
[ALSOFT] (WW) Error generated on context 0x7fa46001e200, code 0xa003, "Value out of range
*/

//////////

/* This converts the bytes data referred to
by `audio_data`, and updates `data_length` too.
Note that both have previously valid values upon function entry,
and that conversion will not happen if it isn't needed. */
static void convert_audio_data(
	const Uint8 new_num_channels,
	const SDL_AudioFormat new_format,
	const ALchar* const path,
	const SDL_AudioSpec* const orig_spec,
	Uint8** audio_data, Uint32* const data_length) {

	////////// Checking if conversion is really needed

	const Uint8 orig_num_channels = orig_spec -> channels;
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

static ALuint init_audio_buffer(const ALchar* const path, const bool is_positional) {
	////////// Loading the original WAV data in

	SDL_AudioSpec audio_spec;
	Uint8* wav_buffer;
	Uint32 wav_length;

	if (SDL_LoadWAV(get_temp_asset_path(path), &audio_spec, &wav_buffer, &wav_length) == NULL)
		FAIL(OpenFile, "Could not load '%s': %s", path, SDL_GetError());

	////////// Setting up some shared vars Converting the WAV data, if needed

	// OpenAL only supports unsigned 8-bit and signed 16-bit
	const SDL_AudioFormat
		supported_sdl_8_bit_format = AUDIO_U8,
		supported_sdl_16_bit_format = AUDIO_S16;

	const Uint8 // Positional audio must be mono for OpenAL
		new_num_channels = is_positional ? 1 : 2,
		orig_num_bits_for_format = SDL_AUDIO_BITSIZE(audio_spec.format);

	////////// Doing a bit conversion

	SDL_AudioFormat sdl_output_format;

	switch (orig_num_bits_for_format) {
		case 8:
			sdl_output_format = supported_sdl_8_bit_format;
			break;

		// TODO: why does converting to 8-bit here not work? Is there some data loss going on?
		case 16: case 32: // Converting to 16 for 32-bit WAV, b/c OpenAL doesn't support it
			sdl_output_format = supported_sdl_16_bit_format;
			break;

		default:
			FAIL(CreateAudioBuffer, "Could not use '%s', because %hhu-bit "
				"WAV formats are not supported.", path, orig_num_bits_for_format);
	}

	convert_audio_data(new_num_channels, sdl_output_format,
		path, &audio_spec, &wav_buffer, &wav_length);

	////////// Finding the AL format

	ALenum al_mono_format, al_stereo_format;

	if (SDL_AUDIO_BITSIZE(sdl_output_format) == 8) {
		al_mono_format = AL_FORMAT_MONO8;
		al_stereo_format = AL_FORMAT_STEREO8;
	}
	else {
		al_mono_format = AL_FORMAT_MONO16;
		al_stereo_format = AL_FORMAT_STEREO16;
	}

	////////// Making an audio buffer, and then freeing the wav buffer

	ALuint al_buffer;
	alGenBuffers(1, &al_buffer);

	alBufferData(al_buffer,
		is_positional ? al_mono_format : al_stereo_format,
		wav_buffer, (ALsizei) wav_length, audio_spec.freq);

	SDL_FreeWAV(wav_buffer);

	return al_buffer;
}

////////// Adding clips and sources to the context

void add_audio_clip_to_audio_context(AudioContext* const context, const ALchar* const path, const bool is_positional) {
	const ALuint audio_buffer = init_audio_buffer(path, is_positional);
	typed_insert_into_dict(&context -> clips, path, audio_buffer, string, unsigned_int);
}

static void add_audio_source_to_audio_context(AudioContext* const context,
	const void* const source_data, const bool source_is_positional, const bool source_loops) {

	const ALchar* const path = source_is_positional
		? ((PositionalAudioSourceMetadata*) source_data) -> path
		: source_data;

	//////////

	const ALuint al_buffer = typed_read_from_dict(&context -> clips, path, string, unsigned_int);

	ALint num_channels;
	alGetBufferi(al_buffer, AL_CHANNELS, &num_channels);
	const bool clip_is_positional = (num_channels == 1);

	if (clip_is_positional != source_is_positional)
		FAIL(CreateAudioSource,
			"Found a clip with the same path as %spositional audio source with path '%s'. "
			"However, the clip was %spositional, which doesn't match up with the audio source.",
			source_is_positional ? "" : "non", path, source_is_positional ? "non" : ""
		);

	ALuint al_source;
	alGenSources(1, &al_source);
	alSourcei(al_source, AL_BUFFER, (ALint) al_buffer);

	if (source_loops) alSourcei(al_source, AL_LOOPING, AL_TRUE);

	if (source_is_positional)
		push_ptr_to_list(&context -> positional_sources, &(PositionalAudioSource) {
			al_source, *(PositionalAudioSourceMetadata*) source_data}
		);
	else
		typed_insert_into_dict(&context -> nonpositional_sources, path, al_source, string, unsigned_int);
}

void add_positional_audio_source_to_audio_context(AudioContext* const context,
	const PositionalAudioSourceMetadata* const metadata, const bool loops) {

	add_audio_source_to_audio_context(context, metadata, true, loops);
}

void add_nonpositional_audio_source_to_audio_context(
	AudioContext* const context, const ALchar* const path, const bool loops) {

	// TODO: make sure that there's no duplicate nonpositional audio sources (don't do that through a `Dict`)
	add_audio_source_to_audio_context(context, path, false, loops);
}

////////// Applying actions to nonpositional sources

static void apply_action_to_nonpositional_audio_source(const AudioContext* const context,
	const ALchar* const path, void (*const action) (const ALuint source)) {

	action(typed_read_from_dict(&context -> nonpositional_sources, path, string, unsigned_int));
}

void play_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path) {
	apply_action_to_nonpositional_audio_source(context, path, alSourcePlay);
}

void stop_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path) {
	apply_action_to_nonpositional_audio_source(context, path, alSourceStop);
}

////////// Init, deinit, and updating

AudioContext init_audio_context(void) {
	#define CHECK_AL_INIT_ERROR(cond, description) if (cond) FAIL(LoadOpenAL,\
		"Could not load OpenAL because it was impossible to %s", description)

	ALCdevice* const device = alcOpenDevice(NULL);
	CHECK_AL_INIT_ERROR(device == NULL, "open a device");

	ALCcontext* const context = alcCreateContext(device, NULL);
	CHECK_AL_INIT_ERROR(context == NULL, "create an OpenAL context");

	CHECK_AL_INIT_ERROR(!alcMakeContextCurrent(context), "make the OpenAL context the current one");

	#undef CHECK_AL_INIT_ERROR

	//////////

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	return (AudioContext) {
		device, context, // TODO: allow for input size guesses
		.positional_sources = init_list(1, PositionalAudioSource),
		.nonpositional_sources = init_dict(1, DV_String, DV_UnsignedInt),
		.clips = init_dict(1, DV_String, DV_UnsignedInt)
	};
}

void deinit_audio_context(const AudioContext* const context) {
	const List* const positional_sources = &context -> positional_sources;

	const Dict
		*const nonpositional_sources = &context -> nonpositional_sources,
		*const clips = &context -> clips;

	LIST_FOR_EACH(positional_sources, PositionalAudioSource, source,
		const ALuint al_source = source -> al_source;
		alSourceStop(al_source);
		alDeleteSources(1, &al_source);
	);

	DICT_FOR_EACH(nonpositional_sources, source,
		const ALuint al_source = source -> value.unsigned_int;
		alSourceStop(al_source);
		alDeleteSources(1, &al_source);
	);

	DICT_FOR_EACH(clips, clip,
		alDeleteBuffers(1, &clip -> value.unsigned_int);
	);

	deinit_dict(&context -> clips);
	deinit_dict(nonpositional_sources);
	deinit_list(*positional_sources);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(context -> context);
	alcCloseDevice(context -> device);
}

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

		/* TODO: perhaps only update when playing? Also, the updater
		should return a set of audio source params instead */
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
