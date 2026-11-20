"""Live 2D plot of the tag's position.

Reads CSV lines "x,y,d1,d2,d3" from the tag over USB serial and draws the
anchors, the latest position and a short trail behind it. It also draws a
circle around each anchor with radius = the measured distance (the tag is
where the circles cross) and shows the trilateration math for each reading.

Usage:
    pip install -r tools/requirements.txt
    python tools/live_plot.py --port COM5
    python tools/live_plot.py --demo        # no board needed, fake tag

Close the PlatformIO serial monitor first; only one program can hold the port.
"""
import argparse
import math
import random
import time

import matplotlib.pyplot as plt
from matplotlib.patches import Circle
import serial

# ---- Settings ----
# Real anchor positions in metres. Must match ANCHOR_POS in
# lib/uwb_config/uwb_config.h; fill in once the anchors are tape-measured.
ANCHORS = [(0.0, 0.0), (0.0, 0.0), (0.0, 0.0)]
# How many past positions the trail shows (~2.5 s at 20 positions a second).
TRAIL_LEN = 50

# Field used by --demo: 3 anchors in a triangle, 5 m wide and 4 m tall.
DEMO_ANCHORS = [(0.0, 0.0), (5.0, 0.0), (2.5, 4.0)]
# Random error added to each fake distance, in metres. About what calibrated
# UWB gives.
DEMO_NOISE = 0.05


# ---- Trilateration ----
# Turns distances into a position. A Python copy of trilaterate2D() in
# lib/Trilateration/trilateration.cpp, step for step, so the demo and the math
# panel use the same method as the tag.
def trilaterate(anchors, dists):
    """Return (x, y, rows) or None if there is no unique answer (fewer than 3
    anchors, or anchors in a line). rows holds each linear equation as
    (a1, a2, b), meaning a1*x + a2*y = b."""
    if len(anchors) < 3:
        return None

    # Anchor 1 is the reference: its circle gets subtracted from every other
    # anchor's circle, which cancels the x^2 and y^2 terms and leaves one
    # straight-line equation per remaining anchor.
    (x0, y0), d0 = anchors[0], dists[0]
    rows = []
    sxx = sxy = syy = sxb = syb = 0.0
    for (xi, yi), di in zip(anchors[1:], dists[1:]):
        a1 = 2 * (xi - x0)
        a2 = 2 * (yi - y0)
        b = d0 * d0 - di * di + xi * xi - x0 * x0 + yi * yi - y0 * y0
        rows.append((a1, a2, b))

        # Running sums for least squares (the 2x2 "normal equations").
        sxx += a1 * a1
        sxy += a1 * a2
        syy += a2 * a2
        sxb += a1 * b
        syb += a2 * b

    # Solve the 2x2 system. det near 0 means the anchors are in a line, so
    # the tag and its mirror image look identical and there is no unique answer.
    det = sxx * syy - sxy * sxy
    if abs(det) < 1e-6:
        return None
    x = (syy * sxb - sxy * syb) / det
    y = (sxx * syb - sxy * sxb) / det
    return x, y, rows


# ---- Fake tag for --demo ----
# Pretends to be the serial port. Like the real tag, it measures distances
# (here: true distance plus random noise) and solves them for a position, then
# sends the same CSV line the real tag will.
class FakeTag:
    def __init__(self, anchors):
        self.anchors = anchors
        self.start = time.time()

    def readline(self):
        # About 20 positions a second, like the real tag.
        time.sleep(0.05)

        # Where the pretend tag really is: driving an ellipse that stays
        # inside the anchor triangle, where accuracy is best.
        t = time.time() - self.start
        true_x = 2.5 + 1.3 * math.cos(t * 0.6)
        true_y = 1.4 + 0.8 * math.sin(t * 0.6)

        # What it "measures": each true distance plus noise. Then solve for
        # the position, exactly as the tag does with its real measurements.
        dists = [math.hypot(true_x - ax, true_y - ay) + random.gauss(0, DEMO_NOISE)
                 for ax, ay in self.anchors]
        x, y, _ = trilaterate(self.anchors, dists)
        return (",".join(f"{v:.3f}" for v in [x, y] + dists) + "\n").encode()

    def close(self):
        pass


