#pragma once

#include <platform_folders.h>

#include <string>
#include <vector>
#include <filesystem>

#include "file-utils.hpp"
#include "serializer.hpp"

struct Configs
{
	std::vector<std::string> recentProjects;

	static std::string getConfigFilePath();

	static Configs load();

	static void save(Configs& configs);
};

void serialize(Serializer& s, Configs& configs);
