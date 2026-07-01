# Gurung Scientific IME

Gurung Scientific IME is a Windows Text Services Framework (TSF) input method for fieldwork typing in Gurung. It is designed for two related workflows:

- Gurung Devanagari typing from a US QWERTY keyboard.
- Gurung scientific Latin transcription with nasal and underdot dead keys.

The existing Lekhnus release in this workspace was used as a behavioral reference only. This project has separate GUIDs, names, install paths, and scripts, so it does not overwrite Lekhnus.

## Project Layout

- `data/gurung_inventory.yaml` - reviewed linguistic inventory derived from the Markdown guide.
- `src/core/` - portable C++ transliteration engine.
- `src/ime/` - native Windows TSF COM text service.
- `src/helper/` - tray helper placeholder for status/menu support.
- `tests/` - C++ transliterator tests.
- `scripts/` - build, install, uninstall, and package scripts.
- `docs/typing-guide.md` - compact typing guide.

## Build

Install Visual Studio Build Tools with the **Desktop development with C++** workload, then open an **x64 Native Tools Command Prompt for VS** or equivalent Developer PowerShell.

```powershell
cd C:\Users\Sofya\gurung-layout
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Platform x64
```

Build outputs are written to `build\x64`.

## Install

Run from an elevated PowerShell after building:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\install.ps1 -Platform x64
```

Then add the keyboard under:

`Settings > Time & Language > Language & Region > Nepali > Language options > Keyboards`

Choose **Gurung Scientific IME**.

To uninstall:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1 -Platform x64
```

## Runtime Keys

- `Ctrl+Space`: toggle Gurung mode and English pass-through.
- `Ctrl+Shift+Space`: toggle Gurung Devanagari and Gurung Latin transcription modes.
- `Ctrl+Shift+N`: nasal dead key.
- `Ctrl+Shift+D`: underdot dead key.
- `Space`: commits the current Devanagari candidate.
- `1`-`9`: commits an alternate candidate when the current buffer has multiple candidates.

## Typing

Devanagari mode composes from plain QWERTY keys. A consonant by itself has inherent Gurung `ạ`: `k` -> `क`, `m` -> `म`. Type `a` only for long `आ`/`ा`: `ka` -> `का`. Type `H` or `a.` for explicit short `ạ`/`अ`.

### Devanagari Consonants

| Type | Output | Type | Output | Type | Output |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `k` | `क` | `kh`, `K` | `ख` | `g` | `ग` |
| `gh`, `G` | `घ` | `ng`, `<` | `ङ` | `ts`, `c` | `च` |
| `tsh`, `ch`, `C` | `छ` | `dz`, `j` | `ज` | `dzh`, `jh`, `J` | `झ` |
| `t.`, `q` | `ट` | `t.h`, `qh`, `Q` | `ठ` | `d.`, `x` | `ड` |
| `d.h`, `xh`, `X` | `ढ` | `t` | `त` | `th`, `T` | `थ` |
| `d` | `द` | `dh`, `D` | `ध` | `n` | `न` |
| `p` | `प` | `ph`, `P`, `f` | `फ` | `b` | `ब` |
| `bh`, `B` | `भ` | `m` | `म` | `y` | `य` |
| `r` | `र` | `l` | `ल` | `w`, `v` | `व` |
| `s` | `स` | `h` | `ह` |  |  |

### Devanagari Vowels

| Type | Independent | After consonant | Example |
| :-- | :-- | :-- | :-- |
| `H`, `a.` | `अ` | inherent/no matra | `kH` -> `क` |
| `a`, `A` | `आ` | `ा` | `ka` -> `का` |
| `i` | `इ` | `ि` | `ki` -> `कि` |
| `ii`, `I` | `ई` | `ी` | `kii` -> `की` |
| `u` | `उ` | `ु` | `ku` -> `कु` |
| `uu`, `U` | `ऊ` | `ू` | `kuu` -> `कू` |
| `e` | `ए` | `े` | `ke` -> `के` |
| `ai`, `E` | `ऐ` | `ै` | `kai` -> `कै` |
| `o`, `O` | `ओ` | `ो` | `ko` -> `को` |
| `ao`, `au`, `W` | `औ` | `ौ` | `kau` -> `कौ` |

### Nasal, Underdot, Halant

- Nasal vowels: press `Ctrl+Shift+N`, then the vowel. Examples: `k Ctrl+Shift+N o` -> `कोँ`; `m l Ctrl+Shift+N o g u` -> `म्लोँगु`. In tests or raw buffers, `~` and `M` are also nasal markers: `ko~`, `mlo~gu`.
- Latin nasal mode outputs: `Ctrl+Shift+N` plus `a e i o u` -> `ã ẽ ĩ õ ũ`.
- Underdot in Latin mode: `Ctrl+Shift+D` plus `a t d n s l r h` -> `ạ ṭ ḍ ṇ ṣ ḷ ṛ ḥ`.
- Retroflex in Devanagari mode: use `q/Q/x/X` or `t./t.h/d./d.h` for `ट ठ ड ढ`.
- Halant/half form: type `/` after a consonant. Examples: `t/` -> `त्`; `k/ri` -> `क्रि`.

### Clusters, Candidates, Examples

- Automatic clusters are recognized before `l r y w h dz`: `kri` -> `क्रि`, `sya` -> `स्या`, `nwara` -> `न्वार`, `mih` -> `म्हि`, `mleba` -> `म्लेब`.
- For other clusters, use `/`: `r/ts`, `t/s`, etc.
- Ambiguous breathy forms expose candidates. For `khi`, candidates include `खि` and `क्हि`; press `1`-`9` to choose an alternate or `Space` to accept the top candidate.
- Useful samples: `tamu` -> `तमु`; `phodzo` -> `फोजो`; `tsa` -> `च`; `tsah` -> `च्ह`; `mHl/khu` or `ma.l/khu` -> `मल्खु`; `mlogya` -> `म्लोग्या`.

Latin transcription mode lets normal ASCII keys pass through and uses the dead keys above for scientific letters. The standalone transliteration core also accepts ASCII spellings such as `a.`, `t.`, `d.`, `a~`, `o~` and normalizes them to Unicode Latin output.

## Current Implementation Notes

The transliteration core and TSF edit-session path are implemented. The core produces candidate lists for ambiguous spellings such as `khi`, where both `खि` and `क्हि` are plausible. The initial IME path supports selecting numbered candidates, but it does not yet draw a polished candidate popup window; that UI can be added on top of `last_candidates_` without changing the transliteration engine.

This shell does not currently expose `cl.exe` or `msbuild.exe`, so native compilation must be done from a Visual Studio C++ build shell.
