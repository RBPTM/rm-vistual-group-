#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./build/image_toolbox data/armor_blue.png output/toolbox-blue blue
./build/armor_lab data/armor_blue.png blue output/armor-blue
