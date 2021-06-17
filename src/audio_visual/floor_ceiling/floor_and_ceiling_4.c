/*
1: #3 is faster than #1 and #2. SIMD makes a big difference, but not a monumental one.
2: Find out my assumption that's making #3 only work with a 66 degree FOV.
3: Once #3 is fixed, #4 can utilize SIMD too to maximize performance.

https://lodev.org/cgtutor/raycasting2.html
For 90 degrees, the floor doesn't turn when turning
For 60 degrees, there's no texture misalignment problem
Find other horizontal floor code on Github
*/

void lodev_expanded_fc(const Player player) {
	const int screenWidth = settings.screen_width, screenHeight = settings.screen_height;
	const double angle = to_radians(player.angle);
	const double height_over_proj_dist = settings.screen_height / settings.proj_dist;
	// multiply by a magic ratio for the two angles
	const double dirX = cos(angle) / height_over_proj_dist, dirY = sin(angle) / height_over_proj_dist;
	const double planeX = -dirY, planeY = dirX;
	const double posX = player.pos[0], posY = player.pos[1];
	const Sprite sprite = current_level.walls[2];
	const SDL_Surface* restrict surface = sprite.surface;
	const int texWidth = surface -> w, texHeight = surface -> h;

	//FLOOR CASTING
	for(int y = 0; y < screenHeight; y++) {
	  // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
	  double rayDirX0 = dirX - planeX;
	  double rayDirY0 = dirY - planeY;
	  double rayDirX1 = dirX + planeX;
	  double rayDirY1 = dirY + planeY;

	  // Current y position compared to the center of the screen (the horizon)
	  int p = y - screenHeight / 2;

	  // Vertical position of the camera.
	  double posZ = 0.5 * screenHeight;

	  // Horizontal distance from the camera to the floor for the current row.
	  // 0.5 is the z position exactly in the middle between floor and ceiling.
	  double rowDistance = posZ / p;

	  // calculate the real world step vector we have to add for each x (parallel to camera plane)
	  // adding step by step avoids multiplications with a weight in the inner loop
	  double floorStepX = rowDistance * (rayDirX1 - rayDirX0) / screenWidth;
	  double floorStepY = rowDistance * (rayDirY1 - rayDirY0) / screenWidth;

	  // real world coordinates of the leftmost column. This will be updated as we step to the right.
	  double floorX = posX + rowDistance * rayDirX0;
	  double floorY = posY + rowDistance * rayDirY0;

	  for(int x = 0; x < screenWidth; x++) {
	    // the cell coord is simply got from the integer parts of floorX and floorY
	    int cellX = (int) (floorX);
	    int cellY = (int) (floorY);

	    int tx = (int)(texWidth * (floorX - cellX)) & (texWidth - 1);
	    int ty = (int)(texHeight * (floorY - cellY)) & (texHeight - 1);

	    floorX += floorStepX;
	    floorY += floorStepY;

	    const Uint32 pixel = get_surface_pixel(surface -> pixels, surface -> pitch, tx, ty);
	    *get_pixbuf_pixel(x, y) = pixel;
		}
	}
}
