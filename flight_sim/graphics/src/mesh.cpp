#pragma once

#include <cmath>
#include <SFML/Graphics.hpp>
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
  sf::Vector3f rotation;
  sf::Vector3f position;
  Camera (sf::Vector3f rot, sf::Vector3f pos) : rotation(rot), position(pos) {}
};
typedef struct Camera Camera;

typedef std::vector<sf::Vector3f> mat;

sf::Vector3f transformMV(mat matrix, sf::Vector3f vector) {
  return (sf::Vector3f){ matrix[0].x * vector.x + matrix[0].y * vector.y + matrix[0].z * vector.z,
                         matrix[1].x * vector.x + matrix[1].y * vector.y + matrix[1].z * vector.z,
                         matrix[2].x * vector.x + matrix[2].y * vector.y + matrix[2].z * vector.z,
  };
}

sf::Vector2f projectPoint(const sf::Vector3f& point, Camera cam) {
  sf::Vector3f newPoint = point;
  // Apply yaw
  mat yaw = {
    {(float)cos(cam.rotation.z), -(float)sin(cam.rotation.z), 0},
    {(float)sin(cam.rotation.z), (float)cos(cam.rotation.z), 0},
    {0, 0, 1}
  };
  newPoint = transformMV(yaw, newPoint);

  // Apply pitch
  mat pitch = {
    {1, 0, 0},
    {0, (float)cos(cam.rotation.y), -(float)sin(cam.rotation.y)},
    {0, (float)sin(cam.rotation.y), (float)sin(cam.rotation.y)}
  };
  newPoint = transformMV(pitch, newPoint);

  // Translate point
  newPoint.x -= cam.position.x;
  newPoint.y -= cam.position.y;


  return {newPoint.x, newPoint.y};
}

void renderMesh(Mesh &mesh, sf::RenderWindow &window, Camera cam) {
  for (int i = 0; i < mesh.vertices.size(); i++) {
    // Creating Point
    sf::CircleShape point(5);
    point.setPosition(projectPoint(mesh.vertices[i], cam));
    point.setPosition({point.getPosition().x + window.getSize().x/2,
      point.getPosition().y + window.getSize().y/2});
    point.setFillColor(sf::Color(0xffffffff));
    point.setOrigin({5, 5});
    window.draw(point);
  }
  for (int i = 0; i < mesh.indices.size(); i++) {
    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
    sf::Vector2f p1 = projectPoint(mesh.vertices[mesh.indices[i].first], cam);
    sf::Vector2f p2 = projectPoint(mesh.vertices[mesh.indices[i].second], cam);
    line[0].position = {p1.x + window.getSize().x/2, p1.y + window.getSize().y/2};
    line[1].position = {p2.x + window.getSize().x/2, p2.y + window.getSize().y/2};
    window.draw(line);
  }
}
