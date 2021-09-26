#ifdef SOUND_ENABLED

static const byte quietest_sound_dist = 10, num_sound_channels = 20;
static const char* const out_of_channel_error = "No free channels available";

typedef struct {
	byte is_short;
	union { // `short` and `long` alone are datatypes, so wouldn't work
			Mix_Chunk* short_sound;
			Mix_Music* long_sound;
	} type;
	const char* path;
} Sound;

static void when_channel_done(const int channel) {
	Mix_UnregisterAllEffects(channel);
}

inlinable void init_audio_subsystem(void) {
	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, AUDIO_CHUNK_SIZE) == -1)
		FAIL("Could not initialize SDL2_mixer: %s\n", Mix_GetError());

	Mix_ChannelFinished(when_channel_done);
	Mix_AllocateChannels(num_sound_channels);
}

#define deinit_audio_subsystem Mix_CloseAudio

void fail_sound(const Sound* const sound, const char* const error_type) {
	FAIL("Could not %s a %s sound of path %s: %s\n",
		error_type, sound -> is_short ? "short" : "long", sound -> path, Mix_GetError());
}

Sound init_sound(const char* const path, const byte is_short) {
	Sound sound = {.is_short = is_short, .path = path};
	if (is_short) {
		if ((sound.type.short_sound = Mix_LoadWAV(path)) == NULL)
			fail_sound(&sound, "initialize");
	}
	else if ((sound.type.long_sound = Mix_LoadMUS(path)) == NULL)
		fail_sound(&sound, "initialize");

	return sound;
}

inlinable void deinit_sound(const Sound* const sound) {
	if (sound -> is_short) Mix_FreeChunk(sound -> type.short_sound);
	else Mix_FreeMusic(sound -> type.long_sound);
}

//////////

static int play_short_sound(const Sound* const sound) { // returns the channel played on
	const int channel = Mix_PlayChannel(-1, sound -> type.short_sound, 0);
	if (channel == -1 && strcmp(Mix_GetError(), out_of_channel_error) != 0)
		fail_sound(sound, "play");

	return channel;
}

//////////

void play_sound_from_billboard_data(const Sound* const sound,
	const DataBillboard* const billboard_data, const vec p_pos, const double p_height) {

	const int beta_degrees = billboard_data -> beta * 180.0 / M_PI;

	const vec delta_2D = billboard_data -> pos - p_pos;
	const double delta_height = billboard_data -> height - p_height;

	const double distance_3D = sqrt(delta_2D[0] * delta_2D[0] + delta_2D[1] * delta_2D[1] + delta_height * delta_height);
	const double distance_3D_percent = distance_3D / quietest_sound_dist;

	int audio_library_distance = distance_3D_percent * 255;
	if (audio_library_distance == 0) audio_library_distance = 1;
	else if (audio_library_distance > 254) audio_library_distance = 254;

	const int channel = play_short_sound(sound);
	Mix_SetPosition(channel, 360 - beta_degrees, audio_library_distance);
}

// this loops long sounds
void play_sound(const Sound* const sound) {
	if (sound -> is_short) play_short_sound(sound);
	else if (Mix_PlayMusic(sound -> type.long_sound, -1) == -1) fail_sound(sound, "play");
}

#else

typedef byte Sound;
#define init_audio_subsystem()
#define deinit_audio_subsystem()
#define fail_sound(a, b)
#define init_sound(a, b) 0
#define deinit_sound(a) (void) a
#define play_sound_from_billboard_data(a, b, c, d) {(void) c; (void) d;}
#define play_sound(a)

#endif
