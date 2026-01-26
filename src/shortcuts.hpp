#pragma once

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>

struct Shortcut
{
	enum Modifier
	{
		Ctrl	= 1 << 0,
		Shift	= 1 << 1,
		Alt		= 1 << 2,
	};

	std::function<void()> callback;

	std::string name;
	sf::Keyboard::Key key;
	int modifiers;

	bool matchesEvent(const sf::Event::KeyPressed& event) const
	{
		if (event.code != key)
		{
			return false;
		}
		else if (!!(modifiers & Modifier::Ctrl) != event.control)
		{
			return false;
		}
		else if (!!(modifiers & Modifier::Shift) != event.shift)
		{
			return false;
		}
		else if (!!(modifiers & Modifier::Alt) != event.alt)
		{
			return false;
		}

		return true;
	}

	std::string makeKeyStr() const
	{
		if (key == sf::Keyboard::Key::Unknown)
		{
			return {};
		}

		std::string str;

		if (modifiers & Modifier::Ctrl)
		{
			str += "Ctrl+";
		}

		if (modifiers & Modifier::Alt)
		{
			str += "Alt+";
		}

		if (modifiers & Modifier::Shift)
		{
			str += "Shift+";
		}

		str += sf::Keyboard::getDescription(sf::Keyboard::delocalize(key));

		return str;
	}
};

using Shortcuts = std::unordered_map<std::string, std::vector<Shortcut>>;