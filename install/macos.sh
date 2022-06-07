# This script assumes that git, make, brew, and clang are installed (they come in the developer tools package)

brew install sdl2 cmake

git clone git@github.com:recp/cglm.git
cd cglm
mkdir build
cd build
cmake ..
make
sudo make install
cd ../..
sudo rm -r cglm
