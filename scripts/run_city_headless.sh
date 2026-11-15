#!/usr/bin/env bash
# EdgeTrust - Run Erlangen City Simulation in Terminal (Headless Cmdenv)

set -e
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VEINS_DIR="$PROJECT_DIR/omnetpp/veins/examples/veins"
if [ ! -d "$VEINS_DIR" ]; then
    VEINS_DIR="/home/aashiq/Desktop/FinalYearProject/edgetrust-test/omnetpp/veins/examples/veins"
fi
SUMO_BIN="/home/aashiq/.local/src/sumo-1.22.0/bin"
LAUNCHD_BIN="$VEINS_DIR/../../bin/veins_launchd"

export PATH="$SUMO_BIN:$PATH"

# 1. Start veins_launchd if not running on port 9999
if ! ss -tulpn | grep -q ":9999 "; then
    echo "[*] Starting background veins_launchd daemon on port 9999..."
    python3 "$LAUNCHD_BIN" -vv -c "$SUMO_BIN/sumo" > /tmp/veins_launchd_city.log 2>&1 &
    LAUNCHD_PID=$!
    sleep 1
else
    echo "[*] veins_launchd already active on port 9999."
fi

# 2. Run OMNeT++ City Simulation
CONFIG="${1:-City_Full_Erlangen}"
SIM_TIME="${2:-30s}"

echo "[*] Running City Simulation:"
echo "    Config: $CONFIG"
echo "    Time Limit: $SIM_TIME"
echo "    Working Dir: $VEINS_DIR"

cd "$VEINS_DIR"
./run -f city.ini -u Cmdenv -c "$CONFIG" --sim-time-limit="$SIM_TIME"

echo ""
echo "[+] Simulation finished! Summary of recorded metrics:"
SCA_FILE="results/$CONFIG-#0.sca"
if [ -f "$SCA_FILE" ]; then
    echo "--- SDN Controller Telemetry ---"
    grep "sdn_" "$SCA_FILE" | awk '{printf "  %-32s : %s\n", $3, $4}'
    echo ""
    echo "--- RSU Features Extracted ---"
    grep "edgeTrust_extractedRecords" "$SCA_FILE" | awk '{printf "  %-36s : %s\n", $2, $4}'
    echo ""
    echo "--- Regional Fog Tracked Vehicles ---"
    grep "edgeTrust_fogTrackedVehicles" "$SCA_FILE" | awk '{printf "  %-36s : %s\n", $2, $4}'
fi
