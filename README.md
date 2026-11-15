# SlayterHIL

A hardware-in-the-loop test system codesigned with a drone flight controller. Essentially: make the drone flight software think it's flying before you risk your hardware in a real-world test. The payoff is safe, repeatable, automated testing for embedded flight code, without crashing a real drone.

### Our Subteams

1. Test Automation: The front-end interface for user interaction with the HIL system
2. Flight Simulation: The in-house physics simulation that generates sensor outputs for the DUT
3. Test Node: the deterministic injection system that simulates the DUT's sensors using the simulation's outputs.
4. DUT: the drone itself. The subteam is building flight controller software. 

### Dashboard Hosting

The test dashboard (`app/dashboard`) runs on the team Pi (`raspi-5-orcheslayter`) as a systemd service and is shared over Tailscale only (no Funnel / public access).

Open it from any device on the tailnet: **https://raspi-5-orcheslayter.tailf4a1c1.ts.net/**

The page connects its WebSocket to the same host it was loaded from, so **Start Test** works from any device on the tailnet.

### Sim Output View

Each test panel has a **Sim output** strip under the 3D view that plays back what the drone actually did. It reads JSON files that flight sim writes into `sim_output/` (override with `SIM_OUTPUT_DIR`). It draws the actual path over the planned one, with a drone model, velocity arrow, error line, a readout and error stats, and each layer can be toggled. File format: [docs/dashboard/sim_output_format.md](docs/dashboard/sim_output_format.md). To try it without the sim, run `python3 tests/mock_sim_output.py hover` and pick the file on the Hover Test panel.

How it's set up:
- `server.py` listens on `127.0.0.1:8080` (set via `DASHBOARD_HOST` / `DASHBOARD_PORT`; defaults to `localhost:8080` for local dev).
- `app/dashboard/slayterhil-dashboard.service` runs it from `~/slayterHIL/.venv` and restarts it if it crashes.
- `tailscale serve --bg 8080` publishes it to the tailnet over HTTPS.

Common commands (on the Pi):

```bash
systemctl status slayterhil-dashboard          # is it running?
journalctl -u slayterhil-dashboard -f          # follow logs
sudo systemctl restart slayterhil-dashboard    # restart (e.g. after git pull)
tailscale serve status                         # check the tailnet URL
```

First-time install (already done on the Pi):

```bash
cd ~/slayterHIL
python3 -m venv .venv && .venv/bin/pip install aiohttp
sudo cp app/dashboard/slayterhil-dashboard.service /etc/systemd/system/
sudo systemctl daemon-reload && sudo systemctl enable --now slayterhil-dashboard
tailscale serve --bg 8080
```
