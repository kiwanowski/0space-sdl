# 0Space — SDL3 port

A native C/SDL3 port of *0Space* (Beau Blyth / Teknopants, music by Max Coburn),
originally built in GameMaker 7.

The original game's code and assets are the authors' copyright.

## Building

Needs SDL3, SDL3_image, SDL3_mixer and Python 3 with Pillow.

```bash
cmake -S . -B build && cmake --build build -j8
```

Options: `--scale N`, `--fullscreen`, `--room N`, `--list-rooms`