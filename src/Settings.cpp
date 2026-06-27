#include "Settings.h"

#include "PCH.h"

namespace
{
    constexpr auto kConfigPath = "Data/SKSE/Plugins/MorrowindArmorFormula.toml";

    std::string Trim(std::string_view value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos) {
            return {};
        }

        const auto last = value.find_last_not_of(" \t\r\n");
        return std::string{ value.substr(first, last - first + 1) };
    }

    std::string StripComment(std::string_view line)
    {
        auto inString = false;
        for (std::size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"') {
                inString = !inString;
            } else if (!inString && line[i] == '#') {
                return Trim(line.substr(0, i));
            }
        }
        return Trim(line);
    }

    bool ParseBool(std::string_view raw, bool fallback)
    {
        auto value = Trim(raw);
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (value == "true" || value == "1" || value == "yes" || value == "on") {
            return true;
        }
        if (value == "false" || value == "0" || value == "no" || value == "off") {
            return false;
        }
        return fallback;
    }

    float ParseFloat(std::string_view raw, float fallback)
    {
        auto value = Trim(raw);
        if (value.empty()) {
            return fallback;
        }

        if (value.front() == '"' && value.back() == '"' && value.size() >= 2) {
            value = value.substr(1, value.size() - 2);
        }

        float parsed = fallback;
        const auto* begin = value.data();
        const auto* end = value.data() + value.size();
        const auto result = std::from_chars(begin, end, parsed);
        if (result.ec != std::errc{} || result.ptr != end) {
            return fallback;
        }
        return parsed;
    }

    MAF::ArmorSource ParseArmorSource(std::string_view raw, MAF::ArmorSource fallback)
    {
        auto value = Trim(raw);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (value == "displayed" || value == "displayeddamageresist") {
            return MAF::ArmorSource::kDisplayedDamageResist;
        }
        if (value == "displayedplushidden" || value == "displayed_plus_hidden" || value == "displayed+hidden") {
            return MAF::ArmorSource::kDisplayedPlusHiddenArmorSlots;
        }
        return fallback;
    }

    void ApplySetting(MAF::Settings& settings, std::string_view key, std::string_view value)
    {
        if (key == "enabled") {
            settings.enabled = ParseBool(value, settings.enabled);
        } else if (key == "affectPlayer") {
            settings.affectPlayer = ParseBool(value, settings.affectPlayer);
        } else if (key == "affectNPCs") {
            settings.affectNPCs = ParseBool(value, settings.affectNPCs);
        } else if (key == "affectCreatures") {
            settings.affectCreatures = ParseBool(value, settings.affectCreatures);
        } else if (key == "armorSource") {
            settings.armorSource = ParseArmorSource(value, settings.armorSource);
        } else if (key == "percentHealthPerArmorPoint") {
            settings.percentHealthPerArmorPoint = ParseFloat(value, settings.percentHealthPerArmorPoint);
        } else if (key == "minDamageMultiplier") {
            settings.minDamageMultiplier = ParseFloat(value, settings.minDamageMultiplier);
        } else if (key == "hiddenArmorPerSlot") {
            settings.hiddenArmorPerSlot = ParseFloat(value, settings.hiddenArmorPerSlot);
        } else if (key == "hiddenArmorHead") {
            settings.hiddenArmorHead = ParseBool(value, settings.hiddenArmorHead);
        } else if (key == "hiddenArmorBody") {
            settings.hiddenArmorBody = ParseBool(value, settings.hiddenArmorBody);
        } else if (key == "hiddenArmorHands") {
            settings.hiddenArmorHands = ParseBool(value, settings.hiddenArmorHands);
        } else if (key == "hiddenArmorFeet") {
            settings.hiddenArmorFeet = ParseBool(value, settings.hiddenArmorFeet);
        } else if (key == "vanillaArmorReductionPerPoint") {
            settings.vanillaArmorReductionPerPoint = ParseFloat(value, settings.vanillaArmorReductionPerPoint);
        } else if (key == "vanillaMaxReduction") {
            settings.vanillaMaxReduction = ParseFloat(value, settings.vanillaMaxReduction);
        } else if (key == "requireRecentHitData") {
            settings.requireRecentHitData = ParseBool(value, settings.requireRecentHitData);
        } else if (key == "affectUnidentifiedHealthDamage") {
            settings.affectUnidentifiedHealthDamage = ParseBool(value, settings.affectUnidentifiedHealthDamage);
        } else if (key == "preserveDragonhide") {
            settings.preserveDragonhide = ParseBool(value, settings.preserveDragonhide);
        } else if (key == "dragonhideDamageMultiplier") {
            settings.dragonhideDamageMultiplier = ParseFloat(value, settings.dragonhideDamageMultiplier);
        } else if (key == "staleHitDamageTolerance") {
            settings.staleHitDamageTolerance = ParseFloat(value, settings.staleHitDamageTolerance);
        } else if (key == "logAdjustments") {
            settings.logAdjustments = ParseBool(value, settings.logAdjustments);
        }
    }
}

namespace MAF
{
    Settings& GetSettings()
    {
        static Settings settings;
        return settings;
    }

    void LoadSettings()
    {
        auto& settings = GetSettings();

        std::ifstream file{ kConfigPath };
        if (!file) {
            SKSE::log::warn("Config file not found: {}", kConfigPath);
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            const auto stripped = StripComment(line);
            if (stripped.empty() || stripped.front() == '[') {
                continue;
            }

            const auto eq = stripped.find('=');
            if (eq == std::string::npos) {
                continue;
            }

            const auto key = Trim(std::string_view{ stripped }.substr(0, eq));
            const auto value = Trim(std::string_view{ stripped }.substr(eq + 1));
            ApplySetting(settings, key, value);
        }

        settings.percentHealthPerArmorPoint = (std::max)(0.0F, settings.percentHealthPerArmorPoint);
        settings.minDamageMultiplier = std::clamp(settings.minDamageMultiplier, 0.0F, 1.0F);
        settings.hiddenArmorPerSlot = (std::max)(0.0F, settings.hiddenArmorPerSlot);
        settings.vanillaArmorReductionPerPoint = (std::max)(0.0F, settings.vanillaArmorReductionPerPoint);
        settings.vanillaMaxReduction = std::clamp(settings.vanillaMaxReduction, 0.0F, 0.99F);
        settings.dragonhideDamageMultiplier = std::clamp(settings.dragonhideDamageMultiplier, 0.0F, 1.0F);
        settings.staleHitDamageTolerance = (std::max)(0.0F, settings.staleHitDamageTolerance);

        SKSE::log::info(
            "Loaded config: enabled={}, percentHealthPerArmorPoint={}, minDamageMultiplier={}, requireRecentHitData={}, affectUnidentifiedHealthDamage={}, preserveDragonhide={}, dragonhideDamageMultiplier={}, logAdjustments={}",
            settings.enabled,
            settings.percentHealthPerArmorPoint,
            settings.minDamageMultiplier,
            settings.requireRecentHitData,
            settings.affectUnidentifiedHealthDamage,
            settings.preserveDragonhide,
            settings.dragonhideDamageMultiplier,
            settings.logAdjustments);
    }
}
