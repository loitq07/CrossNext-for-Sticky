#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Plugin folders may live under any of these SD roots; earlier roots win on
// name collisions. /.crosspoint/plugins is the historical home; /plugins and
// /.plugins are friendlier for users copying folders onto the card from a
// computer.
namespace PluginLocations {
inline constexpr const char* ROOTS[] = {"/.crosspoint/plugins", "/plugins", "/.plugins"};
inline constexpr size_t ROOT_COUNT = sizeof(ROOTS) / sizeof(ROOTS[0]);

// What this firmware tells plugins and catalogs it supports (reported by
// /api/status and matched against a catalog item's `compatibility`). Only list
// a capability once the generic catalog code really implements it.
inline constexpr const char* FIRMWARE_FAMILY = "crosspoint";
inline constexpr int PLUGIN_API = 1;
inline constexpr const char* PLUGIN_CAPABILITIES = "catalog.lists,catalog.notices,catalog.bundle_install,catalog.version_tracking";

// Plugin Hub (https://github.com/crosspoint-reader/crosspoint-plugin-hub) is an
// ordinary SD plugin. When it is absent the plugin picker offers a built-in
// bootstrap catalog (see PluginCatalogActivity) that installs it.
inline constexpr const char* HUB_NAME = "pluginhub";
inline constexpr const char* HUB_BOOTSTRAP_URL =
    "https://raw.githubusercontent.com/crosspoint-reader/crosspoint-plugin-hub/main/bootstrap-catalog.json";
// A bundle install drops this file in the plugin folder just before swapping
// files in, and removes it once every file landed. A folder still carrying it
// is a half-installed plugin and is not discovered.
inline constexpr const char* INSTALLING_MARKER = ".installing";

enum class DeviceKind { None, Catalog, Background };
enum class PickerAction { None, Catalog, Readme };

constexpr DeviceKind classifyDeviceManifest(const bool hasBrowseUrl, const bool hasEvents) {
  if (hasBrowseUrl) return DeviceKind::Catalog;
  if (hasEvents) return DeviceKind::Background;
  return DeviceKind::None;
}

// A catalog opens; any other plugin with a README shows it (web-only plugins
// included, so their setup notes are readable on the device).
constexpr PickerAction pickerAction(const DeviceKind kind, const bool hasReadme) {
  if (kind == DeviceKind::Catalog) return PickerAction::Catalog;
  return hasReadme ? PickerAction::Readme : PickerAction::None;
}

// Exactly MAJOR.MINOR.PATCH: three digit-only components, nothing else (no
// sign, spaces, suffix or fourth part), each fitting in 32 bits.
inline bool parseVersion(const std::string& v, uint32_t (&out)[3]) {
  size_t pos = 0;
  for (int k = 0; k < 3; k++) {
    if (k > 0 && (pos >= v.size() || v[pos++] != '.')) return false;
    if (pos >= v.size() || v[pos] < '0' || v[pos] > '9') return false;
    uint64_t n = 0;
    for (; pos < v.size() && v[pos] >= '0' && v[pos] <= '9'; pos++) {
      n = n * 10 + static_cast<uint64_t>(v[pos] - '0');
      if (n > UINT32_MAX) return false;
    }
    out[k] = static_cast<uint32_t>(n);
  }
  return pos == v.size();
}

// Plugin versions are MAJOR.MINOR.PATCH (the catalog contract). True when
// `catalog` is newer than `installed`. An installed copy without a valid
// version is offered the update; a malformed catalog version never is.
inline bool isNewerVersion(const std::string& catalog, const std::string& installed) {
  uint32_t c[3], i[3];
  if (!parseVersion(catalog, c)) return false;
  if (!parseVersion(installed, i)) return true;
  for (int k = 0; k < 3; k++) {
    if (c[k] != i[k]) return c[k] > i[k];
  }
  return false;
}

// One SD plugin folder, classified by the marker files it carries.
struct Entry {
  std::string name;          // folder name
  std::string dir;           // "<root>/<name>"
  bool hasPluginJs = false;  // browser-side plugin (plugin.js)
  bool hasDevice = false;    // on-device catalog manifest (device.json)
  bool hasManifest = false;  // web UI card metadata (manifest.json)
};

// Scans every root. The earliest root containing a folder name claims it —
// matching findPluginDir, which serves that folder's files — and folders with
// none of the marker files are omitted. This is the single definition of
// "what is a plugin"; callers filter by the markers they need.
std::vector<Entry> scanPlugins(bool firstRunnableOnly = false);

// Directory of the named plugin ("<root>/<name>"), or "" when absent.
std::string findPluginDir(const char* name);
}  // namespace PluginLocations
