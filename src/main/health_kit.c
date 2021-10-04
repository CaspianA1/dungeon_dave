static Sprite health_kit_sprite;
static Sound health_kit_sound;

void init_health_kit_resources(void) {
	health_kit_sprite = init_sprite("assets/objects/health_kit.bmp", 0);
	health_kit_sound = init_sound("assets/audio/sound_effects/teleporter_zap.wav", 1);
}

void deinit_health_kit_resources(void) {
	deinit_sprite(health_kit_sprite);
	deinit_sound(&health_kit_sound);
}
