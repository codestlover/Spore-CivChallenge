
# Civ Challenge

A challenge mod for the **Civilization stage** of *Spore*. It takes away the player's strongest shortcuts and leaves
the AI nations untouched. Every rule is a separate switch in the game's own settings window, and all mod texts follow
the game language in all 23 Spore locales.

Current version: **1.0.1**. Download `CivChallenge.sporemod` from the [Releases](../../releases) page.

## Rules

All rules apply to **the player only** and **only in the Civilization stage**. AI nations keep the normal rules,
and other stages are not changed.

| Rule | What it does |
| --- | --- |
| **No land raids** | Your land vehicles cannot raid tribes. Raid orders that were already given are cancelled. |
| **3 turrets per city** | The 4th turret is not built. At 3 turrets the purchase card in the city editor is disabled and shows the game's own "no free slots" tooltip. |
| **4 spice sources** | You can own at most 4 spice sources on the whole planet, land and sea together. Towers under construction reserve a place. Losing a source frees one. At the limit you can still attack or convert other nations' sources, but a source you win becomes neutral instead of yours. |
| **No superweapons** | All 12 superweapon panel abilities (military, religious and economic) are disabled, including their hotkeys. Nothing is paid and no cooldown starts. |
| **No compliments** | The *Compliment* answer is disabled when you talk to any empire, like an answer you cannot afford. |
| **3 gifts per empire** | Each nation accepts at most 3 paid gifts of any size (1000, 2000 or 4000). Every nation has its own counter. After the third gift the *Gift* entry and all amounts for that nation are disabled. |

Details that matter in play:

- Only paid gifts count. A gift you could not afford does not use the limit, gifts made while the rule is off are
  not counted, and paying tribute that another nation demanded is not a gift.
- The gift counters are stored in your saved game. They start empty in a new game and are restored when you load
  a save, including a reload after a defeat.
- At the spice limit you cannot claim a free source or buy one. Military and religious attacks on other nations'
  sources still go ahead, with the usual diplomatic cost, and a source your side wins is released as neutral. A
  capture or conversion by your vehicles, and a purchase that was already paid for when the limit filled up, play
  the refusal sound; the money for such a purchase is lost.
