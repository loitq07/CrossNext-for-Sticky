#pragma once

#include <string>

// Saved reading percentage, or -1 when unavailable. Never builds reading caches.
int loadBookProgress(const std::string& path);

// Returns true if the book is marked as read/finished (100% or readingstats complete).
bool isBookRead(const std::string& path);

// Sets the book read status:
// If markAsRead is true: saves 100% progress and records completion in readingstats.
// If markAsRead is false: resets progress and records reopening in readingstats.
bool setBookReadStatus(const std::string& path, const std::string& title, const std::string& author, bool markAsRead);
