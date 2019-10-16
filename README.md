# Cathook Training Software
![banner](http://i.imgur.com/w96wdtE.png)
[![CircleCI](https://circleci.com/gh/nullworks/cathook.svg?style=svg)](https://circleci.com/gh/nullworks/cathook)

## Risk of VAC detection

The software could be detected by VAC in the future. Only use it on accounts you won't regret getting VAC banned.

## Community
You can chat with other cathook users in [our official telegram group](https://t.me/nullworks) and the [cathook announcements channel](https://t.me/cathook_cheat).

## Reporting Issues

If some things doesn't work as expected, please open issues on GitHub using [this page](https://github.com/nullworks/cathook/issues).

## Contributing

Do you want to submit code to cathook? Please read `CONTRIBUTING.md` for a short introduction.

## Overview

Cathook is a training software designed for Team Fortress 2 for Linux. Cathook includes some joke features like

* Ignore Hoovy
* Encrypted chat
* IRC Support (Find other Cathook users in-game automatically)
* Chance to get manually VAC banned by Valve

and a lot of useful features, including

* Anti Backstab with option to use "No" voice command when spy tries to backstab you
* Extremely customizable spam (you can make spam lines that'll include name of random dead enemy pyro or sniper)
* Follow Bots
* Navparser Bots (Walkbots than can walk on any map without manual configuration)
* Working crit hack (does not work right now (works right now))

[FULL LIST OF FEATURES HERE](https://github.com/nullworks/cathook/wiki/Feature-List-and-explanations) (list might be outdated)

# INSTALLATION

## Automatic: (Ubuntu 18.04+/Fedora/Arch based only)
Run in terminal:

* `bash <(wget -qO- https://raw.githubusercontent.com/nullworks/One-in-all-cathook-install/master/install-all)`

## Manual:

### Clone Cathook:
#### User mode:
```git clone --depth 1 https://github.com/nullworks/cathook```
#### Developer mode:
```git clone --recursive https://github.com/nullworks/cathook```

### Install dependencies:
#### Arch/Manjaro:
```
git boost cmake make gcc gdb lib32-sdl2 lib32-glew lib32-freetype2 rsync lib32-libglvnd dialog
```
#### Ubuntu
```
software-properties-common build-essential git g++ g++-multilib libboost-all-dev gdb libsdl2-dev:i386 libglew-dev:i386 libfreetype6-dev:i386 cmake dialog rsync
```
#### Fedora
```
cmake dialog make gcc-c++ glibc-devel.i686 freetype-devel.i686 SDL2-devel.i686 glew-devel.i686 boost-devel.i686 rsync gdb git
```
#### Other distros
You will have to find matching packages yourself.
### Compile Cathook:
Go into the cathook directory and run
```./update```

## Updating Cathook
Run the `update` script in Cathook folder.

Cathook requires a special data folder (contains shaders, font files, walkbot paths, etc). This folder is located at `/opt/cathook/data` and is generated automatically when you compile Cathook.

## Injection
`sudo ./attach` to attach Cathook into TF2. Optionally, you can provide a PID (for bots or other linux users).

`sudo ./attach-gdb` to attach and print backtrace in case TF2 crashes. Requires a debug Cathook build (run `./config` to configure the build mode). Some users report that this causes FPS drop in-game. This is recommended if you want to grab a log of what went wrong if Cathook is crashing on you ([report it!](https://github.com/nullworks/cathook/issues))

## Followbots (outdated)
`cathook-ipc-server` allows you to run and control Followbots to do your evil bidding in-game. The installation for Followbots is quite complex, and will not be covered on this page. Obviously, you must have several user accounts ready to run TF2.  
A guide for Followbots can be found here: [How to setup and use followbots.](https://www.youtube.com/watch?v=kns5-nw7xUg)  

The installation script is as followed:
```bash
git clone --recursive https://github.com/nullworks/cathook-ipc-server && cd cathook-ipc-server && make -j4
```
To run the Followbot server, run `./bin/cathook-ipc-server`. You can also use `./bin/cathook-ipc-server &>/dev/null &` to run it in background.
