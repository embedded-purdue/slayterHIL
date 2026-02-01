# SlayterHIL
*A hardware-in-the-loop flight controller testbed for Embedded Systems @ Purdue.*

SlayterHIL is a **hardware-in-the-loop (HIL)** platform that lets us run a drone **flight controller on real hardware** while the “world” around it (sensors + physics + environment) is simulated.

The payoff is **safe, repeatable, automated testing** for embedded flight code, without crashing a real drone.

---

## Why this matters
Flight controllers are a perfect embedded “stress test”:
- **Real-time constraints** (timing matters)
- **Multiple subsystems at once** (IMU + comms + control loops + motor outputs)
- **Safety-critical behavior** (you want strong validation before real flight)

SlayterHIL is also meant to feel like industry:
- small, well-scoped tickets
- CI-driven regression testing
- clear interfaces between teams
- reusable infrastructure for future club projects

---

## The big idea (in one picture, conceptually)

**Test Automation** defines a scenario → **Flight Simulation** generates realistic sensor outputs → **Test Node** delivers them deterministically to the **DUT (flight controller)** → DUT outputs motor commands → simulation updates the world → automation decides **pass/fail**.

---

## What we’re building
### Core components
- **DUT (Device Under Test):** the flight controller firmware being validated
- **Test Node:** real-time bridge between sim and DUT (deterministic injection / fast I/O)
- **Flight Simulation:** physics + sensor generation (turn motor outputs into sensor streams)
- **Test Automation:** defines tests, runs them locally/CI, and evaluates pass/fail

### Hardware & software stack
**Hardware**
- Raspberry Pi (server-side orchestration; model TBD)
- ESP32-S3 (test node / interface layer)
- Sensors planned for validation + contracts:
  - LSM6DSO IMU
  - MPL3115A2 barometer/altimeter

**Software**
- C / C++
- Zephyr RTOS (on the flashing/interface device/test node components)
- PyTest (test definitions + assertions)
- CI/CD via **GitHub Actions** (chosen over Jenkins for accessibility + ease of use)

---

## What’s been accomplished (Fall 2025)
By end of Fall 2025, the team:
- finalized the **overall system design**
- onboarded tooling + workflows
- eliminated major blockers and clarified responsibilities
- built early test artifacts and interface prototypes

