# aromadoom
DoomGeneric port for Android recovery mode using Libaroma

## How I did
Already having libaroma, and thanks to the existence of DoomGeneric, it just was a matter of putting it all together :P  
Libaroma provides the screen and input, Doom provides the game engine and draws to the libaroma screen buffer :)  
BTW button keys (WASD) don't work yet, they are not implemented 
## How to use this
- Build and install [libaroma](https://github.com/MLXProjects/libaroma)
- Using gcc, build all .c files here and link to libaroma
- Push the binary and the res.zip in zip folder to the device
- Run the binary passing the zip path as argument:
```/path/to/binary /path/to/res.zip```
## Complains?
Open an issue and I will be happy to read it, almost nobody looks at this kind of projects xd
