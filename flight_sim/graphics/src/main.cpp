#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include "mesh.cpp"
#include <cmath>
#include <algorithm>

int main()
{
  auto window = sf::RenderWindow(sf::VideoMode::getDesktopMode(), "CMake SFML Project", sf::State::Fullscreen);
  window.setFramerateLimit(60);

  Camera camera((sf::Vector3f){0, 0, 0}, (sf::Vector3f){0, 0, 0});

  std::vector<sf::Vector3f> drone = {
    {-100, -100, 0},
    {100, -100, 0},
    {100, 100, 0},
    {-100, 100, 0},
    {0, 0, 0}
  };

  std::vector<std::pair<int, int>> drone_inds = {
    {0, 2},
    {1, 3},
  };

  sf::Clock clock;
 
  Mesh mesh = Mesh(drone, drone_inds);

  while (window.isOpen())
  {
    while (const std::optional event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
    {
      camera.rotation.z -= 0.05;          
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
    {
      camera.rotation.z += 0.05;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
    {
      camera.rotation.y -= 0.05;          
      if (camera.rotation.y < -3.14) camera.rotation.y = -3.14;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
    {
      camera.rotation.y += 0.05;          
      if (camera.rotation.y > 0) camera.rotation.y = 0;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
    {
      camera.position.y += 5;          
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
    {
      camera.position.y -= 5;          
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
    {
      camera.position.x += 5;          
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
    {
      camera.position.x -= 5;          
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::PageUp))
    {
      camera.zoom *= 1.1;          
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::PageDown))
    {
      camera.zoom *= 0.9;          
    }

    window.clear(sf::Color(0xaaccffff));
    renderMesh(mesh, window, camera);
    window.display();
  }
}
