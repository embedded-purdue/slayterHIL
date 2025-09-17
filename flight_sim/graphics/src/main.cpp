#include <SFML/Graphics.hpp>
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
}
