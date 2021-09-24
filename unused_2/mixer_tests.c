#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

typedef uint_fast8_t byte;

#define AUDIO_CHUNK_SIZE 4096
#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

// cl; gcc -lSDL2 -lSDL2_mixer mixer_tests.c && ./a.out

// quietest at 255
void sound_at_dist(Mix_Chunk* const sound, const byte distance) {
	const int channel = Mix_PlayChannel(-1, sound, 0);
	Mix_SetDistance(channel, distance);
	// SDL_Delay(800);
}

void sound_test(const int seconds) {
	Mix_Chunk* const sound = Mix_LoadWAV("assets/audio/sound_effects/whip_crack.wav");

	for (byte i = 1; i < 255; i += 9) {
		DEBUG(i, d);
		sound_at_dist(sound, i);
	}

	Mix_FreeChunk(sound);
}

int main(void) {
	int a = SDL_Init(SDL_INIT_AUDIO);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, AUDIO_CHUNK_SIZE);
	sound_test(10);
	SDL_Quit();
}
