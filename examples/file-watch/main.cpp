#include "mls/material.hpp"
#include "wtr/watcher.hpp"

#include <SFML/Graphics.hpp>

#include <iostream>

using namespace std;
using namespace wtr;

auto show(event e)
{
    cout << to<string>(e.effect_type) + ' ' + to<string>(e.path_type) + ' ' + to<string>(e.path_name) +
                (e.associated ? " -> " + to<string>(e.associated->path_name) : "")
         << endl;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({200, 200}), "SFML works!");

    MaterialRepo repo = *MaterialRepo::loadFromFile(MLS_EXAMPLE_DIR "/example.mlsp");
    auto materialInstance = repo.makeInstance("Voronoi");

    sf::CircleShape shape(100.f);
    shape.setTextureRect({{}, {1, 1}});

    bool bFileChanged = false;
    auto watcher = wtr::watch(MLS_EXAMPLE_DIR, [&](const wtr::event& e)
    {
        if(e.effect_type == wtr::event::effect_type::modify)
        {
            bFileChanged = true;
        }
    });

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        repo.update();

        if (bFileChanged)
        {
            bFileChanged = false;

            if (auto newRepo = MaterialRepo::loadFromFile(MLS_EXAMPLE_DIR "/example.mlsp"))
            {
                repo.merge(std::move(*newRepo));
            }
        }

        window.clear();
        window.draw(shape, sf::RenderStates{materialInstance});
        window.display();
    }
}