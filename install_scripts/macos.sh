# This script assumes that the developer tools are installed.
# That should include clang, make, and git.

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
