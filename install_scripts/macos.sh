# This script assumes that the developer tools are installed.
# That should include clang, make, and git.

brew install pkg-config sdl2 cmake

git clone git@github.com:recp/cglm.git
cd cglm
mkdir build
cd build
cmake ..
make
sudo make install
cd ../..
sudo rm -r cglm
