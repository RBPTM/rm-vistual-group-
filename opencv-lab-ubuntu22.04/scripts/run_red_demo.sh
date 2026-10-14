#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./build/image_toolbox data/armor_red.png output/toolbox-red red
./build/armor_lab data/armor_red.png red output/armor-red
