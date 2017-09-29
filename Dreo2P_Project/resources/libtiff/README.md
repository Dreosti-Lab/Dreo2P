# Building LibTiff from source

Download libtiff [LibTiff](http://libtiff.maptools.org/v4.0.8.html)

Open A "Developer Command Prompt" for Visual Studio 2017
Navigate to LibTiff download folder
Run:
```
nmake /f Makefile.vc
```

Make sure you use the x64 tools for Visual Studio (a different command prompt) to make a 64bit version of the libtiff.lib

Good luck.

- May confloct with NIDAQ (int32 redefintion)