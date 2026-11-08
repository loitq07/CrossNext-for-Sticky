#include <HalStorage.h>
#include <cassert>
#include <cstdio>
#include "RecentBooksStore.h"

int main() {
  auto& store = RECENT_BOOKS;
  JsonDocument old;
  auto books = old["books"].to<JsonArray>();
  // More than a full recent list of old articles must not hide the books below.
  for (int n = 0; n < 40; ++n) {
    auto entry = books.add<JsonObject>();
    entry["path"] = "/books/Wallabag/article-" + std::to_string(n) + ".epub";
    entry["title"] = "Article";
  }
  auto novel = books.add<JsonObject>();
  novel["path"] = "/books/novel.epub";
  novel["title"] = "Novel";
  assert(store.fromJson(old.as<JsonVariantConst>()));
  assert(store.resaveRequested);
  assert(store.getCount() == 1 && store.getBooks()[0].title == "Novel");
  testStorage.files["/books/novel.epub"] = {1};
  testStorage.files["/books/new.epub"] = {1};
  testStorage.files["/BOOKS/WALLABAG/article.epub"] = {1};
  const int saves = store.saves;
  store.addBook("/BOOKS/WALLABAG/article.epub", "Article", "", "");
  assert(store.getCount() == 1 && store.saves == saves);
  assert(RecentBooksStore::isMissing({"/BOOKS/WALLABAG/article.epub", "", "", ""}));
  store.addBook("/books/new.epub", "New book", "", "");
  assert(store.getCount() == 2 && store.getBooks()[0].title == "New book");
  JsonDocument clean;
  store.toJson(clean);
  assert(clean["books"].size() == 2);
  for (JsonObjectConst book : clean["books"].as<JsonArrayConst>())
    assert(std::string(book["path"].as<const char*>()).find("Wallabag") == std::string::npos);
  std::puts("Recent-book migration and article exclusion checks passed");
}
