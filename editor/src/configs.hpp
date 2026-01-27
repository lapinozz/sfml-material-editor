#pragma once

#include <platform_folders.h>

#include <string>
#include <vector>
#include <filesystem>

#include "file-utils.hpp"
#include "mls/serializer.hpp"

#include "imgui.h"

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