# ---- Math panel ----
# Builds the text shown beside the plot: the measured distances, the line
# equations they turn into, and the solved position.
def math_panel_text(anchors, x, y, dists):
    lines = ["Measured distances"]
    for i, ((ax, ay), d) in enumerate(zip(anchors, dists), start=1):
        lines.append(f"  d{i} = {d:5.2f} m   anchor {i} at ({ax:.1f}, {ay:.1f})")

    lines += ["", "Each anchor gives a circle:", "  (x - xi)^2 + (y - yi)^2 = di^2", "",
              "Subtract anchor 1's circle from the",
              "others. The squares cancel, leaving lines:",
              "  a1*x + a2*y = b"]
    solved = trilaterate(anchors, dists)
    if solved is None:
        lines.append("  (anchor positions not set, or in a line)")
    else:
        for a1, a2, b in solved[2]:
            lines.append(f"  {a1:6.2f}*x + {a2:6.2f}*y = {b:6.2f}")

    lines += ["", "Solve the lines (least squares):", f"  x = {x:.2f} m,  y = {y:.2f} m"]
    return "\n".join(lines)


# ---- Reading the tag's output ----
# Turns one line from the tag into numbers. The tag also prints other text
# (start-up messages, "anchor 81: 2.13 m" debug lines), so anything that isn't
# exactly x, y and one distance per anchor is skipped by returning None.
def parse_line(line):
    """Return (x, y, [d1..d3]) for a position line, or None for anything else."""
    parts = line.split(",")
    if len(parts) != 2 + len(ANCHORS):
        return None
    try:
        values = [float(p) for p in parts]
    except ValueError:
        return None
    return values[0], values[1], values[2:]


def main():
    # ---- Command-line options ----
    # Exactly one data source: a real tag on a serial port, or the fake tag.
    parser = argparse.ArgumentParser()
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--port", help="e.g. COM5")
    source.add_argument("--demo", action="store_true", help="fake tag, no board needed")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    # ---- Data source ----
    # Both have a readline() method, so the loop below works with either.
    if args.demo:
        anchors = DEMO_ANCHORS
        ser = FakeTag(anchors)
    else:
        anchors = ANCHORS
        ser = serial.Serial(args.port, args.baud, timeout=0.1)

    # ---- Window layout ----
    # Plot on the left, math panel on the right. Interactive mode (ion) lets
    # the window redraw while the loop keeps running.
    plt.ion()
    fig, (ax, ax_math) = plt.subplots(1, 2, figsize=(11, 5.5),
                                      gridspec_kw={"width_ratios": [3, 2]})
    if args.demo:
        fig.suptitle("SIMULATED: fake distances with ~5 cm noise, no hardware")

    # ---- Things drawn once ----
    # Anchors as labelled triangles.
    ax.scatter(*zip(*anchors), marker="^", s=120, label="anchors")
    for i, (ax_x, ax_y) in enumerate(anchors, start=1):
        ax.annotate(f"A{i}", (ax_x, ax_y), textcoords="offset points", xytext=(8, 6))
    ax.set_aspect("equal")
    ax.grid(True)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")

    # ---- Things updated every reading ----
    # Created empty here and moved later, which is much faster than redrawing
    # everything each time: one circle per anchor (radius = measured
    # distance), the trail, the tag's dot and the math text.
    circles = [ax.add_patch(Circle(a, 0, fill=False, linestyle="--", alpha=0.35))
               for a in anchors]
    trail = []
    trail_line, = ax.plot([], [], "-", alpha=0.4)
    tag_dot, = ax.plot([], [], "o", markersize=10, label="tag")
    ax.legend()
    ax_math.axis("off")
    math_text = ax_math.text(0, 1, "", va="top", family="monospace", fontsize=9)

    # ---- Main loop ----
    # Runs until the window is closed: read a line, and if it's a position,
    # update everything.
    while plt.fignum_exists(fig.number):
        raw = ser.readline().decode(errors="ignore").strip()
        parsed = parse_line(raw)
        if parsed is not None:
            x, y, dists = parsed

            # Add the new position and keep only the last TRAIL_LEN.
            trail.append((x, y))
            del trail[:-TRAIL_LEN]

            # Move the trail, the dot and the circles, and refresh the math.
            xs, ys = zip(*trail)
            trail_line.set_data(xs, ys)
            tag_dot.set_data([x], [y])
            for circle, d in zip(circles, dists):
                circle.set_radius(max(d, 0.0))
            math_text.set_text(math_panel_text(anchors, x, y, dists))

            # Zoom to fit the anchors and the trail, with 1 m of margin.
            all_x = list(xs) + [a[0] for a in anchors]
            all_y = list(ys) + [a[1] for a in anchors]
            ax.set_xlim(min(all_x) - 1, max(all_x) + 1)
            ax.set_ylim(min(all_y) - 1, max(all_y) + 1)

        # Give matplotlib a moment to redraw the window.
        plt.pause(0.001)

    ser.close()


if __name__ == "__main__":
    main()
