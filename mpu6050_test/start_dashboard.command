#!/bin/bash
# ─────────────────────────────────────────────────────────────
#  MPU-6050 dashboard launcher
#
#  Double-click this file in Finder.  It serves this folder on
#  localhost and opens the dashboard in Chrome.
#
#  Why a server instead of just opening dashboard.html?
#  Chrome treats file:// as a special case: Web Bluetooth usually
#  works but forgets permissions, and behaviour differs between
#  versions.  http://localhost is a proper secure context, so both
#  the Bluetooth and the USB buttons behave predictably.
# ─────────────────────────────────────────────────────────────
set -e

cd "$(dirname "$0")"
PORT=8080

# Pick the first free port if 8080 is taken.
while lsof -nP -iTCP:$PORT -sTCP:LISTEN >/dev/null 2>&1; do
  PORT=$((PORT + 1))
done

URL="http://localhost:$PORT/dashboard.html"
echo "Serving $(pwd)"
echo "Dashboard: $URL"
echo
echo "Leave this window open while you use the dashboard."
echo "Press Ctrl-C to stop."
echo

# Open the browser once the server is actually up.
( sleep 1.2; open -a "Google Chrome" "$URL" 2>/dev/null || open "$URL" ) &

exec python3 -m http.server "$PORT" --bind 127.0.0.1
