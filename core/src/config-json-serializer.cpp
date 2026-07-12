#include "goxvi/config-json-serializer.h"

#include <nlohmann/json.hpp>

#include "utf8-utf16-conversion.h"

namespace goxvi {

EngineConfig parseConfigJson(const std::string& utf8Json) {
  EngineConfig config;  // defaults: old style, telex
  const nlohmann::json doc =
      nlohmann::json::parse(utf8Json, nullptr, /*allow_exceptions=*/false);
  if (doc.is_discarded() || !doc.is_object()) return config;

  if (auto it = doc.find("toneStyle"); it != doc.end() && it->is_string()) {
    const std::string style = it->get<std::string>();
    if (style == "new") {
      config.toneStyle = ToneStyle::New;
    } else if (style == "old") {
      config.toneStyle = ToneStyle::Old;
    }
  }
  if (auto it = doc.find("inputMethod"); it != doc.end() && it->is_string()) {
    const std::string method = it->get<std::string>();
    if (method == "vni") {
      config.inputMethod = InputMethod::Vni;
    } else if (method == "telex") {
      config.inputMethod = InputMethod::Telex;
    }
  }
  if (auto it = doc.find("shortcutsEnabled");
      it != doc.end() && it->is_boolean()) {
    config.shortcutsEnabled = it->get<bool>();
  }
  if (auto it = doc.find("shortcuts"); it != doc.end() && it->is_array()) {
    for (const auto& entry : *it) {  // tolerant: skip a bad entry, not the array
      if (!entry.is_object()) continue;
      const auto trig = entry.find("trigger");
      const auto exp = entry.find("expansion");
      if (trig == entry.end() || !trig->is_string()) continue;
      if (exp == entry.end() || !exp->is_string()) continue;
      const std::string trigger = trig->get<std::string>();
      const std::string expansion = exp->get<std::string>();
      if (trigger.empty() || expansion.empty()) continue;
      ShortcutEntry shortcut{detail::utf8ToWide(trigger),
                             detail::utf8ToWide(expansion)};
      // Oversized entries are skipped like any other bad entry: a trigger
      // longer than the raw-key buffer can never match, and expansions are
      // capped so a hand-edited config can't balloon commit text.
      if (shortcut.trigger.size() > static_cast<size_t>(kMaxRawKeys)) continue;
      if (shortcut.expansion.size() >
          static_cast<size_t>(kMaxShortcutExpansionChars)) {
        continue;
      }
      config.shortcuts.push_back(std::move(shortcut));
    }
  }
  return config;  // version/unknown fields intentionally ignored
}

std::string serializeConfigJson(const EngineConfig& config) {
  nlohmann::json shortcuts = nlohmann::json::array();
  for (const auto& s : config.shortcuts) {  // vector order preserved (last wins)
    shortcuts.push_back({{"trigger", detail::wideToUtf8(s.trigger)},
                         {"expansion", detail::wideToUtf8(s.expansion)}});
  }
  const nlohmann::json doc = {
      {"version", kConfigSchemaVersion},
      {"toneStyle", config.toneStyle == ToneStyle::New ? "new" : "old"},
      {"inputMethod", config.inputMethod == InputMethod::Vni ? "vni" : "telex"},
      {"shortcutsEnabled", config.shortcutsEnabled},
      {"shortcuts", shortcuts},
  };
  return doc.dump(2) + "\n";
}

}  // namespace goxvi