- Turning the turret or spice rule back on applies it at once: extra turrets are removed and extra sources are
  released (the first 4 in the game's own list are kept). The same happens when you load an older save or take
  over a city that is over the limit. These are normal game-state changes, so disabling or removing the mod later
  does not bring them back. For a fresh run, enable the mod before the Civilization stage starts.

Trying a forbidden action plays Spore's native **refusal sound**, including clicks on disabled cards, buttons and
answers. A group order plays one cue. Hovering, UI refreshes and background checks are silent. No extra sound files
are used, so the game's volume settings apply.

On every game launch a native blue information banner, *civilization challenge activated*, appears once in the
galaxy menu. If the mod cannot activate, a message says so instead.

## Other mods

Civ Challenge works together with other mods that hook the same game functions, for example CivDrive (manual
control of civilization vehicles), which also hooks vehicle orders.
When another mod has already put its hook on one of these functions, Civ Challenge puts its own in front of it and
calls through to the other mod's hook, so both run and the load order does not matter. Only a jump into another
loaded module's code is accepted there; any other change to the game's code still stops the mod with the usual
message. The rules apply the same way: a raid that another mod asks for on your behalf is refused like your own.

## Settings

Open **Settings → Civ Challenge**. The six switches are grouped in three collapsible sections: *Warfare* (land raids,
superweapons), *Cities & spice* (turrets, spice sources) and *Diplomacy* (compliments, gifts).

- A gold switch with a check mark means the rule is on; grey means off. All rules are on by default.
- Clicking a rule's name expands its explanation without changing it.
- Section headers collapse and expand with an animation, and a badge shows how many of the section's rules are on
  (for example *1/2 on*).
- The list scrolls with the mouse wheel. A thin gold scrollbar appears when the content does not fit; you can drag
  it or click the track to move by a page.

Changes apply immediately and are saved automatically to `%APPDATA%\CivChallenge\settings.ini`. Settings files from
older versions still work; rules they do not mention are treated as enabled.

## Languages

The banner, the settings tab and page, rule names and explanations, badges and the error message are translated
into every Spore locale: cs-cz, da-dk, de-de, el-gr, en-gb, en-us, es-es, fi-fi, fr-fr, hu-hu, it-it, ja-jp,
ko-kr, nl-nl, no-no, pl-pl, pt-br, pt-pt, ru-ru, sv-se, th-th, zh-cn and zh-tw. The language is taken from the
game's locale manager; an unknown language falls back to English. Terms follow each language's own game text
tables, and every string was measured with the game's own fonts to fit its place.

## Installation

1. Install the [Spore ModAPI Launcher Kit](https://davoonline.com/sporemodder/rob55rod/ModAPI/Public/index.html)
   if you do not have it yet.
2. Open `CivChallenge.sporemod` with **Spore ModAPI Easy Installer**.
3. Start the game with **Spore ModAPI Launcher**. If the game was already running, restart it completely.

To remove the mod, use **Spore ModAPI Easy Uninstaller → Civ Challenge**.

The mod adds no new object types to your saves. Its only addition is the small record with the gift counters in
the ModAPI subsystem block; without the mod ModAPI skips that record by its size, and the save loads as usual.

## Building from source (Linux)

> **A note from the author:** I build and test this mod only on Linux. I personally have no idea how to build it on
> Windows, so there are no Windows build instructions here and I cannot help with a Windows build.
> To play the mod, you don't need to build anything: just use the `.sporemod` from Releases.

The mod is a 32-bit Windows DLL cross-compiled with clang-cl. Tested with:

- LLVM 21.1.8: `clang-cl`, `lld-link`, `llvm-lib`, `llvm-mt` and `llvm-rc` in `PATH` (on Debian and Ubuntu the last
  three come with the `llvm` package)
- CMake 4.3.4 (3.20 or newer is required; before 3.27 the manifests stay next to the binaries as `.manifest` files)
  and `make`
- GNU `bash` 4.4 or newer, coreutils, findutils and grep (with `-P`), plus `git`, `curl`, `unzip`, `tar` and `jq` 1.8.2
- clang-format 21, only for `cmake/format.sh`

Like every file of the project, each script starts with an empty line (see
[Formatting and the layout guard](#formatting-and-the-layout-guard)), so the scripts have no shebang line and are run
with `bash`.

### Build

```sh
git clone https://github.com/codestlover/Spore-CivChallenge.git && cd Spore-CivChallenge
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clangcl-i686.cmake \
    -DCIV_ACCEPT_MSVC_LICENSE=ON
cmake --build build
```

This produces `build/bin/CivChallenge.dll` and the legacy `build/bin/Installer.exe`.
The build is reproducible: with CMake 3.27 or newer and the pinned dependencies it gives byte-identical binaries in
any folder, so a build can be compared with the `CivChallenge.dll` inside the release `.sporemod`.

### Dependencies

The first `cmake` run downloads everything the build needs into `deps/`, which git ignores, by running
`cmake/setup.sh`:

- `deps/Spore-ModAPI`: the [Spore ModAPI SDK](https://github.com/Spore-Community/Spore-ModAPI) at tag `v2.5.559`,
  checked by commit hash.
- `deps/sdk`: its headers with a few small fixes so that clang-cl accepts them.
- `deps/Detours`: [Microsoft Detours](https://github.com/microsoft/Detours) at tag `v4.0.1`, checked by commit hash.
- `deps/modapi`: `SporeModAPI.lib` from the `v2.5.559` release, checked by SHA-256.
- `deps/msvc-sdk`: the MSVC CRT 14.44 and Windows SDK 10.0.26100 for x86, downloaded with
  [xwin](https://github.com/Jake-Shadle/xwin) 0.10.0. If `xwin` 0.10.0 is not in `PATH`, the script downloads that
  release into `deps/.xwin` and checks it by SHA-256 (x86_64 and aarch64 Linux).

The MSVC CRT and Windows SDK come under the Microsoft license and are not redistributed.
`-DCIV_ACCEPT_MSVC_LICENSE=ON` means you accept that license; without it the first `cmake` run stops and explains
the choice. If you already have an xwin "splat" tree, pass `-DCIV_MSVC_SDK=/path/to/tree` instead (keep it outside
the repository). To keep the dependencies outside the repository, pass `-DCIV_DEPS=/some/dir` or set `CIV_DEPS` in
the environment before the first `cmake` run: an absolute path to a folder used only for these dependencies. To
change `CIV_DEPS` or `CIV_MSVC_SDK` later, configure a new build folder. `deps/` needs about 1 GB; xwin's download
cache is removed once the SDK is unpacked.

Later runs reuse `deps/`. `setup.sh` records its own checksum in `deps/.setup-stamp`, so when a new version of the
script arrives (for example after `git pull`), the next `cmake` run prepares the dependencies again and downloads
whatever the new version pins differently. The script can also be run by hand, for example to prepare the
dependencies before configuring: `bash cmake/setup.sh --accept-msvc-license`. It also accepts `--xwin PATH` (a
specific xwin binary), `--xwin-cache PATH` (a download cache to keep) and `--msvc-sdk PATH` (an existing splat tree,
linked as `deps/msvc-sdk`), and reads `CIV_DEPS` from the environment.

### Translations

The string table is generated at build time: `cmake/strings.sh` reads `locale/strings.json` with `jq` and writes
`build/generated/Strings.hpp`.

`locale/strings.json` is a JSON object that maps each locale code to its strings, in table order:

- The first locale, `en-us`, defines the order of the text ids. Every locale must contain every id.
- `#` in a string is replaced by a number at runtime. It must appear in a translation exactly when it appears in the
  English text.
- `widths` holds the pixel widths of the three section captions and of the widest badge text, measured with the
  game's own fonts. They place the section dividers and size the badges.

The generator refuses incomplete or malformed files.

### Testing and releases

Releases are checked with an emulator test suite that runs the compiled DLL's x86 code (89 scenarios, including 15
that execute the game's own code around the hooks) and with Wine smoke tests that install and remove all 19 hooks,
chain a hook with another mod's in both load orders and exercise the settings file. These tools and the packaging scripts are not part of this repository, so the
`.sporemod` is only published on the [Releases](../../releases) page. These checks are not a replacement for playing:
the diplomacy rules, the scrolling settings page and the translations are verified in the emulator but have not yet
been played through in-game.

## Project layout

| Path | Contents |
| --- | --- |
| `src/core` | Rules, hooks, the banner, gift counters in saves, allocator, SDK compatibility, the hook table |
| `src/settings` | Settings file, the settings page and native drawable wrappers |
| `src/localization` | Language detection; the string table comes from `locale/strings.json` |
| `src/installer` | The legacy `Installer.exe` |
| `cmake` | The clang-cl cross toolchain and the scripts `setup.sh`, `strings.sh`, `layout.sh`, `format.sh` |
| `locale` | `strings.json`, the translations |

The hook table in `src/core/Hooks.hpp` (addresses and expected original bytes) was produced with private
reverse-engineering tools that work on the game's own code. That code cannot be redistributed, so those tools
are not part of this repository.

### Formatting and the layout guard

```sh
bash cmake/format.sh
```

This formats the C++ sources with clang-format 21 and fixes the framing of the files. To use a specific clang-format
binary, set `CLANG_FORMAT=/path/to/clang-format`.

The build also enforces the project layout with `cmake/layout.sh`. It checks every file of the project tree except
the build and dependency folders and editor and tool caches:

- Every text file starts with exactly one empty line and ends with exactly two.
- C++ headers use `.hpp`.
- `src/` keeps its files in subdirectories.

CMake runs this check at configure time and before every build. Any violation stops the build with a list of files.
`bash cmake/layout.sh --fix` fixes the framing. The first rule is also why the scripts have no shebang: `#!` only
works on the very first line of a file.

## Credits and license

Civ Challenge is licensed under **GPL-3.0-or-later** (see [`LICENSE`](LICENSE)).

- [Spore ModAPI SDK](https://github.com/Spore-Community/Spore-ModAPI) by Eric Mor and contributors, GPL-3.0-or-later.
  `src/core/Allocator.cpp` is adapted from it.
- [Microsoft Detours](https://github.com/microsoft/Detours), MIT, fetched by `cmake/setup.sh`.
- The settings UI integration comes from the Spore Multiplayer Mod, MIT. The release archive, not this repository,
  also contains the switch and row textures from that mod.

Full notices are in [`NOTICE.md`](NOTICE.md) and [`licenses/`](licenses). Spore and Galactic Adventures belong to
Electronic Arts/Maxis. This repository and the release contain no game executables, game assets, save data or
decompiled game code.

