#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FQBN="arduino:avr:mega:cpu=atmega2560"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli no instalado" >&2
  exit 127
fi

SKETCH_DIR=""
if [ -f "$ROOT_DIR/domotic-home.ino" ]; then
  SKETCH_DIR="$ROOT_DIR"
elif [ -f "$ROOT_DIR/src/app/Proyecto_Domotica.ino" ]; then
  SKETCH_DIR="$ROOT_DIR/src/app"
else
  echo "No se encontro sketch principal" >&2
  exit 1
fi

arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli board listall "Mega"
arduino-cli compile --fqbn "$FQBN" --warnings all "$SKETCH_DIR"
