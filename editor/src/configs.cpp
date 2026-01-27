#include "configs.hpp"

std::string Configs::getConfigFilePath()
{
    return (std::filesystem::path{sago::getConfigHome()} / "MLSE" / "mlse_config.json").string();
}

void Configs::loadFromFile()
{
    try
    {
        if (const auto fileData = FileUtils::readFile(getConfigFilePath(), true))
        {
            auto j = json::parse(*fileData);
            Serializer s(false, j);
            serialize(s, *this);
        }
    } catch (...)
    {
        *this = {};
    }
}

void Configs::saveToFile()
{
    try
    {
        json j;
        Serializer s(true, j);
        serialize(s, *this);
        FileUtils::writeFile(getConfigFilePath(), j.dump());
    } catch (...)
    {
    }
}

void Configs::openMenu()
{
    needOpenMenu = true;
}

void Configs::showMenu()
{
    static constexpr auto modalId{"Preferences"};

    if (needOpenMenu)
    {
        needOpenMenu = false;
        ImGui::OpenPopup(modalId);
    }

    if (ImGui::BeginPopupModal(modalId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Checkbox("Auto open last project", &autoLoadLastProject);

        ImGui::NewLine();

        if (ImGui::Button("Close"))
        {
            saveToFile();
            ImGui::CloseCurrentPopup();
        }

        ImGui::End();
    }
}

void serialize(Serializer& s, Configs& configs)
{
    s.serialize("recentProjects", configs.recentProjects);
    s.serialize("autoLoadLastProject", configs.autoLoadLastProject);
}