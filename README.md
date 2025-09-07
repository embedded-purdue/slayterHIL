# SlayterHIL

A hardware-in-the-loop test system codesigned with a drone flight controller.

## Setup to work on orchestrator

_Note: If you are on a PC, Zephyr does not work well with WSL. Please use Windows native, or consider eventually dual-booting linux!_

0. Install prerequisites


    - [Python3](https://www.geeksforgeeks.org/python/download-and-install-python-3-latest-version/)

    - Complete **only** the following sections of the [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#getting-started-guide).
        - [Install Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies)
        - Skip *Get Zephyr and install Python dependencies*
        - [Install the Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-the-zephyr-sdk)

    - [Just command runner](https://github.com/casey/just?tab=readme-ov-file#installation) - This is used like a makefile as a hotkey for all of our project-specific commands
If you are using Windows, installing chocolatey package manager is recommended. After installing chocolatey, you can install just by running ``choco install just``.  

2. Clone this repository:
    ```bash
    git clone git@github.com:embedded-purdue/slayterHIL.git
    cd zephyr_monorepo_template
    ```

3. Create and activate a Python virtual environment:
    ```bash
    python -m venv .venv
    source .venv/bin/activate  # On Linux/macOS
    ```

4. Install west
    ```bash
    pip install west
    ```

5. Update the workspace
    ```bash
    cd firmware
    west update
    west packages pip --install
    ```
    
6. Try to build and run
    ```bash
    just run-sim
    ```
