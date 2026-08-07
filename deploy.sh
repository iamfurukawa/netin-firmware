#!/usr/bin/env bash
set -euo pipefail

rtk pio run
rtk pio run -t upload
