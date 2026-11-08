#pragma once

#include <string>
#include <vector>

#include "activities/UiListActivity.h"

// Downloaded articles are available without starting Wi-Fi or a plugin session.
class ReadLaterActivity final : public UiListActivity {
 public:
  ReadLaterActivity(GfxRenderer& renderer, MappedInputManager& input);
  void onEnter() override;

 private:
  struct Article {
    std::string path, title, status;
    uint32_t modified = 0;
  };
  std::vector<Article> articles;
  std::vector<freeink::ui::ListItem> rows;
  bool scanFailed = false;
  bool truncated = false;
  void reload();
  void scan(const std::string& path, int depth);
  int listCount() const override { return static_cast<int>(rows.size()); }
  const char* headerTitle() const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  void onBackButton() override;
};
