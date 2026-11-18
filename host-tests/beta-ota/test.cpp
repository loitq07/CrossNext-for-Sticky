// Beta updates: ReleaseJsonParser list mode over GitHub's `/releases` array.
//
// The beta channel reads the newest few releases (prereleases included) and
// must pick the FIRST one that carries this board's OTA asset. Releases are
// fed in arbitrary chunk sizes, as they arrive over TLS.
#include <ReleaseJsonParser.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

#include "../../src/network/FirmwareBoardTag.h"
#include "../../src/network/ReleaseSources.h"

namespace {
int checks = 0, failed = 0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) {
    ++failed;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}

std::string asset(const std::string& name, const std::string& url, int size) {
  return "{\"size\":" + std::to_string(size) + ",\"uploader\":{\"name\":\"x\",\"size\":1},\"browser_download_url\":\"" +
         url + "\",\"name\":\"" + name + "\"}";
}

// A release object shaped like GitHub's: nested author object carrying keys
// (name, size) that must not be mistaken for the release's own, a reactions
// object, and assets either before or after tag_name.
std::string release(const std::string& tag, bool prerelease, const std::string& assets, bool assetsFirst) {
  const std::string head = "{\"author\":{\"login\":\"me\",\"tag_name\":\"bogus\",\"assets\":[]},\"prerelease\":" +
                           std::string(prerelease ? "true" : "false") + ",\"body\":\"notes [with] {braces}\"";
  const std::string tagPart = ",\"tag_name\":\"" + tag + "\"";
  const std::string assetPart = ",\"assets\":[" + assets + "]";
  return head + (assetsFirst ? assetPart + tagPart : tagPart + assetPart) + ",\"reactions\":{\"total\":0}}";
}

struct Result {
  bool tag, firmware;
  std::string tagName, url;
  size_t size;
};

Result parse(const std::string& json, bool listMode, size_t chunk) {
  ReleaseJsonParser parser;
  parser.setFirmwareAssetName(CROSSPOINT_RELEASE_ASSET);
  parser.setListMode(listMode);
  parser.reset();  // list mode must survive reset()
  for (size_t at = 0; at < json.size(); at += chunk) parser.feed(json.data() + at, std::min(chunk, json.size() - at));
  return {parser.foundTag(), parser.foundFirmware(), parser.getTagName(), parser.getFirmwareUrl(),
          parser.getFirmwareSize()};
}

const std::string kAsset = CROSSPOINT_RELEASE_ASSET;
std::string url(const char* tag) { return std::string("https://example.invalid/") + tag + "/" + kAsset; }
}  // namespace

int main() {
  check(std::strstr(release_sources::kBetaListUrl, "/releases?") != nullptr, "beta source is the release list");
  check(std::strstr(release_sources::kBetaListUrl, "/releases/latest") == nullptr, "beta source is not latest");
  check(std::strstr(release_sources::kUrls[0], "/releases/latest") != nullptr, "stable source stays latest");

  const std::string other = asset("CrossNext-Sticky-v9-factory.bin", "https://example.invalid/factory.bin", 3);

  for (size_t chunk : {size_t(1), size_t(3), size_t(7), size_t(64), size_t(100000)}) {
    for (bool assetsFirst : {false, true}) {
      // Newest prerelease has the asset: it wins over the older stable release.
      {
        const std::string json = "[" + release("v1.4.13", true, other + "," + asset(kAsset, url("v1.4.13"), 111),
                                               assetsFirst) +
                                 "," + release("v1.4.12", false, asset(kAsset, url("v1.4.12"), 222), assetsFirst) + "]";
        const Result r = parse(json, true, chunk);
        check(r.tag && r.firmware, "newest release with asset found");
        check(r.tagName == "v1.4.13", "newest (prerelease) tag chosen");
        check(r.url == url("v1.4.13"), "URL from the chosen release");
        check(r.size == 111, "size from the chosen release");
      }
      // Newest release lacks this board's asset: skipped, the next one wins,
      // and nothing of the skipped release leaks into the result.
      {
        const std::string json = "[" + release("v1.5.0", true, other, assetsFirst) + "," +
                                 release("v1.4.12", false, asset(kAsset, url("v1.4.12"), 222), assetsFirst) + "," +
                                 release("v1.4.11", false, asset(kAsset, url("v1.4.11"), 333), assetsFirst) + "]";
        const Result r = parse(json, true, chunk);
        check(r.tag && r.firmware, "falls through to a release with the asset");
        check(r.tagName == "v1.4.12", "first qualifying release, not a later one");
        check(r.url == url("v1.4.12") && r.size == 222, "asset belongs to that release");
      }
      // No release carries the asset: nothing found (OtaUpdater then asks latest).
      {
        const std::string json =
            "[" + release("v1.5.0", true, other, assetsFirst) + "," + release("v1.4.12", false, "", assetsFirst) + "]";
        const Result r = parse(json, true, chunk);
        check(!r.firmware, "no asset anywhere -> no firmware");
        check(!r.tag, "no asset anywhere -> no tag either");
      }
      // Single-release mode is unchanged.
      {
        const Result r = parse(release("v1.4.12", false, asset(kAsset, url("v1.4.12"), 222), assetsFirst), false, chunk);
        check(r.tag && r.firmware && r.tagName == "v1.4.12" && r.size == 222, "latest-object mode still works");
      }
    }
  }
  check(!parse("[]", true, 1).tag, "empty list");

  std::printf("beta-ota: %d checks, %d failed\n", checks, failed);
  return failed ? 1 : 0;
}
