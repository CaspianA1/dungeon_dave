# This script assumes that brew, make, git, and a valid C compiler are installed.

##########

# Params: command as a string, command to run upon failure as a string
try_command() {
  eval $1

  if [[ $? != 0 ]]; then
    echo "This command failed: '$1'"
    exit 1
  fi
}

# Params: the needed command, action to take
run_if_cmd_is_nonexistent() {
	if ! command -v $1 &> /dev/null; then
		try_command "eval $2"
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

	##### If on MacOS, using .dylib files instead

	library_file_end="so"

	if [[ `uname` == "Darwin" ]]; then
		library_file_end="dylib"
	fi

	##### Then, moving everything needed into its own directory, and removing everything else

	mkdir temp
	mv $3 build/$4*.$library_file_end* temp

	project_dir_name=`basename $PWD`

	mv temp ..
	cd ..
	sudo rm -r $project_dir_name
	mv temp $5
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

# Params: package manager
# package manager's installation command,
# dependencies as a string (separated by spaces)
install_dependencies() {
	package_manager="$1"
	installation_command="$2"
	dependencies_as_string="$3"

	run_if_cmd_is_nonexistent\
		"$package_manager"\
		"echo \"You need $package_manager to install dependencies on your platform.\"; exit 1"

	for dependency in $dependencies_as_string; do
		echo "> Checking if dependency $dependency should be installed"
		run_if_cmd_is_nonexistent "$dependency" "$installation_command $dependency"
	done
}

install_dependencies_for_macos() {
	install_dependencies "brew" "brew install" "make cmake git pkg-config sdl2"
}

install_dependencies_for_linux() {
	install_dependencies "dnf" "sudo dnf install" "clang make cmake git pkg-config SDL2-devel"
}

##########

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

	if [[ $files_without_glad != "" ]]; then
		sudo rm -r $files_without_glad
	fi

	##### Then, installing everything

	install_cglm
	install_cjson
	install_openal_soft
}

main
