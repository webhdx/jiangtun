# Jiangtun (江豚)

GameCube automation alternative firmware for RP2040.

- [NX Macro Controller](https://blog.bzl-web.com/entry/2020/01/20/165719)
- [Poke-Controller Modified](https://github.com/Moi-poke/Poke-Controller-Modified)
- [ORCA GC Controller](https://github.com/yatsuna827/Orca-GC-Controller)

## Pin assignment

| Pico  | XIAO RP2040 |  RP2040-One  |      Function      |
| :---: | :---------: | :----------: | :----------------: |
|   7   |     D5      |     GP0      |   GameCube DATA    |
|   0   |     D6      | GP18 (unused) |       Servo        |
|   3   |     D10     | GP19 (unused) | Reset (Active Low) |

## Button mapping

| NXMC2/PokeCon | GameCube |
| :-----------: | :------: |
|       Y       |    Y     |
|       B       |    B     |
|       A       |    A     |
|       X       |    X     |
|       L       |    L     |
|       R       |    R     |
|      ZL       |          |
|      ZR       |    Z     |
|       -       |          |
|       +       |  Start   |
|    L Click    |          |
|    R Click    |          |
|     Home      |  Reset   |
|    Capture    |          |

## Development

This directory is a [PlatformIO](https://platformio.org/) project. Clone with `--recursive` as it contains submodules. Note [the path character length limit in Git for Windows](https://arduino-pico.readthedocs.io/en/latest/platformio.html#important-steps-for-windows-users-before-installing).

```
git config --system core.longpaths true
```

## Reference

- [GC自動化ver3アップデート](https://note.com/gamewagashi/n/n026a29d00a85)
