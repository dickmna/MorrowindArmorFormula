#include "PCH.h"

#include "ArmorFormula.h"
#include "Hooks.h"
#include "Settings.h"

using namespace std::literals;

SKSEPluginInfo(
    .Version = REL::Version{ 1, 0, 0, 0 },
    .Name = "MorrowindArmorFormula"sv,
    .Author = "Codex"sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
    .MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }
)

namespace
{
    void SetupLog()
    {
        auto path = SKSE::log::log_directory();
        if (!path) {
            return;
        }

        *path /= "MorrowindArmorFormula.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

        spdlog::set_default_logger(std::move(log));
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);
    }

    void LogFormulaExamples()
    {
        const auto& settings = MAF::GetSettings();
        for (const auto armor : { 0.0F, 100.0F, 300.0F, 600.0F, 900.0F }) {
            SKSE::log::info(
                "Armor {} -> damage multiplier {}",
                armor,
                MAF::MorrowindDamageMultiplier(armor, settings));
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    SetupLog();

    SKSE::log::info("MorrowindArmorFormula 1.0.0 loading");

    MAF::LoadSettings();
    LogFormulaExamples();
    MAF::Hooks::Install();

    SKSE::log::info("MorrowindArmorFormula loaded");
    return true;
}
