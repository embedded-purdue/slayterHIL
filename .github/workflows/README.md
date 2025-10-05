# GitHub Actions Workflows

This directory contains GitHub Actions workflows for the SlayterHIL project.

## Active Workflows

### 1. `zephyr-build.yml` - Multi-Board Zephyr Build
- **Purpose**: Comprehensive build and test for multiple target boards
- **Triggers**: Push and pull requests to main branch
- **What it does**:
  - Builds for multiple boards: qemu_x86, esp32s3_devkitc, nucleo_f302r8
  - Runs tests on QEMU
  - Creates releases with firmware artifacts for all boards
  - Only creates releases on pushes to main branch

### 2. `cmake-build.yml` - Traditional CMake Build
- **Purpose**: Build non-Zephyr components using standard CMake
- **Triggers**: Push and pull requests to main branch
- **What it does**:
  - Builds flight_controller and flight_sim components if they have CMakeLists.txt
  - Useful for future non-Zephyr components

## Disabled Workflows

### `placeholder_ubuntu.yml.disabled` - Basic Zephyr Build Test
- **Status**: Disabled (renamed from .yml to .yml.disabled)
- **Reason**: Redundant with zephyr-build.yml

### `placeholder_windows.yml.disabled` - Python Package
- **Status**: Disabled (renamed from .yml to .yml.disabled)
- **Reason**: No Python code in current codebase

## Usage

The workflows will automatically run when you:
- Push code to the main branch
- Create a pull request to the main branch

## Build Artifacts

- **Zephyr builds**: Available in the Actions tab as downloadable artifacts
- **Releases**: Automatically created on main branch pushes with firmware files

## Troubleshooting

If builds fail:
1. Check the Actions tab in GitHub for detailed logs
2. Ensure all dependencies are properly installed
3. Verify board configurations in the `test_node/app/boards/` directory
4. Check that the Zephyr workspace is properly initialized

## Adding New Boards

To add support for new boards:
1. Add the board name to the matrix in `zephyr-build.yml`
2. Create appropriate device tree overlay files in `test_node/app/boards/`
3. Test the build locally before pushing
