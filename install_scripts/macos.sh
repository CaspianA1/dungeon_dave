# This script assumes that brew, make, and git are installed.

# Params: usermame, project name
get_from_github() {
	git clone git@github.com:$1/$2.git
}

install_cglm() {
	get_from_github recp cglm
	mv cglm/include/cglm temp
	sudo rm -r cglm
	mv temp cglm
}

install_openal_soft() {
	get_from_github kcat openal-soft
	cd openal-soft/build

	cmake .. # TODO: test for failure for these two
	make -j OpenAL

	cd ../..
	mkdir openal
	mv openal-soft/include/AL/* openal-soft/build/*.dylib openal

	sudo rm -r openal-soft
}

main() { # TODO: install SDL locally
	brew install pkg-config sdl2 cmake

	cd ../lib
	install_cglm
	install_openal_soft
}

main
