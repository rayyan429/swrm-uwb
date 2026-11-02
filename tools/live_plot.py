"""Live 2D plot of the tag's position.

Reads CSV lines "x,y,d1,d2,d3,d4" from the tag over USB serial and draws the
anchors, the latest position and a short trail behind it.

Usage:
    pip install -r tools/requirements.txt
    python tools/live_plot.py --port COM5

Close the PlatformIO serial monitor first; only one program can hold the port.
"""
import argparse

import matplotlib.pyplot as plt
import serial

# Keep in sync with ANCHOR_POS in lib/uwb_config/uwb_config.h (metres).
ANCHORS = [(0.0, 0.0), (0.0, 0.0), (0.0, 0.0), (0.0, 0.0)]
TRAIL_LEN = 50


def parse_line(line):
    """Return (x, y, [d1..d4]) for a position line, or None for anything else.

    The tag also prints debug text ("anchor found: ...") so most lines won't
    be CSV; those must return None rather than crash.
    """
    # TODO: split on commas, expect 2 + len(ANCHORS) values, convert to float
    # (catch ValueError), and return (x, y, distances).
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="e.g. COM5")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.1)

    plt.ion()
    fig, ax = plt.subplots()
    ax.scatter(*zip(*ANCHORS), marker="^", s=120, label="anchors")
    ax.set_aspect("equal")
    ax.grid(True)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.legend()

    trail = []
    # TODO: create the artists once here, e.g.
    #   trail_line, = ax.plot([], [], "-", alpha=0.4)
    #   tag_dot, = ax.plot([], [], "o", markersize=10)

    while plt.fignum_exists(fig.number):
        raw = ser.readline().decode(errors="ignore").strip()
        parsed = parse_line(raw)
        if parsed is not None:
            x, y, dists = parsed
            # TODO: append (x, y) to trail, keep only the last TRAIL_LEN
            # points, then update the artists with set_data().
        plt.pause(0.001)

    ser.close()


if __name__ == "__main__":
    main()
