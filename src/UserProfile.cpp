#include "UserProfile.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstring>

namespace {

// Device-wide, so it stays in /.crosspoint whichever profile is active.
constexpr char ACTIVE_PROFILE_FILE[] = "/.crosspoint/profile";

struct ProfilePaths {
  const char* root;
  const char* dataDir;
  const char* settings;
  const char* state;
  const char* recent;
};

constexpr ProfilePaths PATHS[UserProfile::COUNT] = {
    {"/", "/.crosspoint", "/.crosspoint/settings.json", "/.crosspoint/state.json", "/.crosspoint/recent.json"},
    {"/.profiles/2", "/.profiles/2/.crosspoint", "/.profiles/2/.crosspoint/settings.json",
     "/.profiles/2/.crosspoint/state.json", "/.profiles/2/.crosspoint/recent.json"},
};

uint8_t activeIndex = 0;

}  // namespace

void UserProfile::load() {
  char buf[4] = {};
  activeIndex = 0;
  if (Storage.exists(ACTIVE_PROFILE_FILE) && Storage.readFileToBuffer(ACTIVE_PROFILE_FILE, buf, sizeof(buf)) > 0) {
    const int index = buf[0] - '1';
    if (index >= 0 && index < COUNT) activeIndex = static_cast<uint8_t>(index);
  }
  if (activeIndex != 0 && !Storage.ensureDirectoryExists(PATHS[activeIndex].dataDir)) {
    LOG_ERR("PROFILE", "Cannot create %s, using profile 1", PATHS[activeIndex].dataDir);
    activeIndex = 0;
  }
  LOG_INF("PROFILE", "Active profile %u (%s)", static_cast<unsigned>(activeIndex) + 1, libraryRoot());
}

uint8_t UserProfile::active() { return activeIndex; }

bool UserProfile::setActive(const uint8_t index) {
  if (index >= COUNT) return false;
  Storage.mkdir("/.crosspoint");
  const char value[2] = {static_cast<char>('1' + index), '\0'};
  if (!Storage.writeFile(ACTIVE_PROFILE_FILE, value)) {
    LOG_ERR("PROFILE", "Failed to save active profile");
    return false;
  }
  return true;
}

const char* UserProfile::libraryRoot() { return PATHS[activeIndex].root; }
const char* UserProfile::dataDir() { return PATHS[activeIndex].dataDir; }
const char* UserProfile::settingsPath() { return PATHS[activeIndex].settings; }
const char* UserProfile::statePath() { return PATHS[activeIndex].state; }
const char* UserProfile::recentBooksPath() { return PATHS[activeIndex].recent; }

std::string UserProfile::toStoragePath(const char* libraryPath) {
  if (libraryPath[0] == '\0') libraryPath = "/";
  if (activeIndex == 0) return libraryPath;
  std::string path = PATHS[activeIndex].root;
  if (strcmp(libraryPath, "/") != 0) {
    if (libraryPath[0] != '/') path += '/';
    path += libraryPath;
  }
  return path;
}

const char* UserProfile::toLibraryPath(const char* storagePath) {
  if (activeIndex == 0) return storagePath;
  const char* root = PATHS[activeIndex].root;
  const size_t rootLen = strlen(root);
  if (strncmp(storagePath, root, rootLen) != 0) return storagePath;
  if (storagePath[rootLen] == '\0') return "/";
  return storagePath[rootLen] == '/' ? storagePath + rootLen : storagePath;
}

bool UserProfile::isLibraryRoot(const char* storagePath) { return strcmp(storagePath, libraryRoot()) == 0; }
