#ifndef AUDIO_H
#define AUDIO_H

#include "utils/al_include.h" // For various OpenAL defs
#include "camera.h" // For `Camera`
#include "utils/list.h" // For `List`

/* TODO:
- Pass in a shared audio context config to `init_audio_context`
- Perhaps base billboard sounds on their orientation
- Make a sound occlusion model (through reverb)
- Stop the crackle in some way for the weapon sprite
- Add sound for the title screen
- Start using OpenALsoft
*/

//////////

typedef bool (*const audio_source_activator_t) (const void* const data);
typedef void (*const audio_source_metadata_updater_t) (const void* const data, const ALuint al_source);

////////// Positional source types

typedef struct {
	const void* const data;
	const ALchar* const path;
	const audio_source_activator_t activator;
	const audio_source_metadata_updater_t updater;
} PositionalAudioSourceMetadata;

typedef struct {
	ALuint al_source;
	const PositionalAudioSourceMetadata metadata;
} PositionalAudioSource;

////////// Nonpositional source type

typedef struct {
	ALuint al_source;
	const ALchar* const path;
} NonPositionalAudioSource;

//////////

typedef struct {
	ALCdevice* const device;
	ALCcontext* const context;
	List clips, positional_sources, nonpositional_sources;
} AudioContext;

//////////

typedef enum {
	AudioIsPositional = 1,
	AudioLoops = 2
} AudioClipFlags;

//////////

/* Excluded:
convert_audio_data, init_audio_buffer,
add_audio_source_to_audio_context,
apply_action_to_nonpositional_audio_source */

AudioContext init_audio_context(void);

void add_audio_clip_to_audio_context(AudioContext* const context, const ALchar* const path, const byte flags);

void add_positional_audio_source_to_audio_context(AudioContext* const context, const PositionalAudioSourceMetadata* const metadata);
void add_nonpositional_audio_source_to_audio_context(AudioContext* const context, const ALchar* const path);

void play_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path);
void stop_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path);

void update_audio_context(const AudioContext* const context, const Camera* const camera);
void deinit_audio_context(const AudioContext* const context);

#endif
