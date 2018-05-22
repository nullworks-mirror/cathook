#
# Install base Dependencies
#

sudo apt update && sudo apt install build-essential gcc-multilib g++-multilib software-properties-common -y && sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y && sudo apt update && sudo apt install gcc-snapshot g++-6-multilib gcc-6 g++-6 -y && sudo apt update && sudo apt install git libssl-dev:i386 libboost-all-dev libc6-dev:i386 gdb libsdl2-dev libglew-dev:i386 libglew-dev libfreetype6-dev libfreetype6-dev:i386 cmake libpng-dev libssl-dev gcc-multilib g++-multilib -y

#
# Install libglez
#

git clone --recursive https://github.com/nullworks/libglez.git;cd libglez;mkdir build;cd build;cmake ..;make;sudo make install;cd ..;cd ..

#
# Install libxoverlay
#

git clone --recursive https://github.com/nullworks/libxoverlay.git;cd libxoverlay;mkdir build;cd build;cmake ..;make;sudo make install;cd ..;cd ..

#
# Install Simple-ipc
#

git clone --recursive https://github.com/nullworks/simple-ipc.git;cd simple-ipc;mkdir build;cd build;cmake ..;make;sudo make install;cd ..;cd ..

#
# Build cathook
#

mkdir build;cd build;cmake ..;make -j$(grep -c '^processor' /proc/cpuinfo)
