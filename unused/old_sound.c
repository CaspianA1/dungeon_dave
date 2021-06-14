typedef struct {
	Uint8* buffer;
	Uint32 length;
	SDL_AudioDeviceID device_id;
	const char* path; // least used member last in struct
} Sound;

// https://lazyfoo.net/SDL_tutorials/lesson11/index.php

inlinable Sound init_sound(const char* path) {
	Sound sound = {.path = path};
	SDL_AudioSpec data;

	if (SDL_LoadWAV(path, &data, &sound.buffer, &sound.length) == NULL)
		FAIL("Could not load a sound: %s\n", path);

	sound.device_id = SDL_OpenAudioDevice(NULL, 0, &data, NULL, 0);
	if (sound.device_id == 0) FAIL("Could not open an audio device: %s\n", SDL_GetError());
	return sound;
}

inlinable void pause_unpause_sound(const Sound sound, const byte pause) {
	SDL_PauseAudioDevice(sound.device_id, pause);
}

inlinable void play_sound(const Sound sound) {
	#ifdef SOUND_ENABLED

	if (SDL_QueueAudio(sound.device_id, sound.buffer, sound.length) < 0)
		FAIL("Could not play the sound %s: %s\n", sound.path, SDL_GetError());

	pause_unpause_sound(sound, 0);

	#else

	(void) sound;

	#endif
}

inlinable void loop_if_finished_sound(const Sound sound) {
	if (SDL_GetQueuedAudioSize(sound.device_id) == 0)
		play_sound(sound);
}

inlinable void deinit_sound(const Sound sound) {
	SDL_CloseAudioDevice(sound.device_id); // maybe do the more complicated method
	SDL_DequeueAudio(sound.device_id, sound.buffer, sound.length);
	SDL_FreeWAV(sound.buffer);
}
