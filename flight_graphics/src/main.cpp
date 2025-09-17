#include <SFML/Graphics.hpp>

int main()
{
  auto window = sf::RenderWindow(sf::VideoMode({1024u, 1024u}), "CMake SFML Project");
  window.setFramerateLimit(100);

  sf::CircleShape drone(10);
  drone.setPosition({512, 512});
  drone.setFillColor(sf::Color(0xff0000ff));

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

    window.draw(drone);

    window.display();
  }
}
