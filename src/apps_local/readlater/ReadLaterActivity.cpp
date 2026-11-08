#include "ReadLaterActivity.h"

#include <ContentVisibility.h>
#include <FsHelpers.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Memory.h>

#include <algorithm>
#include <cstdio>

#include "activities/ActivityResult.h"
#include "activities/plugins/PluginCatalogActivity.h"
#include "apps_local/ui/PixelMenuTheme.h"
#include "components/UITheme.h"
#include "util/BookProgress.h"

namespace fui = freeink::ui;

ReadLaterActivity::ReadLaterActivity(GfxRenderer& renderer, MappedInputManager& input)
    : UiListActivity("ReadLater", renderer, input, false, true) {}

void ReadLaterActivity::onEnter() {
  UiListActivity::onEnter();
  reload();
}

const char* ReadLaterActivity::headerTitle() const { return tr(STR_READ_LATER); }

void ReadLaterActivity::scan(const std::string& path, int depth) {
  if (!Storage.exists(path.c_str())) return;
  auto directory = Storage.open(path.c_str());
  if (!directory || !directory.isDirectory()) {
    scanFailed = true;
    return;
  }
  bool failed = false;
  for (auto entry = directory.openNextFile(failed); entry; entry = directory.openNextFile(failed)) {
    char name[256]{};
    const auto length = entry.getName(name, sizeof(name));
    const bool isDirectory = entry.isDirectory();
    const auto size = entry.fileSize();
    const auto modified = entry.modificationTime();
    entry.close();
    if (!length || name[0] == '.') continue;
    const std::string child = path + "/" + name;
    if (isDirectory) {
      if (depth < 4) scan(child, depth + 1);
      else truncated = true;
    } else if (size && FsHelpers::hasEpubExtension(child)) {
      if (articles.size() == 512) {
        truncated = true;
        break;
      }
      Article article;
      article.path = child;
      article.title.assign(name, length - 5);
      article.modified = modified;
      const int progress = loadBookProgress(child);
      if (progress >= 0) {
        char text[32];
        std::snprintf(text, sizeof(text), "Wallabag · %d%%", std::clamp(progress, 0, 100));
        article.status = text;
      } else {
        article.status = tr(STR_READ_LATER_OFFLINE);
      }
      articles.push_back(std::move(article));
    }
    if ((articles.size() & 15) == 0) delay(1);
  }
  scanFailed |= failed;
}

void ReadLaterActivity::reload() {
  resetUi();
  rows.clear();
  articles.clear();
  scanFailed = truncated = false;
  scan(content::WALLABAG_DIRECTORY, 0);
  std::sort(articles.begin(), articles.end(), [](const Article& a, const Article& b) {
    return a.modified != b.modified ? a.modified > b.modified : a.title < b.title;
  });
  rows.reserve(articles.size() + 3);
  fui::ListItem wallabag;
  wallabag.label = tr(STR_READ_LATER_WALLABAG);
  wallabag.subtitle = tr(STR_READ_LATER_HELP);
  wallabag.actionValue = 0;
  rows.push_back(wallabag);
  fui::ListItem refresh;
  refresh.label = tr(STR_READ_LATER_REFRESH);
  refresh.actionValue = 1;
  rows.push_back(refresh);
  if (articles.empty() || scanFailed || truncated) {
    fui::ListItem notice;
    notice.label = scanFailed ? tr(STR_READ_LATER_SCAN_ERROR)
                             : truncated ? tr(STR_READ_LATER_LIMIT) : tr(STR_READ_LATER_EMPTY);
    notice.subtitle = tr(STR_READ_LATER_HELP);
    notice.actionValue = 2;
    rows.push_back(notice);
  }
  const int first = static_cast<int>(rows.size());
  for (const auto& article : articles) {
    fui::ListItem row;
    row.label = article.title.c_str();
    row.subtitle = article.status.c_str();
    row.actionValue = static_cast<int16_t>(rows.size());
    if (rows.size() == static_cast<size_t>(first)) row.sectionHeading = tr(STR_READ_LATER_OFFLINE);
    rows.push_back(row);
  }
  requestUpdate();
}

void ReadLaterActivity::activateIndex(int index) {
  app.clearTapFlash();
  if (index == 0) {
    auto catalog = makeUniqueNoThrow<PluginCatalogActivity>(renderer, mappedInput, false, false, "wallabag");
    if (catalog) {
      // A download can complete before Back returns here. Refresh the offline list then.
      startActivityForResult(std::move(catalog), [this](const ActivityResult&) { reload(); });
    }
  } else if (index == 1) {
    reload();
  } else {
    const int article = index - (listCount() - static_cast<int>(articles.size()));
    if (article >= 0 && article < static_cast<int>(articles.size()))
      activityManager.goToReader(articles[article].path);
  }
}

void ReadLaterActivity::onBackButton() { onGoHome(HomeMenuItem::READ_LATER); }

void ReadLaterActivity::buildScreen(UiScreen& screen) {
  pixelui::FontScope fonts(uiTarget, renderer, true);
  const int hints = mappedInput.hasTouch() ? 0 : UITheme::getInstance().getMetrics().buttonHintsHeight;
  pixelui::begin(screen, hints);
  fui::ListProps props;
  props.items = rows.data();
  props.count = static_cast<uint16_t>(rows.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.labelText = screen.theme().bodyText;
  props.labelText.maxLines = 2;
  props.subtitleText = screen.theme().smallText;
  props.subtitleText.maxLines = 2;
  syncListViewport(screen, props);
  pixelui::list(screen, props);
}
