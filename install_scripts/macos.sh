# This script assumes that the developer tools are installed.
# That should include clang, make, and git.

build_from_github() {
	repo_name=$1
	project_name=$2

	git clone git@github.com:$repo_name/$project_name.git

	cd $project_name
	mkdir build
	cd build
	cmake ..
	make
	sudo make install
	cd ../..
	sudo rm -r $project_name
}

main() {
	brew install pkg-config sdl2 cmake
	build_from_github recp cglm
	build_from_github DaveGamble cJSON
}

main
