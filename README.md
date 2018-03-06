# GBX

GBX is a lightweight Scene-Entity game engine for the Gamebuino-META console (https://gamebuino.com/). It is an attempt at creating a fully featured game engine that runs on minimalistic systems.

Demos can be found on the companion repo: https://github.com/jlauener/GBX-demos

# Disclaimer

GBX is still very early in development so its API might change.

# Features
* Scene-Entity system: Provides easy to use Scene and Entity objects
* Debug console: Shows metrics and hitboxes
* Renderables: Sprites, animators and tilemaps
* Animation: Support looping and one-shot animations, animation data is stored in PROGMEM
* Layers: Optionally layers can be used to display renderables
* Collision system: AABB collision system with pixel perfect movement and callbacks
* Memory management: Cached dynamic allocation and entity pools (no fragmentation!)

# Roadmap (a.k.a the idea box)

* Text/labels with different font and alignement
* Map entity and grid based collision
* More animation types: random, ping-pong
* Sfx/music
* UI components
* Asset pipeline
