import serial
import csv
import os
import sys
from datetime import datetime

PORT  = "COM8"        # change to your port — check Arduino IDE bottom right
BAUD  = 115200

SCENARIOS = {
    "1": "normal_flow",
    "2": "dry_running",
    "3": "cavitation"
}

def get_filename(scenario, run_number):
    return f"{scenario}_run{run_number:02d}.csv"

def log_session(scenario):
    # find next run number
    run = 1
    while os.path.exists(get_filename(scenario, run)):
        run += 1
    filename = get_filename(scenario, run)

    print(f"\nLogging → {filename}")
    print("Press Ctrl+C to stop early\n")

    with serial.Serial(PORT, BAUD, timeout=3) as ser, \
         open(filename, "w", newline="") as f:

        writer = csv.writer(f)
        row_count = 0

        while True:
            line = ser.readline().decode("utf-8", errors="ignore").strip()

            if not line:
                continue

            # skip comments and status lines
            if line.startswith("#") or line.startswith("ERROR"):
                print(f"  [{line}]")
                continue

            # write header row as-is
            if line.startswith("timestamp"):
                writer.writerow(line.split(","))
                continue

            # data row
            parts = line.split(",")
            if len(parts) == 5:
                writer.writerow(parts)
                f.flush()
                row_count += 1
                if row_count % 50 == 0:
                    print(f"  {row_count} samples logged...")

            # stop when ESP32 signals done
            if "DONE" in line:
                print(f"\nFinished — {row_count} rows saved to {filename}")
                break

def main():
    print("═══════════════════════════════")
    print("  Pump Dataset Logger")
    print("═══════════════════════════════")
    print("Select scenario:")
    for k, v in SCENARIOS.items():
        print(f"  {k} → {v}")
    print()

    choice = input("Enter 1, 2, or 3: ").strip()
    if choice not in SCENARIOS:
        print("Invalid choice")
        return

    scenario = SCENARIOS[choice]

    print(f"\nScenario: {scenario.upper()}")
    print("Instructions:")

    if scenario == "normal_flow":
        print("  — Pump fully submerged, normal operation")
        print("  — Do nothing, just let it run")

    elif scenario == "dry_running":
        print("  — Remove pump from water")
        print("  — WARNING: submerge every 20-25s to avoid motor damage")
        print("  — Alternate: 15s dry, 5s submerged, repeat")

    elif scenario == "cavitation":
        print("  — Pump submerged and running")
        print("  — Partially block or squeeze the inlet pipe")
        print("  — Or tilt pump so inlet sucks in air intermittently")

    input("\nPress Enter when ready to start recording...")
    log_session(scenario)

    again = input("\nRecord another run? (y/n): ").strip().lower()
    if again == "y":
        main()

if __name__ == "__main__":
    main()