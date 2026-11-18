# Interface languages on the Sticky (microSD language packs)

The Sticky firmware keeps **English only** in flash
(`custom_i18n_builtin_langs = en` in `[sticky_common]`). Vietnamese, and any
other interface language, is an upstream CrossNext 2.0 language pack:
`/languages/<bcp47>.cplang` on the microSD, e.g. `/languages/vi.cplang`.
Built-in Vietnamese cost about 190 KB of a firmware image that was at 98.3% of
its 8 MB slot; the pack costs that much PSRAM instead, once, when it is used.

## How the firmware uses a pack

- `lib/I18n/LanguagePack.*` is upstream 2.0's loader, unchanged: it validates
  the header, every entry, printf signatures against English and a CRC, then
  keeps the text in one PSRAM arena until reset. A translation the pack lacks
  falls back to English per string.
- `src/apps_local/i18n/UiLanguage.*` is the fork's policy:
  - at boot, `SETTINGS.languagePack` names the pack. A saved non-built-in
    language with no pack named (settings written by a firmware that had
    Vietnamese built in) adopts `<bcp47>.cplang`, so updated devices keep
    Vietnamese without visiting Settings;
  - a missing or damaged pack runs that boot in English and keeps the saved
    choice: installing the pack brings the language back;
  - switching applies in place, as the fork always did. Only replacing an
    already loaded pack by a different one asks for a restart.
- Settings → System → Language lists the built-in languages, then the packs
  found in `/languages`, with their state (built in / microSD / in use /
  unavailable).

## Getting the pack onto the card

The App Store lists **Tiếng Việt** (`lang-vi`, `/languages/vi.cplang`). It is
published with the native modules of each firmware build, under
`appstore:modules/<abi>/languages/vi.cplang`, because a pack's keys follow the
firmware's string table. It counts as an outdated item, and so joins the
first-Home "update apps" prompt after every firmware update, whenever the saved
language uses it and `/.crosspoint/vi.cplang.abi` does not name the running
build. After the download the language switches on if it was waiting for it.

Offline, copy a `vi.cplang` built for the same firmware into `/languages/` and
restart.

## Building packs

```sh
python tools_local/sticky_language_packs.py            # -> ../output/modules/languages/vi.cplang
python tools_local/sticky_language_packs.py OUT --langs vi,es
```

`tools_local/native_modules/release.py` runs it before the firmware build, so
`--publish` pushes the pack with the modules. Edit wording in
`lib/I18n/translations/vietnamese.yaml`; never edit a `.cplang` by hand.
Upstream's `tools_local/language_packs.py` (CSV template, `build`, `bundle`) is
kept verbatim for community translations.
