#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include "mesh.cpp"
#include <cmath>
#include <algorithm>

int main()
{
  auto window = sf::RenderWindow(sf::VideoMode::getDesktopMode(), "CMake SFML Project", sf::State::Fullscreen);
  // window.setFramerateLimit(60);

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
    sf::Time deltaTime = clock.restart();
    float dt = deltaTime.asSeconds();

    while (const std::optional event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      } else if (const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
        if (mouseScrolled->wheel == sf::Mouse::Wheel::Vertical) {
          camera.rotation.y += dt * mouseScrolled->delta * 100;
          if (camera.rotation.y < -3.14) camera.rotation.y = -3.14;
          if (camera.rotation.y > 0) camera.rotation.y = 0;
        }
        else {
          camera.rotation.z += dt * mouseScrolled->delta * 100;
        }
      }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
    {
      camera.rotation.z -= 2 * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
    {
      camera.rotation.z += 2 * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
    {
      camera.rotation.y -= 2 * dt;          
      if (camera.rotation.y < -3.14) camera.rotation.y = -3.14;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
    {
      camera.rotation.y += 2 * dt;          
      if (camera.rotation.y > 0) camera.rotation.y = 0;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
    {
      camera.position.y -= 500 * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
    {
      camera.position.y += 500 * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
    {
      camera.position.x -= 500 * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
    {
      camera.position.x += 500 * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::PageUp))
    {
      camera.zoom *= exp(dt * 2);   
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::PageDown))
    {
      camera.zoom *= exp(-dt * 2);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0))
    {
      camera = (Camera){(sf::Vector3f){0, 0, 0}, (sf::Vector3f){0, 0, 0}};
    }

    window.clear(sf::Color(0xaaccffff));

    int fps = 1 / dt;
    drawTextAtPoint("FPS: " + std::to_string(fps) + "\nControls:\nPan: WASD\nRotate: Arrow Keys\
                    \nZoom: PgUp/PgDn\nReset Camera: 0", {0, 0}, 1, window);

    renderMesh(mesh, window, camera);
    window.display();
  }
}
