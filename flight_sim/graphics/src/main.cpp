#include <SFML/Graphics.hpp>
<<<<<<< HEAD

int main()
{
    auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "CMake SFML Project");
    window.setFramerateLimit(144);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        window.clear();
        window.display();
    }
=======
#include <iostream>
#include <string>
#include "mesh.cpp"

int main()
{
  auto window = sf::RenderWindow(sf::VideoMode({800u, 800u}), "CMake SFML Project");
  window.setFramerateLimit(60);

  std::vector<sf::Vector3f> verts = {
    {100, 100, 0},
    {200, 200, 0},
    {100, 200, 0},
    {200, 100, 0},
    {150, 150, 0}
  };

  std::vector<std::pair<int, int>> inds = {
    {0, 1},
    {2, 3}
  };

  Camera cam({-400, -400, 0}, 0, 0);

  Mesh mesh = Mesh(verts, inds);

  while (window.isOpen())
  {
    while (const std::optional event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      }
    }
    window.clear();

    renderMesh(mesh, window, cam);

    window.display();
  }
>>>>>>> 1a058e877e5c6c95b2bc461dad2057097b497dc9
}
