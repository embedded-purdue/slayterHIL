import pytest
import os 
import numpy as np
import json
from datetime import datetime


class Drone:
    """Simple drone simulator for testing"""
    
    def __init__(self):
        self.altitude = 0.0
        self.target_altitude = 7.5
        self.ascent_rate = 1.5  # m/s (7.5m / 5s)
        self.descent_rate = 1.5  # m/s (7.5m / 5s)
        self.x_vel_ext = 0.0
        self.y_vel_ext = 0.0
        self.z_vel_ext = 0.0
        
    def update(self, dt, phase):
        """Update drone position based on phase"""
        if phase == "ascent":
            self.altitude += self.ascent_rate * dt
            self.altitude = min(self.altitude, 7.5)
        elif phase == "hover":
            self.altitude = 7.5
        elif phase == "descent":
            self.altitude -= self.descent_rate * dt
            self.altitude = max(self.altitude, 0.0)
    
    def get_altitude(self):
        return self.altitude
    def get_external_velocity(self, type):
        if type == "x":
            return self.x_vel_ext
        elif type == "y":
            return self.y_vel_ext
        else:
            return self.z_vel_ext

def test_drone_hover_mission():
    """
    Test drone hover mission with three phases:
    - 0-5s: Ascent to 7.5m linearly
    - 5-10s: Hover at 7.5m
    - 10-15s: Descent to 0m linearly
    """
    drone = Drone()
    dt = 0.1  # Time step in seconds
    tolerance = 0.2  # Altitude tolerance in meters
    
    test_passed = True
    failure_message = ""
    current_phase = ""
    
    try:
        # Phase 1: Ascent (0-5 seconds)
        current_phase = "Ascent"
        print("\n=== Phase 1: Ascent ===")
        for t in np.arange(0, 5, dt):
            drone.update(dt, "ascent")
            expected_altitude = min(1.5 * t, 7.5)
            print(f"t={t:.1f}s: altitude={drone.get_altitude():.2f}m (expected: {expected_altitude:.2f}m)")
            assert abs(drone.get_altitude() - expected_altitude) < tolerance, \
                f"Ascent phase: altitude mismatch at t={t:.1f}s"
        
        # Verify we reached target altitude
        assert abs(drone.get_altitude() - 7.5) < tolerance, \
            "Failed to reach target altitude of 7.5m"
        
        # Phase 2: Hover (5-10 seconds)
        current_phase = "Hover"
        print("\n=== Phase 2: Hover ===")
        for t in np.arange(5, 10, dt):
            drone.update(dt, "hover")
            expected_altitude = 7.5
            print(f"t={t:.1f}s: altitude={drone.get_altitude():.2f}m (expected: {expected_altitude:.2f}m)")
            assert abs(drone.get_altitude() - expected_altitude) < tolerance, \
                f"Hover phase: altitude mismatch at t={t:.1f}s"
        
        # Phase 3: Descent (10-15 seconds)
        current_phase = "Descent"
        print("\n=== Phase 3: Descent ===")
        time_in_descent = 0
        for t in np.arange(10, 15, dt):
            drone.update(dt, "descent")
            time_in_descent += dt
            expected_altitude = max(7.5 - 1.5 * time_in_descent, 0)
            print(f"t={t:.1f}s: altitude={drone.get_altitude():.2f}m (expected: {expected_altitude:.2f}m)")
            assert abs(drone.get_altitude() - expected_altitude) < tolerance, \
                f"Descent phase: altitude mismatch at t={t:.1f}s"
        
        # Verify we landed
        assert abs(drone.get_altitude() - 0.0) < tolerance, \
            "Failed to land at 0m"
        
        print("\n✓ All phases completed successfully!")
        failure_message = "All phases completed successfully!"
        
    except AssertionError as e:
        test_passed = False
        failure_message = f"Test failed in {current_phase} phase: {str(e)}"
        print(f"\n✗ {failure_message}")
    
    except Exception as e:
        test_passed = False
        failure_message = f"Unexpected error in {current_phase} phase: {str(e)}"
        print(f"\n✗ {failure_message}")
    
    finally:
        # Save test results to JSON based on success or failure
        
        # Load existing test results if file exists
        results_file = "drone_hover_test_results.json"
        if os.path.exists(results_file):
            with open(results_file, "r") as f:
                all_results = json.load(f)
                # Ensure it's a list
                if not isinstance(all_results, list):
                    all_results = [all_results]
        else:
            all_results = []
            
        if test_passed:
            new_result = {
                "message_id": "All phases completed successfully!",
                "timestamp": datetime.now().isoformat(),
                "X_pos": 0.0,
                "Y_pos": 0.0,
                "Z_pos": drone.get_altitude(),
                "X_vel_ext":drone.get_external_velocity("x"),
                "Y_vel_ext":drone.get_external_velocity("y"),
                "Z_vel_ext":drone.get_external_velocity("z")
            }
        else:
            new_result = {
                "message_id": failure_message,
                "timestamp": datetime.now().isoformat(),
                "X_pos": 0.0,
                "Y_pos": 0.0,
                "Z_pos": drone.get_altitude(),
                "X_vel_ext":drone.get_external_velocity("x"),
                "Y_vel_ext":drone.get_external_velocity("y"),
                "Z_vel_ext":drone.get_external_velocity("z"),
                "test_passed": False,
                "failed_phase": current_phase
            }
        # Append new result to the list
        all_results.append(new_result)
        
        # Save all results back to file
        with open(results_file, "w") as f:
            json.dump(all_results, f, indent=4)
        
        print(f"\n✓ Test results saved to {results_file}")
        print(f"Final position: X={new_result['X_pos']}, Y={new_result['Y_pos']}, Z={new_result['Z_pos']}")
        print(f"Test status: {'PASSED' if test_passed else 'FAILED'}")
        print(f"Total tests recorded: {len(all_results)}")
        
        # Re-raise the exception so pytest registers the failure
        if not test_passed:
            raise



# def test_ascent_rate():
#     """Test that ascent rate is correct (7.5m in 5s = 1.5 m/s)"""
#     drone = Drone()
#     assert drone.ascent_rate == 1.5, "Ascent rate should be 1.5 m/s"


# def test_descent_rate():
#     """Test that descent rate is correct (7.5m in 5s = 1.5 m/s)"""
#     drone = Drone()
#     assert drone.descent_rate == 1.5, "Descent rate should be 1.5 m/s"


if __name__ == "__main__":
    # Run the test
    pytest.main([__file__, "-v", "-s"])