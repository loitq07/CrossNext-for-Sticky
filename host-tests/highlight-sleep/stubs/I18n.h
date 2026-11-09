#pragma once
enum { STR_SLEEP_HIGHLIGHTS, STR_SLEEP_HIGHLIGHTS_EMPTY };
inline const char* tr(int id) { return id == STR_SLEEP_HIGHLIGHTS ? "Highlights" : "No highlights"; }
