# Disclaimer
I know that the style sucks. There is a lot of design errors and cancerous patterns. Most of the code was intended to be *temporary*.

# Discord Server
[Official Discord Server](https://discord.gg/7bu3AFw)

# You need g++\-6 to compile/use cathook

### Full install script for ubuntu (installs g++\-6 and cathook)
```
sudo apt update && \
sudo apt install build-essential software-properties-common -y && \
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
sudo apt update && \
sudo apt install gcc-snapshot -y && \
sudo apt update && \
sudo apt install gcc-6 g++-6 g++-6-multilib -y && \
sudo apt install gdb
git clone --recursive https://github.com/nullifiedcat/cathook && \
cd cathook && \
make -j4
```

### Updating cathook
Navigate into cathook directory (where src, makefile and other files are) and run:
```
git pull origin master && \
git submodule update --remote --recursive && \
make clean && \
make -j4
```

# Injection
`sudo ./attach` to attach to tf2 process (can take argument number 0-N - # of tf2 instance to attach to (for bots))

`sudo ./attach-backtrace` to attach and print backtrace if tf2 crashes. Some users reported that this method makes you get less FPS ingame.

# Followbots

To run followbots, you need to download and install `cathook-ipc-server`.

### Installing script
```
git clone --recursive https://github.com/nullifiedcat/cathook-ipc-server && \
cd cathook-ipc-server && \
make -j4
```
### Updating script is the same as updating cathook

### Running followbot server
`./bin/cathook-ipc-server` or `./bin/cathook-ipc-server &>/dev/null &` to run it in background
