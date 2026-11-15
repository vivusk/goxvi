#include "config-store.h"

#import <Foundation/Foundation.h>

#include <fstream>
#include <iterator>

#include "goxvi/config-json-serializer.h"

ConfigStore& ConfigStore::instance() {
  static ConfigStore store;
  return store;
}

std::string ConfigStore::path() const {
  NSArray<NSString*>* dirs = NSSearchPathForDirectoriesInDomains(
      NSApplicationSupportDirectory, NSUserDomainMask, YES);
  NSString* base = dirs.firstObject ?: [NSHomeDirectory()
      stringByAppendingPathComponent:@"Library/Application Support"];
  NSString* file = [base stringByAppendingPathComponent:@"Goxvi/config.json"];
  return file.fileSystemRepresentation;
}

void ConfigStore::load() {
  config_ = goxvi::EngineConfig{};
  std::ifstream in(path(), std::ios::binary | std::ios::ate);
  if (!in) return;  // first run: no file yet
  const std::streamoff size = in.tellg();
  // Same cap as the Windows TIP/settings app (engine-types.h).
  if (size <= 0 || size > goxvi::kMaxConfigFileBytes) return;
  in.seekg(0);
  std::string json(static_cast<size_t>(size), '\0');
  if (!in.read(json.data(), size)) return;
  config_ = goxvi::parseConfigJson(json);  // tolerant: never throws, per-field defaults
}
