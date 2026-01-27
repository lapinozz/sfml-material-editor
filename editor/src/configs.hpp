#pragma once

#include "file-utils.hpp"
#include "imgui.h"
#include "mls/serializer.hpp"

#include <filesystem>
#include <platform_folders.h>
#include <string>
#include <vector>

struct Configs
{
    std::vector<std::string> recentProjects;
    bool autoLoadLastProject{};

    bool needOpenMenu{};

    static std::string getConfigFilePath();

    void loadFromFile();
    void saveToFile();

    void openMenu();
    void showMenu();
};

void serialize(Serializer& s, Configs& configs);
