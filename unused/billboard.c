inlinable Billboard init_billboard(const char* path) {
	Billboard billboard = {init_sprite(path), {0, 0}, 0, 0};
	return billboard;
}

inlinable Billboard* init_billboards(const int amount, ...) {
	va_list paths;
	va_start(paths, amount);
	Billboard* billboards = calloc(amount, sizeof(Billboard));

	for (int i = 0; i < amount; i++)
		billboards[i] = init_billboard(va_arg(paths, const char*));

	va_end(paths);
	return billboards;
}

inlinable void deinit_billboard(Billboard billboard) {
	deinit_sprite(billboard.sprite);
}

inlinable void deinit_billboards(Billboard* billboards, const int amount) {
	for (int i = 0; i < amount; i++)
		deinit_billboard(billboards[i]);
	free(billboards);
}
