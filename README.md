# Eurobattle.net PvPGN fork

This is the old berlios upstream with a few patches and not much else.

## VS 14 build
```
cd pvpgn
cmake -G "Visual Studio 14 2015" -H./ -B./build -DHAVE_VSNPRINTF=1 -DCMAKE_PREFIX_PATH="C:/Users/klemen/git/pvpgn/pvpgn/depends/libcurl;C:/Users/klemen/git/pvpgn/pvpgn/depends/zlib" -DCURL_STATICLIB=ON -DWITH_WIN32_GUI=OFF -DWITH_PF=ON
```
Make sure you use absolute paths for CMAKE_PREFIX_PATH.

Find package debugging:
```
-DCMAKE_FIND_DEBUG_MODE=1 
```