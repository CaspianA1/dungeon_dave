#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#define AUDIO_CHUNK_SIZE 4096

const char* sounds[7] = {
	"../assets/audio/jump_land.wav",
	"../assets/audio/jump_up.wav",
	"../assets/audio/shotgun.wav",
	"../assets/audio/ambient_wind.wav",
	"../assets/audio/storm.wav",
	"../assets/audio/sultan.wav",
	"../assets/audio/title.wav"
};

int main(void) {
	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, AUDIO_CHUNK_SIZE) == -1) {
		printf("Mix failed!\n");
		return 1;
	}

	for (int i = 0; i < 7; i++) {
		const char* sound_path = sounds[i];
		Mix_Music* sound = Mix_LoadMUS(sound_path);
		if (sound == NULL) {
			printf("Sound load failed for %s: %s\n", sound_path, Mix_GetError());
			return 2;
		}
		Mix_PlayMusic(sound, 1);
		SDL_Delay(1000);
		Mix_FreeMusic(sound);
	}
	Mix_CloseAudio();
	Mix_Quit();
}
