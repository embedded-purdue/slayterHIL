# SlayterHIL

A hardware-in-the-loop test system codesigned with a drone flight controller.

## Setup to work on orchestrator

_Note: If you are on a PC, Zephyr does not work well with WSL. Please use Windows native, or consider eventually dual-booting/VM linux!_

1. Install prerequisites

    - [Python3](https://www.geeksforgeeks.org/python/download-and-install-python-3-latest-version/)

    - Complete **only** the [Install Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) section of the [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#getting-started-guide).

    - [Just command runner](https://github.com/casey/just?tab=readme-ov-file#installation) - This is used like a makefile as a hotkey for all of our project-specific commands
        - If you are using Windows, installing chocolatey package manager is recommended. After installing chocolatey, you can install just by running `choco install just`.
        - If you are on Mac, use the [Homebrew](https://brew.sh/) command `brew install just`.

2. Clone this repository:
    ```bash
    git clone git@github.com:embedded-purdue/slayterHIL.git
    cd slayterHIL
    ```

3. Create and activate a Python virtual environment:
    ```bash
    python -m venv .venv
    
    source .venv/bin/activate     # Linux/macOS
    .venv\Scripts\activate.bat    # PC, Command Prompt
    .venv\Scripts\Activate.ps1    # PC, Powershell
    source .venv/Scripts/activate # PC. Git bash
    ```

4. Install west
    ```bash
    pip install west
    ```

5. Install the [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-the-zephyr-sdk)
     - Stop after that section, nothing else.

6. Update the workspace
    ```bash
    cd firmware
    west update
    west packages pip --install
    ```

7. Try to build and run
    ```bash
    just run-sim
    ```
