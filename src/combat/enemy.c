/*
The enemy AI:

Five enemy states: Idle, Chasing, Attacking, Retreating, Dead

spritesheet layout:
idle, then chasing, then attacking, then retreating, then dead.
for sounds: idle, chasing, attacking, attacked, dead
*/

void update_enemy(Enemy* enemy, const Player player) {
	enemy -> state = Dead;

	static byte i = 1;

	if (i) play_sound(enemy -> sounds[enemy -> state], 0);

	switch (enemy -> state) {
		case Idle: // stay there, idle animation
			printf("Idle\n");
			break;
		case Chasing: // bfs stuff
			update_path_if_needed(&enemy -> navigator, player.pos, player.jump);
			break;
		case Attacking: // attack animation + fighting
			break;
		case Retreating: // run away to some random vantage point
			break;
		case Dead: // do not move, dead single-sprite animation
			break;
	}
	i = 0;
}

inlinable void update_all_enemies(const Player player) {
	for (byte i = 0; i < current_level.enemy_count; i++)
		update_enemy(&current_level.enemies[i], player);
}

void deinit_enemy(const Enemy enemy) {
	deinit_sprite(enemy.animations.billboard.sprite);
	for (byte i = 0; i < 5; i++)
		deinit_sound(enemy.sounds[i]);
	wfree(enemy.sounds);
	deinit_navigator(enemy.navigator);	
}
