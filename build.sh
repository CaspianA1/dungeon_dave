build_type=$1

case $build_type in
	"debug") cmake_build_type="Debug";;
	"release") cmake_build_type="Release";;
	*) echo "Unrecognized build type: '$build_type'. Exiting."; exit 1;;
esac

##########

mkdir -p build/$build_type
cd build/$build_type

cmake -DCMAKE_BUILD_TYPE=$cmake_build_type ../.. || exit 2
make || exit 3
cd ..

if [ "$2" == "run" ]; then
	echo "\nLaunching application\n"
	./$build_type/dungeon_dave
fi
