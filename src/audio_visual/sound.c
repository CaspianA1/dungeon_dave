typedef struct {
	byte is_short;
	union {
			Mix_Chunk* short_sound;
			Mix_Music* long_sound;
	} type;
	const char* path;
} Sound;

inlinable void init_audio_subsystem(void) {
	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, AUDIO_CHUNK_SIZE) == -1)
		FAIL("Could not initialize SDL2_mixer: %s\n", Mix_GetError());
}

inlinable void deinit_audio_subsystem(void) {
	Mix_CloseAudio();
	Mix_Quit();
}

inlinable void fail_sound(const Sound sound, const char* const error_type) {
	FAIL("Could not %s a %s sound of path %s: %s\n",
		error_type, sound.is_short ? "short" : "long", sound.path, Mix_GetError());
}

inlinable Sound init_sound(const char* const path, const byte is_short) {
	Sound sound = {.is_short = is_short, .path = path};
	if (is_short) {
		if ((sound.type.short_sound = Mix_LoadWAV(path)) == NULL)
			fail_sound(sound, "initialize");
	}
	else if ((sound.type.long_sound = Mix_LoadMUS(path)) == NULL)
		fail_sound(sound, "initialize");

	return sound;
}

inlinable void deinit_sound(Sound sound) {
	if (sound.is_short) Mix_FreeChunk(sound.type.short_sound);
	else Mix_FreeMusic(sound.type.long_sound);	
}

inlinable void play_sound(const Sound sound, const byte should_loop) {
	#ifndef SOUND_ENABLED

	(void) sound;
	(void) should_loop;
	return;

	#endif

	const int loop_status = should_loop ? -1 : 0;

	if (sound.is_short) {
		if (Mix_PlayChannel(-1, sound.type.short_sound, loop_status) == -1)
			fail_sound(sound, "play");
	}
	else if (Mix_PlayMusic(sound.type.long_sound, loop_status) == -1)
		fail_sound(sound, "play");
}
