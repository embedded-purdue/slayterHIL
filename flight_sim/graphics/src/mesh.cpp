#pragma once
#include <cmath>
#include <SFML/System/Vector3.hpp>
#include <vector>
#include <utility>

struct Mesh {
    std::vector<sf::Vector3f> vertices;
    std::vector<std::pair<int, int>> indices;

    Mesh() = default;

    Mesh(const std::vector<sf::Vector3f>& verts,
         const std::vector<std::pair<int, int>>& inds)
        : vertices(verts), indices(inds) {}
};

struct Camera {
  sf::Vector3f pos;
  float yaw;
  float pitch;
  Camera (sf::Vector3f position,
          float yaw,
          float pitch) {}
};

sf::Vector2f projectPoint(const sf::Vector3f& point,
                          const Camera& cam,
                          float fov, float aspectRatio) {
  sf::Vector2f newPoint = {point.x, point.y};    



  return newPoint;
}

void renderMesh(Mesh &mesh, sf::RenderWindow &window, Camera cam) {
  for (int i = 0; i < mesh.vertices.size(); i++) {
    // Creating Point
    sf::CircleShape point(5);
    point.setPosition(projectPoint(mesh.vertices[i], cam, 90, 1));
    point.setFillColor(sf::Color(0xffffffff));
    point.setOrigin({5, 5});
    window.draw(point);
  }
  for (int i = 0; i < mesh.indices.size(); i++) {
    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
    line[0].position = {mesh.vertices[mesh.indices[i].first].x, mesh.vertices[mesh.indices[i].first].y};
    line[1].position = {mesh.vertices[mesh.indices[i].second].x, mesh.vertices[mesh.indices[i].second].y};
    window.draw(line);
  }
}
