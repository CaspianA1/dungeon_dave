#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// https://www.libsdl.org/projects/SDL_mixer/docs/SDL_mixer_82.html
// cl; gcc -lSDL2 -lSDL2_mixer mixer_tests.c && ./a.out

typedef uint_fast8_t byte;

#define AUDIO_CHUNK_SIZE 4096
#define DEBUG(var, format) printf(#var " = %" #format "\n", var)



void sound_dist_test(void) {
	Mix_Chunk* const sound = Mix_LoadWAV("../assets/audio/sound_effects/whip_crack.wav");

	for (byte i = 1; i < 255; i++) { // silent at 255
		DEBUG(i, d);
		const int channel = Mix_PlayChannel(-1, sound, 0);
		Mix_SetDistance(channel, i);
		SDL_Delay(800);
	}

	Mix_FreeChunk(sound);
}

void sound_pos_test(void) {
	Mix_Chunk* const sound = Mix_LoadWAV("../assets/audio/enemy_sound_test/chase.wav");

	// 0 = in front, 90 = to right, 180 = behind, 270 = to left, 360 = in front again

	for (int i = 1; i < 360; i++) { // not 1 b/c that unregisters from sound channel
		DEBUG(i, d);
		const int channel = Mix_PlayChannel(-1, sound, 0);
		Mix_SetPosition(channel, i, 0);
		SDL_Delay(800);
	}

	Mix_FreeChunk(sound);
}

int main(void) {
	SDL_Init(SDL_INIT_AUDIO);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, AUDIO_CHUNK_SIZE);
	// sound_dist_test();
	sound_pos_test();
	SDL_Quit();
}
