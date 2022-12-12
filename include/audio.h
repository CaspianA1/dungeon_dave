#ifndef AUDIO_H
#define AUDIO_H

#include "openal/al.h" // For various OpenAL defs
#include "openal/alc.h" // For various ALC defs

#include "camera.h" // For `Camera`
#include "utils/list.h" // For `List`
#include "utils/dict.h" // For `Dict`

/* TODO:
- Pass in a shared audio context config to `init_audio_context`
- Perhaps base billboard sounds on their orientation
- Make a sound occlusion model (through reverb)
- Add a unique track for the title screen
- Use audio initers/updaters for nonpositional audio sources? Menu button clicks, for example
*/

//////////

typedef bool (*const audio_source_activator_t) (const void* const data);
typedef void (*const audio_source_metadata_updater_t) (const void* const data, const ALuint al_source);

////////// Positional source types

typedef struct {
	const ALchar* const path;
	const void* const data;
	const audio_source_activator_t activator;
	const audio_source_metadata_updater_t updater;
} PositionalAudioSourceMetadata;

typedef struct {
	ALuint al_source;
	const PositionalAudioSourceMetadata metadata;
} PositionalAudioSource;

//////////

typedef struct {
	ALCdevice* const device;
	ALCcontext* const context;

	List positional_sources;

	// Paths -> OpenAL sources, and paths -> OpenAL buffers
	Dict nonpositional_sources, clips;
} AudioContext;

//////////

/* Excluded:
convert_audio_data, init_audio_buffer,
add_audio_source_to_audio_context,
apply_action_to_nonpositional_audio_source */

AudioContext init_audio_context(void);
void deinit_audio_context(const AudioContext* const context);

//////////

void add_audio_clip_to_audio_context(AudioContext* const context,
	const ALchar* const path, const bool is_positional);

void add_positional_audio_source_to_audio_context(AudioContext* const context,
	const PositionalAudioSourceMetadata* const metadata, const bool loops);

void add_nonpositional_audio_source_to_audio_context(
	AudioContext* const context, const ALchar* const path, const bool loops);

//////////

void play_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path);
void stop_nonpositional_audio_source(const AudioContext* const context, const ALchar* const path);

void update_audio_context(const AudioContext* const context, const Camera* const camera);

#endif
