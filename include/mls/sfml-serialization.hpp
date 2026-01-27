#pragma once

#include "serializer.hpp"

#include <SFML/Graphics.hpp>

inline void serialize(Serializer& s, sf::Vector2f& vec2)
{
	s.serialize("x", vec2.x);
	s.serialize("y", vec2.y);
}

inline void serialize(Serializer& s, sf::Vector3f& vec3)
{
	s.serialize("x", vec3.x);
	s.serialize("y", vec3.y);
	s.serialize("z", vec3.z);
}

inline void serialize(Serializer& s, sf::Glsl::Vec4& vec4)
{
	s.serialize("x", vec4.x);
	s.serialize("y", vec4.y);
	s.serialize("z", vec4.z);
	s.serialize("w", vec4.w);
}