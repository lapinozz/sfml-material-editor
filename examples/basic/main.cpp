#include <SFML/Graphics.hpp>

#include "mls/material.hpp"

int main()
{
    sf::RenderWindow window(sf::VideoMode({200, 200}), "SFML works!");

    MaterialRepo repo = *MaterialRepo::loadFromFile(MLS_EXAMPLE_DIR "/example.mlsp");
    auto materialInstance = repo.makeInstance("Voronoi");

    sf::CircleShape shape(100.f);
    shape.setTextureRect({{}, {1, 1}});

    sf::Clock clock;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        materialInstance.setValue("time", clock.getElapsedTime().asSeconds());

        window.clear();
        window.draw(shape, {materialInstance});
        window.display();
    }
}