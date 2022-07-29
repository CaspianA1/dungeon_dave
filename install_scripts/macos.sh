# This script assumes that brew and git are installed.

# Params: usermame, project name
get_from_github() {
	git clone git@github.com:$1/$2.git
}

install_cglm() {
	get_from_github recp cglm
	cp -r cglm/include/cglm ../include/lib/cglm
	sudo rm -r cglm
}

main() {
	brew install pkg-config sdl2 cmake
	install_cglm
}

main
