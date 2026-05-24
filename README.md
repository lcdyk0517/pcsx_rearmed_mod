PCSX-ReARMed - yet another PCSX fork
====================================

## MOD Features (lcdyk0517)
* **Built-in cheat database**: 5300+ games with 56000+ cheat codes embedded
  directly in the core. Cheats appear as dynamic core options under the
  "Cheats" category. No external cheat files needed.
* **Full simplified Chinese localization**: all core options, categories,
  and value labels translated to Chinese. The "Cheats" category includes
  a contact for reporting issues or requesting new cheats.
* **Cheat update workflow**: run `tools/gen_cheat_db.py` to regenerate the
  embedded database from `.CHT` files, then rebuild.

## Original README

![CI (Linux)](https://github.com/notaz/pcsx_rearmed/workflows/CI%20(Linux)/badge.svg)

PCSX ReARMed is yet another PCSX fork based on the PCSX-Reloaded project,
which itself contains code from PCSX, PCSX-df and PCSX-Revolution. This
version was originally ARM architecture oriented (hence the name) with
its MIPS->ARM dynamic recompilation and assembly optimizations, but more
recently it targets other architectures too. A fork of this emulator was
used in [PS Classic](https://en.wikipedia.org/wiki/PlayStation_Classic)
(without any coordination or even notification).

## Features
* ARM/ARM64 dynamic recompiler by Ari64
* [lightrec](https://github.com/pcercuei/lightrec/) dynamic recompiler for other architectures
* NEON GPU by Exophase for ARM NEON and x86 SSE2+
* PCSX4ALL GPU by Una-i/senquack for other architectures
* heavily modified P.E.Op.S. SPU
* BIOS HLE emulation (most games run without proprietary BIOS)
* libretro support

