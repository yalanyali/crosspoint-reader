#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// Each profile has its own library root and its own settings, state and recent
// books. The first profile uses the card root and /.crosspoint, so cards from
// earlier firmware keep their data. Other profiles live under /.profiles, which
// every listing hides (FsHelpers::HIDDEN_ITEMS). Book caches, fonts, Wi-Fi,
// OPDS servers and plugins stay shared.
// Code that writes user content (books, clippings, screenshots) must map its
// path with toStoragePath(), or the other profile will see the file.
class UserProfile {
 public:
  static constexpr uint8_t COUNT = 2;
  // In bytes of UTF-8, so it fits the settings row and the switch popup.
  static constexpr size_t MAX_NAME_LENGTH = 24;

  // Reads the active profile. Call after Storage.begin() and before loading
  // any per-profile store.
  static void load();
  static uint8_t active();
  // Saves the profile to use from the next boot; the caller restarts so every
  // store reloads. Until then the running profile stays active.
  static bool setActive(uint8_t index);

  // The name the user gave the profile, or "Profile N" when it has none.
  static std::string name(uint8_t index);
  // Saves a new name; an empty one restores the default.
  static bool setName(uint8_t index, const std::string& name);

  // "/" for profile 0, otherwise the profile's folder ("/.profiles/2").
  static const char* libraryRoot();
  // Holds the profile's library index.
  static const char* dataDir();
  static const char* settingsPath();
  static const char* statePath();
  static const char* recentBooksPath();

  // Maps a path inside the library ("/" is the library root) to a card path.
  static std::string toStoragePath(const char* libraryPath);
  // Inverse of toStoragePath; points into `storagePath`. Paths outside the
  // library come back unchanged.
  static const char* toLibraryPath(const char* storagePath);
  static bool isLibraryRoot(const char* storagePath);
};
