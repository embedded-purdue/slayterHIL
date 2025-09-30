#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include "mesh.cpp"
#include <cmath>

int main()
{
  auto window = sf::RenderWindow(sf::VideoMode({800u, 800u}), "CMake SFML Project");
  window.setFramerateLimit(60);

  Camera camera((sf::Vector3f){0, 0, 0}, (sf::Vector3f){0, 0, 0});

  std::vector<sf::Vector3f> drone = {
    {-100, -100, 0},
    {0, -120, 0},
    {100, -100, 0},
    {120, 0, 0},
    {100, 100, 0},
    {0, 120, 0},
    {-100, 100, 0},
    {-120, 0, 0},
    {0, 0, 0},
  };

  std::vector<std::pair<int, int>> drone_inds = {
    {0, 1},
    {1, 2}, 
    {2, 3},
    {3, 4},
    {4, 5},
    {5, 6},
    {6, 7},
    {7, 0}
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
      camera.rotation.x -= 0.05;          
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
    {
      camera.rotation.x += 0.05;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
    {
      camera.rotation.y -= 0.05;          
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
    {
      camera.rotation.y += 0.05;          
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
   

    
    window.clear();
    
    renderMesh(mesh, window, camera);

    window.display();
  }
}
