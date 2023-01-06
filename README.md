# ImHex Discord Rich Presence

Ths Plugin adds Discord Rich Presence support to ImHex

![image](https://user-images.githubusercontent.com/10835354/211030126-37ea5d52-07e3-468b-82dc-18ccf971f128.png)

## Building

Building a plugin works similarly to building ImHex. Make sure you have all dependencies installed that are necessary to build ImHex itself. Afterwards simply use cmake in the top level of this repository to build libimhex and the plugin. Consult the ImHex README.md for a more in-depth guide on how to compile ImHex using cmake.

When installing, make sure to place the plugin file in the ImHex `plugins` folder and move the discord sdk libraries to a location where ImHex picks them up (next to the executable, in a system library folder, etc)
