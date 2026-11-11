#include "UserProfile.h"

#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <PersistableStore.h>

#include <cstring>

namespace {

// Device-wide, so it stays in /.crosspoint whichever profile is active.
// {"active": 1, "names": ["", ""]}; "active" counts from 1.
constexpr char PROFILES_FILE[] = "/.crosspoint/profiles.json";

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

constexpr StrId DEFAULT_NAMES[UserProfile::COUNT] = {StrId::STR_PROFILE_1, StrId::STR_PROFILE_2};

uint8_t activeIndex = 0;
// The profile the next boot starts in. Differs from activeIndex between a switch
// and its restart, or when the saved profile could not be opened at boot.
uint8_t savedIndex = 0;
std::string names[UserProfile::COUNT];

bool save() {
  JsonDocument doc;
  doc["active"] = savedIndex + 1;
  JsonArray list = doc["names"].to<JsonArray>();
  for (const auto& name : names) list.add(name);
  return PersistableStoreBase::writeDocToFile(PROFILES_FILE, doc);
}

}  // namespace

void UserProfile::load() {
  activeIndex = 0;
  JsonDocument doc;
  if (PersistableStoreBase::readDocFromFile(PROFILES_FILE, doc)) {
    const int index = (doc["active"] | 1) - 1;
    if (index >= 0 && index < COUNT) activeIndex = static_cast<uint8_t>(index);
    JsonArrayConst list = doc["names"].as<JsonArrayConst>();
    for (uint8_t i = 0; i < COUNT && i < list.size(); i++) {
      const char* stored = list[i] | "";
      if (strlen(stored) <= MAX_NAME_LENGTH) names[i] = stored;
    }
  }
  savedIndex = activeIndex;
  if (activeIndex != 0 && !Storage.ensureDirectoryExists(PATHS[activeIndex].dataDir)) {
    LOG_ERR("PROFILE", "Cannot create %s, using profile 1", PATHS[activeIndex].dataDir);
    activeIndex = 0;
  }
  LOG_INF("PROFILE", "Active profile %u (%s)", static_cast<unsigned>(activeIndex) + 1, libraryRoot());
}

uint8_t UserProfile::active() { return activeIndex; }

bool UserProfile::setActive(const uint8_t index) {
  if (index >= COUNT) return false;
  const uint8_t previous = savedIndex;
  savedIndex = index;
  if (save()) return true;
  savedIndex = previous;
  return false;
}

std::string UserProfile::name(const uint8_t index) {
  if (index >= COUNT) return {};
  return names[index].empty() ? I18N.get(DEFAULT_NAMES[index]) : names[index];
}

bool UserProfile::setName(const uint8_t index, const std::string& name) {
  if (index >= COUNT) return false;
  const size_t first = name.find_first_not_of(' ');
  std::string trimmed = first == std::string::npos ? "" : name.substr(first, name.find_last_not_of(' ') - first + 1);
  if (trimmed.size() > MAX_NAME_LENGTH || trimmed == I18N.get(DEFAULT_NAMES[index])) trimmed.clear();
  if (trimmed == names[index]) return true;
  std::string previous = std::move(names[index]);
  names[index] = std::move(trimmed);
  if (save()) return true;
  names[index] = std::move(previous);
  return false;
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
