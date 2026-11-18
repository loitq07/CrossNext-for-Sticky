#pragma once

namespace release_sources {
constexpr const char* const kUrls[] = {
    "https://api.github.com/repos/loitq07/CrossNext-for-Sticky/releases/latest",
};
constexpr int kCount = static_cast<int>(sizeof(kUrls) / sizeof(kUrls[0]));

// Beta channel (Settings -> "Beta updates"): the newest few releases, PRERELEASES
// INCLUDED. `releases/latest` above never returns a prerelease, so publishing a
// build as a prerelease reaches only devices that opted in; promoting that same
// release (gh release edit --prerelease=false --latest) then ships it to everyone.
// Drafts are invisible to an unauthenticated device, so they never appear here.
// Parsed with ReleaseJsonParser::setListMode(true).
constexpr const char* kBetaListUrl =
    "https://api.github.com/repos/loitq07/CrossNext-for-Sticky/releases?per_page=3";
}  // namespace release_sources
