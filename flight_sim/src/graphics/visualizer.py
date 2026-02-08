import pygame
import sys
import math

SCREEN_WIDTH = 800
SCREEN_HEIGHT = 600
BG_COLOR = (30, 30, 30)
DRONE_COLOR = (0, 255, 0)
TEXT_COLOR = (255, 255, 255)

class DroneState:
    def __init__(self):
        #Orientation
        self.roll = 0.0
        self.pitch = 0.0
        self.yaw = 0.0
        
        #Position
        self.x = 0.0
        self.y = 0.0
        self.z = 0.0  # Altitude
        
        self.battery = 100.0

    def update_mock_data(self):
        #wobble
        self.yaw += 0.02
        self.roll = 0.3 * math.sin(pygame.time.get_ticks() * 0.002)
        
        #moves drone
        self.x = 200 * math.sin(pygame.time.get_ticks() * 0.001)
        self.y = 200 * math.cos(pygame.time.get_ticks() * 0.001)
        self.z = 50 + 20 * math.sin(pygame.time.get_ticks() * 0.005)

class Visualizer:
    def __init__(self):
        pygame.init()
        self.screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
        pygame.display.set_caption("Drone HIL 3D Viz")
        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont("Consolas", 18)
        self.drone = DroneState()

        #RECYCLED FROM C++ CODE
        self.body_points = [
            (-100, -100, 0),
            (100, 100, 0),
            (-100, 100, 0),
            (100, -100, 0)
        ]

    def project_3d_point(self, x, y, z):
        new_x = x * math.cos(self.drone.yaw) - y * math.sin(self.drone.yaw)
        new_y = x * math.sin(self.drone.yaw) + y * math.cos(self.drone.yaw)
        x, y = new_x, new_y
        
        new_y = y * math.cos(self.drone.pitch) - z * math.sin(self.drone.pitch)
        z = y * math.sin(self.drone.pitch) + z * math.cos(self.drone.pitch)
        y = new_y
        
        new_x = x * math.cos(self.drone.roll) - z * math.sin(self.drone.roll)
        z = x * math.sin(self.drone.roll) + z * math.cos(self.drone.roll)
        x = new_x

        final_x = x + self.drone.x 
        final_y = y + self.drone.y 
        
        scale = 1.0 + (self.drone.z / 200.0) 
        
        screen_x = int(final_x * scale) + SCREEN_WIDTH // 2
        screen_y = int(final_y * scale) + SCREEN_HEIGHT // 2
        
        return (screen_x, screen_y)

    def draw_ui(self):
        info = [
            f"ROLL:  {math.degrees(self.drone.roll):.1f}°",
            f"PITCH: {math.degrees(self.drone.pitch):.1f}°",
            f"YAW:   {math.degrees(self.drone.yaw):.1f}°"
        ]
        for i, line in enumerate(info):
            s = self.font.render(line, True, TEXT_COLOR)
            self.screen.blit(s, (10, 10 + i * 20))

    def run(self):
        while True:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit(); sys.exit()

            self.drone.update_mock_data()
            self.screen.fill(BG_COLOR)

            #draw out the drone body
            projected_points = []
            for px, py, pz in self.body_points:
                projected_points.append(self.project_3d_point(px, py, pz))

            pygame.draw.line(self.screen, DRONE_COLOR, projected_points[0], projected_points[1], 3)
            pygame.draw.line(self.screen, DRONE_COLOR, projected_points[2], projected_points[3], 3)
            
            for p in projected_points:
                pygame.draw.circle(self.screen, (255, 0, 0), p, 6)

            self.draw_ui()
            pygame.display.flip()
            self.clock.tick(70)

if __name__ == "__main__":
    app = Visualizer()
    app.run()