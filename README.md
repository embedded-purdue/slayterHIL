# SlayterHIL

A hardware-in-the-loop test system codesigned with a drone flight controller. Essentially: make the drone flight software think it's flying before you risk your hardware in a real-world test.

For an in-depth overview, see [WEBSITE.md](https://github.com/embedded-purdue/slayterHIL/blob/main/WEBSITE.md). 

### Our Subteams

1. Test Automation: The front-end interface for user interaction with the HIL system
2. Flight Simulation: The in-house physics simulation that generates sensor outputs for the DUT
3. HIL Node: the deterministic injection system that simulates the DUT's sensors using the simulation's outputs.
4. DUT: the drone itself. The subteam is building flight controller software. 
