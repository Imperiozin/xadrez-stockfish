#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
python3 tools/setup_engine.py --output "${1:-bin/Unix}"
