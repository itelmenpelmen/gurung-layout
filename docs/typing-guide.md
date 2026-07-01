# Gurung Scientific IME Typing Guide

## Modes

- `Ctrl+Space` toggles between Gurung typing and English pass-through.
- `Ctrl+Shift+Space` toggles between Gurung Devanagari and Gurung Latin transcription.

## Dead Keys

Press the dead key first, then the base character.

| Gesture | Latin mode | Devanagari mode |
| :-- | :-- | :-- |
| `Ctrl+Shift+N`, `a` | `ã` | `आँ` |
| `Ctrl+Shift+N`, `i` | `ĩ` | `इँ` |
| `Ctrl+Shift+N`, `u` | `ũ` | `उँ` |
| `Ctrl+Shift+D`, `a` | `ạ` | `अ` |
| `Ctrl+Shift+D`, `t` | `ṭ` | `ट` |
| `Ctrl+Shift+D`, `d` | `ḍ` | `ड` |

## Devanagari Consonants

| Latin input | Output | Notes |
| :-- | :-- | :-- |
| `k` | `क` | inherent `ạ` |
| `kh` or `K` | `ख` | aspirated |
| `g` | `ग` |  |
| `gh` or `G` | `घ` |  |
| `ng` | `ङ` |  |
| `c` or `ts` | `च` | Gurung `ts` |
| `C`, `ch`, or `tsh` | `छ` | Gurung `tsh` |
| `j` or `dz` | `ज` | Gurung `dz` |
| `J`, `jh`, or `dzh` | `झ` | Gurung `dzh` |
| `q` | `ट` | `ṭ` |
| `Q` | `ठ` | `ṭh` |
| `x` | `ड` | `ḍ` |
| `X` | `ढ` | `ḍh` |
| `t` | `त` |  |
| `T` or `th` | `थ` |  |
| `d` | `द` |  |
| `D` or `dh` | `ध` |  |
| `p` | `प` |  |
| `P`, `ph`, or `f` | `फ` |  |
| `b` | `ब` |  |
| `B` or `bh` | `भ` |  |
| `m n y r l s h` | `म न य र ल स ह` |  |
| `v` or `w` | `व` | Gurung `w` |

## Vowels

Consonants have inherent Gurung `ạ` unless a vowel sign follows.

| Input after consonant | Matra | Example |
| :-- | :-- | :-- |
| `a` | `ा` | `ka` -> `का` |
| `i` | `ि` | `ki` -> `कि` |
| `u` | `ु` | `ku` -> `कु` |
| `e` | `े` | `ke` -> `के` |
| `ai` or `E` | `ै` | `kai` -> `कै` |
| `o` | `ो` | `ko` -> `को` |
| `ao`, `au`, or `W` | `ौ` | `kau` -> `कौ` |

Use `~` in typed test strings, or `Ctrl+Shift+N` interactively, for nasal vowels.

Inside a word, press `Ctrl+Shift+N` immediately before the vowel that should be nasalized:

| Input | Output |
| :-- | :-- |
| `m l Ctrl+Shift+N o g u` | `म्लोँगु` |
| `k Ctrl+Shift+N o` | `कोँ` |

## Clusters And Half Forms

- Use `/` after a consonant to force a half form: `k/ri` -> `क्रि`.
- Common Gurung cluster seconds are recognized directly: `kri` -> `क्रि`, `sya` -> `स्या`, `nwara` -> `न्वार`.
- Ambiguous breathy input exposes candidates. For `khi`, the engine includes both `खि` and `क्हि`.

## Sample Words

| Input | Output | Meaning |
| :-- | :-- | :-- |
| `kri` | `क्रि` | one |
| `tamu` | `तमु` | Tamu/Gurung |
| `phodzo` | `फोजो` | Bhujung |
| `tsa` | `च` | that |
| `tsah` | `च्ह` | son |
| `mih` | `म्हि` | person |
| `kli` | `क्लि` | stool |
| `sya` | `स्या` | meat |
| `nwara` | `न्वार` | cat |
| `neme` | `नेमे` | bird |
