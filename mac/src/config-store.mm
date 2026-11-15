#include "config-store.h"

#import <Foundation/Foundation.h>

#include <fstream>
#include <iterator>
#include <utility>

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

bool ConfigStore::update(goxvi::EngineConfig config) {
  config_ = std::move(config);
  ++revision_;
  return write(config_);
}

bool ConfigStore::save(goxvi::EngineConfig config) {
  if (!write(config)) return false;
  config_ = std::move(config);
  ++revision_;
  return true;
}

bool ConfigStore::write(const goxvi::EngineConfig& config) const {
  const std::string json = goxvi::serializeConfigJson(config);
  NSFileManager* fm = [NSFileManager defaultManager];
  const std::string fsPath = path();
  NSString* file = [fm stringWithFileSystemRepresentation:fsPath.c_str() length:fsPath.size()];
  NSError* error = nil;
  if (![fm createDirectoryAtPath:file.stringByDeletingLastPathComponent
                                 withIntermediateDirectories:YES
                                                  attributes:nil
                                                       error:&error]) {
    NSLog(@"Goxvi: cannot create config folder: %@", error.localizedDescription);
    return false;
  }
  NSData* data = [NSData dataWithBytes:json.data() length:json.size()];
  // atomically:YES = write a temp file then rename (same rule as Windows).
  if (![data writeToFile:file options:NSDataWritingAtomic error:&error]) {
    NSLog(@"Goxvi: cannot write config.json: %@", error.localizedDescription);
    return false;
  }
  return true;
}
