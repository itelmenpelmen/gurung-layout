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

## Current Implementation Notes

The transliteration core and TSF edit-session path are implemented. The core produces candidate lists for ambiguous spellings such as `khi`, where both `खि` and `क्हि` are plausible. The initial IME path supports selecting numbered candidates, but it does not yet draw a polished candidate popup window; that UI can be added on top of `last_candidates_` without changing the transliteration engine.

This shell does not currently expose `cl.exe` or `msbuild.exe`, so native compilation must be done from a Visual Studio C++ build shell.
