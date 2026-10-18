#!/bin/bash
set -euo pipefail

sim_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cmake -S "$sim_dir" -B "$sim_dir/build" "$@"
cmake --build "$sim_dir/build" --target flight_sim --parallel
