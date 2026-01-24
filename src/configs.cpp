#include "configs.hpp"

std::string Configs::getConfigFilePath()
{
	return (std::filesystem::path{ sago::getConfigHome() } / "MLSE" / "mlse_config.json").string();
}

Configs Configs::load()
{
	Configs configs{};

	try
	{
		if (const auto fileData = FileUtils::readFile(getConfigFilePath(), true))
		{
			auto j = json::parse(*fileData);
			Serializer s(false, j);
			serialize(s, configs);
		}
	}
	catch (...)
	{
	}

	return configs;
}

void Configs::save(Configs& configs)
{
	try
	{
		json j;
		Serializer s(true, j);
		serialize(s, configs);

		if (std::ofstream of{ getConfigFilePath() })
		{
			of << j.dump();
		}
	}
	catch (...)
	{
	}
}

void serialize(Serializer& s, Configs& configs)
{
	s.serialize("recentProjects", configs.recentProjects);
}
