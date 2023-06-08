# This script assumes that brew, make, git, and a valid C compiler are installed.

##########

# Param: command as a string, command to run upon failure as a string
try_command() {
	eval $1 || (echo "This command failed: \"$1\""; exit 1)
}

# Params: the needed command, additional message
fail_if_cmd_is_nonexistent() {
	if ! command -v $1 &> /dev/null; then
		echo "The command $1 doesn't exist, which is needed for installation. $2"
		exit 1
	fi
}

# Params: usename, project name
get_from_github() {
	try_command "git clone https://github.com/$1/$2"
}

# Params: cmake flags, make flags, header files, object file prefix, renamed project
do_cmake_build() {
	##### First, doing a CMake build

	# Adding `-p`, in case `build` already exists
	mkdir -p build
	cd build
	try_command "cmake .. $1"
	try_command "make -j $2"
	cd ..

	# TODO: abstract out the `.so` suffix
	mkdir temp
	mv $3 build/$4.so* temp

	##### Then, moving everything needed into its own directory, and removing everything else

	project_dir_name=`basename $PWD`
	mv temp ../$5
	cd ..

	sudo rm -r $project_dir_name
}

##########

install_cjson() {
	get_from_github DaveGamble cJSON
	cd cJSON
	do_cmake_build "-DENABLE_CJSON_TEST=Off" "" "cJSON.h" libcjson cjson
}

install_cglm() {
	get_from_github recp cglm
	mv cglm/include/cglm temp
	sudo rm -r cglm
	mv temp cglm
}

install_openal_soft() {
	get_from_github kcat openal-soft
	cd openal-soft
	do_cmake_build "" "OpenAL" "include/AL/*" libopenal openal
}

##########

install_dependencies_for_macos() {
	fail_if_cmd_is_nonexistent "brew" ""
	try_command "brew install make cmake git pkg-config sdl2"
}

install_dependencies_for_linux() {
	fail_if_cmd_is_nonexistent "dnf" "Only Fedora is supported for Linux distros at the moment."
	try_command "sudo dnf install clang make cmake git pkg-config SDL2-devel"
}

main() {
	##### First, installing some stuff via the system's package manager

	platform_name=`uname`

	case $platform_name in
		"Darwin") install_dependencies_for_macos;;
		"Linux") install_dependencies_for_linux;;
		*) echo "Unsupported platform '$platform_name'"; exit 1;;
	esac

	##### Then, removing everything that's not `glad` from `lib`

	cd lib

	files_without_glad=`ls | grep -xv "glad"`

	if [[ $files_without_glad != "" ]] then
		echo "REMOVING FILES"
		rm -r $files_without_glad
	else
		echo "KEEPING FILES"
	fi

	##### Then, installing everything

	install_cglm
	install_cjson
	install_openal_soft
}

main
