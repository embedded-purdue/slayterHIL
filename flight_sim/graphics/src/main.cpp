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

  // Initialize Camera with defualt view
  Camera camera((sf::Vector3f){0, 0, 0}, (sf::Vector3f){0, 0, 0});
  camera.zoom = 2;

  // Define drone shape
  std::vector<sf::Vector3f> drone = {
    {-100, -100, 0},
    {100, 100, 0},
    {-100, 100, 0},
    {100, -100, 0},

    // Optional center point
    // {0, 0, 0},
  };

  // Define lines between drone points
  std::vector<std::pair<int, int>> drone_inds = {
    {0, 1},
    {2, 3}
  };

  sf::Clock clock;
 
  // Create mesh with drone points/indices
  Mesh mesh = Mesh(drone, drone_inds);

  while (window.isOpen())
  {
    // get deltaTime to account for framerate differences when moving camera
    sf::Time deltaTime = clock.restart();
    float dt = deltaTime.asSeconds();

    while (const std::optional event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      } else if (const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
        // Detect mouse scrolled event
        if (mouseScrolled->wheel == sf::Mouse::Wheel::Vertical) {
          // Vertical scroll -> rotate on y axis
          camera.rotation.y += mouseScrolled->delta * 0.1;
          if (camera.rotation.y < -3.14) camera.rotation.y = -3.14;
          if (camera.rotation.y > 0) camera.rotation.y = 0;
        } else {
          // Horizontal scroll -> rotate on z axis
          camera.rotation.z += mouseScrolled->delta * -0.1;
        }
      }
    }

    // Keyboard inputs and corresponding camera movements
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
      camera.zoom = 2;
    }

    // Draw sequence
    window.clear(sf::Color(0xaaccffff));

    int fps = 1 / dt;
    // Display fps and camera controls
    drawText("FPS: " + std::to_string(fps) + "\nControls:\nPan: WASD\nRotate: Arrow Keys or "
                    + "Mouse Scroll/Trackpad\nZoom: PgUp/PgDn\nReset Camera: 0\n", {0, 0}, 18, window);

    renderMesh(mesh, window, camera);
    window.display();
  }
}
