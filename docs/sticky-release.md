# Releasing CrossNext for Sticky

How to cut a `loitq07/CrossNext-for-Sticky` release that devices can install
over the air. Every step exists because skipping it already broke a release:
v1.4.12 first shipped (2026-11-17) with a What's New screen that stranded the
boot splash and a firmware whose native modules 404'd, and had to be pulled.

Paths below are relative to the workspace folder that holds this checkout
(`CrossNext-build/`), the `appstore` branch checkout (`appstore-branch/`) and
the release scripts (`build-sticky.ps1`, `package_clean_release.py`,
`publish_release.py`).

## The rule that shapes everything: one build, released as tested

Every firmware build stamps a **fresh random native-module ABI**
(`scripts_local/native_modules_fw.py` writes
`src/apps_local/modules/NativeModuleAbi.generated.h`). A native module (2048,
Sudoku, Pet, ...) only loads on the exact build it was linked against, and the
App Store fetches it from `appstore:modules/<abi>/`.

So the firmware, its modules and the release assets must all come from **one**
build:

- Build only through `tools_local/native_modules/release.py --publish`. A bare
  `build-sticky.ps1` / `pio run` makes a new ABI with no modules published --
  after updating, every module fails with HTTP 404.
- After that build: **do not rebuild.** Test, package and publish that binary.
  Any later build (even with no code change) is a different ABI. If you must
  change code, go back to step 3.
- Nothing else may rebuild and re-upload the assets. `release-sticky.yml` is
  manual-only for this reason; it used to fire on tag pushes and overwrite the
  tested assets with a CI build whose ABI had no modules.

## Steps

1. **Version.** Bump `[crossplay] version` in `platformio.ini`. OTA offers an
   update only when the release tag (minus `v`) is newer than the running
   `CROSSPOINT_VERSION`.
2. **Notes.** Write `docs/releases/CrossNext-Sticky-v<version>.md` (Vietnamese
   + English). It becomes the release page. Keep the OTA note: the screen sits
   at 0% for about a minute while `esp_ota_begin()` erases the 8 MB slot and
   buttons don't respond -- users think it froze. Update the What's New rows in
   `src/activities/settings/WhatsNewActivity.cpp` too.
3. **Build + publish modules** (from `CrossNext-build/`):

   ```sh
   python tools_local/native_modules/release.py --publish ..\appstore-branch
   ```

   It compiles the modules, refreshes catalog sizes in
   `AppStoreCatalog.h`, builds `gh_release_sticky`, links every module against
   that build and pushes `modules/<abi>/` to the `appstore` branch. Pushing
   modules is safe before the release: no shipped firmware has that ABI yet.
   Check `https://raw.githubusercontent.com/loitq07/CrossNext-for-Sticky/appstore/modules/<abi>/games/2048.elf`
   answers 200.
4. **Test that exact binary on a device** (`.pio/build/gh_release_sticky/firmware.bin`,
   flashed to `0x10000` with `otadata-blank.bin` at `0xe000`), serial
   attached. Back up the device's app slot first if you need it back. At least:
   - cold boot reaches Home;
   - first boot after a version change shows What's New over Home, and
     dismissing it returns to Home (not the boot splash). It shows once per
     version -- to see it again, clear `lastSeenVersion` in
     `/.crosspoint/state.json` on the card;
   - Home's module update downloads every module with no 404, and a game and
     an app open.
5. **Commit and push `master`** -- include the `AppStoreCatalog.h` sizes from
   step 3. Releases are cut from `master`; `main` is not the release branch.
6. **Tag the release commit and push the tag:**

   ```sh
   git tag -a v<version> -m "CrossNext for Seeed reTerminal Sticky v<version>"
   git push github v<version>
   ```

   Never let `gh release create` make the tag: on a missing tag it creates one
   on the default branch (`main`).
7. **Package** (workspace root): `python package_clean_release.py` -- copies the
   current `.pio` build, no rebuild. Confirm `output/release/SHA256SUMS.txt`
   shows the sha256 of the binary you tested. It also produces
   `CrossNext-Sticky-Resources.zip` (microSD `apps/` artwork for PET, Reading
   Quest and The Intercept, plus the reader fonts) with
   `tools_local/sticky_sd_resources.py`. That pack does not depend on the
   firmware build: it is cut from upstream CrossNext's resources, pinned by
   version and sha256 in the script. When you move the pin to a newer upstream,
   check the PET pack image counts still match `petui::*_COUNT`. The loader
   refuses a mismatched pack.
8. **Publish:** `python publish_release.py`. Its preflight refuses to publish
   unless the OTA asset carries the module ABI of the last build, every
   catalog module (`module = true`) is on the `appstore` branch for that ABI,
   and the tag is pushed and points at `CrossNext-build` HEAD.
9. **Verify the live release** exactly as a device sees it:
   `gh api repos/loitq07/CrossNext-for-Sticky/releases/latest` names the new
   tag, and the downloaded `firmware-sticky.bin` has the tested sha256.
10. **OTA from the previous public release** on a device: flash the previous
    release's `firmware-sticky.bin`, then Settings → update. This is the path
    every user takes, and it runs the *old* firmware's updater code.

## Beta first: OTA your own device before everyone else

Settings → System → **Beta updates** (`betaUpdates`, off by default) makes
Check for updates read `releases?per_page=3` instead of `releases/latest`, and
offer the newest release **including prereleases** that carries
`firmware-sticky.bin`. `releases/latest` never returns a prerelease, so devices
without the toggle are not offered one. Drafts are invisible to a device
(they need an authenticated token), so a draft can never be OTA'd.

To try a build on your own device first, run steps 1–7 as usual, then:

8. **Publish as a prerelease:** `python publish_release.py --prerelease`. Same
   preflight. Turn **Beta updates** on, then Settings → Check for updates. The
   build already has its modules on the `appstore` branch, so games and apps work.
9. **When it is good, promote it:** `python publish_release.py --tag v<version> --promote`.
   This only flips the flag (`gh release edit --prerelease=false --latest`): no rebuild,
   no re-upload, same binary and ABI. The script confirms `releases/latest`
   now names the tag; then do step 10.

A bad prerelease: delete it and its tag as below (only Beta devices could have
installed it), fix, and cut the next version as a new prerelease. Version
numbers are not reused.

Caveats: the repo is public, so a prerelease is visible on the Releases page.
Turning Beta off never downgrades; the device stays on its build until a newer
public one exists. Beta needs a device already running a firmware that has the
toggle (the first release after v1.4.12 onward).

## If a release is bad

Delete the release and its tag (`gh release delete v<x> --yes`,
`git push github :refs/tags/v<x>`): `releases/latest` falls back to the previous
release, so devices stop being offered it. Devices that already installed it
keep it.

A device that looks frozen during or after an update is usually alive. Read its
flash over USB before reflashing anything -- the otadata seq and the version
strings in both app slots (`0x10000`, `0x800000`) show what actually ran, and
the `coredump` partition (`0xff0000`) may hold a crash.
