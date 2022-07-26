# This script assumes that brew and git are installed.

# Params: usermame, project name
get_from_github() {
	git clone git@github.com:$1/$2.git
}

install_cglm() {
	get_from_github recp cglm
	cp -r cglm/include/cglm ../include/cglm
	sudo rm -r cglm
}

main() {
	brew install pkg-config sdl2 cmake
	install_cglm
}

main

# After this, go to the `build` directory, and run `cmake .. && make && ./dungeon_dave.`
