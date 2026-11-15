#!/usr/bin/env bash
# EdgeTrust - Run Erlangen City Simulation in GUI Mode (Qtenv + SUMO-GUI)

set -e
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VEINS_DIR="$PROJECT_DIR/omnetpp/veins/examples/veins"
if [ ! -d "$VEINS_DIR" ]; then
    VEINS_DIR="/home/aashiq/Desktop/FinalYearProject/edgetrust-test/omnetpp/veins/examples/veins"
fi
SUMO_BIN="/home/aashiq/.local/src/sumo-1.22.0/bin"
LAUNCHD_BIN="$VEINS_DIR/../../bin/veins_launchd"

export PATH="$SUMO_BIN:$PATH"

# Kill any existing headless launchd to allow GUI launchd
if ss -tulpn | grep -q ":9999 "; then
    echo "[*] Restarting veins_launchd with sumo-gui support..."
    pkill -f "veins_launchd" || true
    sleep 1
fi

echo "[*] Launching veins_launchd with SUMO-GUI on port 9999..."
python3 "$LAUNCHD_BIN" -vv -c "$SUMO_BIN/sumo-gui" > /tmp/veins_launchd_gui.log 2>&1 &
sleep 1

CONFIG="${1:-City_Full_Erlangen}"

echo "[*] Launching OMNeT++ Qtenv for Erlangen City Simulation:"
echo "    Config: $CONFIG"
echo "    Network: CityExampleScenario (6 RSUs across 3 Fog Districts + Central SDN)"
echo ""
echo "    >>> In Qtenv GUI, click the green 'Run' or 'Fast' button to start! <<<"

cd "$VEINS_DIR"
./run -f city.ini -u Qtenv -c "$CONFIG"
