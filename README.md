# Eurobattle.net PvPGN fork

This is the old berlios upstream with a few patches and not much else.

## VS 14 build
```
cd pvpgn
cmake -G "Visual Studio 14 2015" -H./ -B./build -DHAVE_VSNPRINTF=1
```