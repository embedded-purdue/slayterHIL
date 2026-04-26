from ursina import *
import math

class DroneVisualizer:
    def __init__(self):
        self.app = Ursina()
        window.title = 'Drone HIL 3D Viz'
        
        # 1. Create the Drone (This is the invisible parent "container" and collider)
        self.drone = Entity(collider='box', scale=(1, 1, 1))
        
        # Central Hub
        # Central Hub (Changed to 'sphere' to make it a solid puck)
        self.hub = Entity(parent=self.drone, model='sphere', color = rgb(40, 40, 40), scale=(0.8, 0.3, 0.8))
        
        # X-Frame Arms (Two intersecting long boxes)
        Entity(parent=self.drone, model='cube', color=color.green, scale=(2.5, 0.15, 0.1), rotation_y=45)
        Entity(parent=self.drone, model='cube', color=color.green, scale=(2.5, 0.15, 0.1), rotation_y=-45)
        
        # Add solid motor mounts
        motor_offset = 0.9 # Perfect calculation for a length-3 arm at 45 degrees
        
        # Front-Left
        Entity(parent=self.drone, model='sphere', color=color.red, scale=(0.2, 0.2, 0.2), position=(-motor_offset, 0, motor_offset))
        # Front-Right
        Entity(parent=self.drone, model='sphere', color=color.red, scale=(0.2, 0.2, 0.2), position=(motor_offset, 0, motor_offset))
        # Back-Left
        Entity(parent=self.drone, model='sphere', color=color.red, scale=(0.2, 0.2, 0.2), position=(-motor_offset, 0, -motor_offset))
        # Back-Right
        Entity(parent=self.drone, model='sphere', color=color.red, scale=(0.2, 0.2, 0.2), position=(motor_offset, 0, -motor_offset))
        
        # 2. Create the Spatial Environment
        self.ground = Entity(model='plane', scale=(100, 1, 100), color = rgb(4, 0, 35), collider='box')
        self.grid = Entity(model=Grid(100, 100), scale=100, color=color.white50, rotation_x=90, y=0.01)

        # 3. Setup Camera
        camera.position = (0, 15, -20)
        camera.look_at(self.drone)

        # 4. Mock State Variables
        self.mock_time = 0.0
        self.trail_dots = []
        self.last_trail_pos = self.drone.position

        self.global_axes = self.create_axes(scale=5.0)
        self.body_axes = self.create_axes(parent_entity=self.drone, pos=(0, 0.5, 0), scale=2.5)

        # 5. UI Overlay (Anchored to top left)
        # window.top_left gives us the exact coordinate, and we offset it slightly with Vec2 so it's not flush against the bezel
        self.telemetry_ui = Text(
            text='', 
            position=window.top_left + Vec2(0.02, -0.02), 
            background=True, 
            scale=1.2
        )

        # Attach our custom update loop directly to the drone entity
        self.drone.update = self.update_loop

    def draw_trail(self):
        # Only drop a new breadcrumb if the drone has moved more than 0.5 units
        if distance(self.drone.position, self.last_trail_pos) > 0.25:
            
            # Create a semi-transparent cyan dot
            new_dot = Entity(
                model='sphere',
                color=color.rgba(255, 0, 0, 160), # 100 is the alpha (transparency)
                scale=0.15,
                position=self.drone.position
            )
            
            self.trail_dots.append(new_dot)
            self.last_trail_pos = self.drone.position
            
            # FIFO Buffer: Destroy the oldest dot if the trail gets too long
            if len(self.trail_dots) > 190:
                oldest_dot = self.trail_dots.pop(0)
                destroy(oldest_dot)

    def create_axes(self, parent_entity=None, pos=(0,0,0), scale=1.0):
        """Generates an XYZ coordinate frame. R=X, G=Y, B=Z"""
        frame = Entity(parent=parent_entity, position=pos)
        
        t = 0.01 * scale # Thickness of the lines
        L = 0.25 * scale  # Length of the lines
        
        # Sizing up the cones so they are unmistakable
        arrow_thickness = t * 6 
        arrow_length = t * 10
        
        # Pushing the text slightly past the tip of the arrow
        text_pos = L + (arrow_length / 2) + 0.1
        
        # X-Axis (RED) - Points Right
        Entity(parent=frame, model='cube', color=color.red, scale=(L, t, t), position=(L/2, 0, 0))
        Entity(parent=frame, model='cone', color=color.red, scale=(arrow_thickness, arrow_length, arrow_thickness), position=(L, 0, 0), rotation_z=-90)
        Text(parent=frame, text='X', color=color.white, position=(text_pos, 0, 0), scale=2*scale, billboard=True, origin=(0,0))

        # Y-Axis (GREEN) - Points Up
        Entity(parent=frame, model='cube', color=color.green, scale=(t, L, t), position=(0, L/2, 0))
        Entity(parent=frame, model='cone', color=color.green, scale=(arrow_thickness, arrow_length, arrow_thickness), position=(0, L, 0))
        Text(parent=frame, text='Y', color=color.white, position=(0, text_pos, 0), scale=2*scale, billboard=True, origin=(0,0))

        # Z-Axis (BLUE) - Points Forward
        Entity(parent=frame, model='cube', color=color.blue, scale=(t, t, L), position=(0, 0, L/2))
        Entity(parent=frame, model='cone', color=color.blue, scale=(arrow_thickness, arrow_length, arrow_thickness), position=(0, 0, L), rotation_x=90)
        Text(parent=frame, text='Z', color=color.white, position=(0, 0, text_pos), scale=2*scale, billboard=True, origin=(0,0))
        
        return frame

    def update_telemetry(self):
        """Simulate incoming data"""
        self.mock_time += time.dt
        
        self.drone.x = 15 * math.sin(self.mock_time * 0.5)
        self.drone.z = 15 * math.cos(self.mock_time * 0.5)
        self.drone.y = 5 + 5 * math.sin(self.mock_time * 2) 
        
        self.drone.rotation_y += 50 * time.dt               
        self.drone.rotation_x = 20 * math.sin(self.mock_time * 2) 
        self.drone.rotation_z = 20 * math.cos(self.mock_time * 2) 

    def update_loop(self):
        """This runs every single frame"""
        self.update_telemetry()
        self.draw_trail()
        
        # Update the UI text with the latest values
        self.telemetry_ui.text = (
            f"X:     {self.drone.x:.2f}\n"
            f"Y:     {self.drone.y:.2f} (Alt)\n"
            f"Z:     {self.drone.z:.2f}\n"
            f"PITCH: {self.drone.rotation_x:.2f}°\n"
            f"ROLL:  {self.drone.rotation_z:.2f}°\n"
            f"YAW:   {self.drone.rotation_y:.2f}°"
        )
        
        # Make camera follow the drone smoothly
        camera.position = lerp(camera.position, self.drone.position + Vec3(0, 10, -15), time.dt * 2)
        camera.look_at(self.drone)

    def run(self):
        self.app.run()

if __name__ == "__main__":
    viz = DroneVisualizer()
    viz.run()