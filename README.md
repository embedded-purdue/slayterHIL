# SlayterHIL

A hardware-in-the-loop test system codesigned with a drone flight controller. Essentially: make the drone flight software think it's flying before you risk your hardware in a real-world test. The payoff is safe, repeatable, automated testing for embedded flight code, without crashing a real drone.

### Our Subteams

1. Test Automation: The front-end interface for user interaction with the HIL system
2. Flight Simulation: The in-house physics simulation that generates sensor outputs for the DUT
3. HIL Node: the deterministic injection system that simulates the DUT's sensors using the simulation's outputs.
4. DUT: the drone itself. The subteam is building flight controller software. 

### Dashboard Hosting

The test dashboard (`app/dashboard`) runs on the team Pi (`raspi-5-orcheslayter`) as a systemd service and is shared over Tailscale only (no Funnel / public access).

Open it from any device on the tailnet: **https://raspi-5-orcheslayter.tailf4a1c1.ts.net/**

> Note: the page's WebSocket still points at `ws://localhost:8080/ws`, so **Start Test** and live logs only work from a browser on the Pi itself. Other devices can view tests and RC command output.

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
