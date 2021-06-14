#include <SDL2/SDL.h>

int main() {
	SDL_Init(SDL_INIT_AUDIO);
	SDL_AudioSpec wavSpec;
	Uint32 wavLength;
	Uint8 *wavBuffer;
	 
	SDL_LoadWAV("assets/audio/storm.wav", &wavSpec, &wavBuffer, &wavLength);
	SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
	int success = SDL_QueueAudio(deviceId, wavBuffer, wavLength);
	if (success == -1) printf("Fail\n");
	SDL_PauseAudioDevice(deviceId, 0);
	SDL_Delay(3000);
	SDL_CloseAudioDevice(deviceId);
	SDL_FreeWAV(wavBuffer);
	SDL_Quit();
}