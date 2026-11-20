# Guide: calibration and accuracy tools

**Tasks:** G (calibration helper) and H (accuracy logger). (Task J, tidying `live_plot.py`, is already done.)
**Hardware needed:** none. You'll test with fake data, and the tools get used for real once the boards work.
**Language:** Python.
**Time:** about 3–4 hours in total.
**Branch name:** `calibration-tools`

Do [SETUP.md](SETUP.md) first if you haven't, including `pip install -r tools/requirements.txt`.

## Why these tools matter
Once the hardware works, two lab jobs come next:
- **Milestone 3, calibration.** The raw distances read off by a roughly constant amount, tens of cm, until `ANTENNA_DELAY` in `lib/uwb_config/uwb_config.h` is tuned. Doing that by hand means reading numbers off a scrolling screen, averaging them in your head, guessing a new value and re-flashing, over and over. **G** does the averaging and the math for you.
- **Milestone 6, accuracy test.** Put the tag on marked spots, compare where it *says* it is with where it *really* is, then repeat with someone blocking an anchor. **H** records that and gives you the numbers for the write-up.

Having both ready before the boards work saves a lot of lab time.

## What the tag prints (both tools read this)
Over USB serial at **115200 baud**, the tag prints two kinds of lines:
```
anchor 81: 2.13 m          ← one per measured distance (from newRange())
1.92,0.88,2.11,3.40,2.95   ← position lines: x,y,d1,d2,d3 (metres)
```
It also prints start-up text like `tag starting` and `Device ID: DECA0302`. **Your tools must skip any line they don't understand**, without crashing.

To read serial in Python (only one program can hold the port, so close `pio device monitor` first):
```python
import serial
ser = serial.Serial("COM5", 115200, timeout=1)
line = ser.readline().decode(errors="ignore").strip()   # "" if nothing arrived within 1 s
```

---

## Before you start: look at the live plot
Run `python tools/live_plot.py --demo` and read through `tools/live_plot.py`. Every section has a comment explaining it, and your tools follow the same pattern: read lines, parse them, use the numbers. The demo draws a circle around each anchor with radius = the measured distance (the tag is where they cross), and the panel on the right shows the trilateration math live.

---

## Part 1: calibration helper, `tools/calibrate.py` (task G)
Branch: `calibration-tools`.

### What it does
The tag sits at a known distance from one anchor, measured with a tape. The script collects that anchor's readings, averages them, and tells you what to set `ANTENNA_DELAY` to.

```
> python tools/calibrate.py --port COM5 --anchor 81 --true 2.00 --current 16385
Collecting 100 readings from anchor 81...
  mean 2.312 m   median 2.310 m   spread (std) 0.021 m
  error: +0.312 m (reads long)
Set ANTENNA_DELAY to 16418 (was 16385), re-flash ALL boards, and run this again.
```

### The math
Raising `ANTENNA_DELAY` by 1 shortens every reading by about **9.4 mm** (0.00938 m). It's one DW3000 time unit of 15.65 ps, which light covers in 4.69 mm, and it counts twice. So:

```
new_delay = current_delay + round((mean_measured - true_distance) / 0.00938)
```
A reading that's too long gives a positive error, so the delay goes up. Too short, and it goes down. Because every board uses the same `ANTENNA_DELAY`, **all boards must be re-flashed** after each change.

### Steps
1. **Arguments** (use `argparse`, like `live_plot.py` does):
   - `--port COM5` *or* `--file some.txt` (exactly one; see `add_mutually_exclusive_group` in `live_plot.py`). `--file` lets you test with no hardware.
   - `--anchor 81`: which anchor to listen to, as the two hex digits the tag prints.
   - `--true 2.00`: the tape-measured distance in metres.
   - `--current 16385`: the `ANTENNA_DELAY` the boards were flashed with.
   - `--samples 100` (default 100).
2. **Parse lines** with a regular expression. This one matches `anchor 81: 2.13 m` and captures the address and the number:
   ```python
   import re
   RANGE_RE = re.compile(r"anchor ([0-9A-F]{2}): (-?\d+\.\d+) m")
   m = RANGE_RE.match(line)
   if m and m.group(1) == args.anchor:
       distance = float(m.group(2))
   ```
   Anything that doesn't match is skipped.
3. **Collect** readings until you have `--samples` of them. For `--file`, stop at the end of the file even if there are fewer, and say so.
4. **Report** the mean, median and standard deviation (Python's `statistics` module has `mean`, `median` and `stdev`), the error, and the suggested new delay.
5. **Sanity checks:**
   - If fewer than 10 readings arrived, say so and don't suggest a delay. The tag probably can't reach that anchor.
   - If the spread (stdev) is more than about 0.1 m, warn that readings are unstable (something blocking, or the tag moving) and that the suggestion may be off.

### Test it without hardware
Make a test file, `tools/samples/calib_sample.txt`, with some realistic tag output. Include junk lines and another anchor's readings, since the script must ignore them:
```
tag starting
Device ID: DECA0302
anchor 81: 2.30 m
anchor 82: 3.51 m
anchor 81: 2.33 m
anchor 81: 2.29 m
anchor 81: 2.32 m
```
(Add more `anchor 81` lines if you like.) Then:
```
python tools/calibrate.py --file tools/samples/calib_sample.txt --anchor 81 --true 2.00 --current 16385
```
With exactly the 4 readings above, the mean is 2.31 m, the error is +0.31 m, and the suggestion is 16385 + round(0.31 / 0.00938) = 16385 + 33 = **16418**. Also check: `--anchor 82` gives 1 reading (too few, so no suggestion), and a file with no matching lines doesn't crash.

Commit: `git add tools/calibrate.py tools/samples/calib_sample.txt` and `git commit -m "Add antenna delay calibration helper"`.

---

## Part 2: accuracy logger, `tools/log_accuracy.py` (task H)
Same branch, `calibration-tools`.

### What it does
For each marked spot on the floor: you type its true position, the script records the tag's reported positions for a few seconds, then prints and saves the error.

```
> python tools/log_accuracy.py --port COM5 --out results.csv --label line-of-sight
True position "x y" (or q to quit): 1.0 2.0
  50 samples: mean (1.06, 1.93)  error 0.092 m  jitter 0.031 m
True position "x y" (or q to quit): 3.5 1.0
  50 samples: mean (3.44, 1.08)  error 0.100 m  jitter 0.028 m
True position "x y" (or q to quit): q
Saved 2 spots to results.csv. Average error 0.096 m.
```

### Reuse the plot's parser
`tools/live_plot.py` already has `parse_line()`, which turns `x,y,d1,d2,d3` into `(x, y, [d1, d2, d3])` and returns `None` for anything else. Don't rewrite it:
```python
from live_plot import parse_line
```
This works because Python looks for imports in the script's own folder, `tools/`.

### Steps
1. **Arguments:** `--port COMx` *or* `--demo`, plus `--out results.csv`, `--label` (free text describing the conditions, e.g. `line-of-sight` or `blocked-anchor-2`), and `--samples 50`.
2. **Loop:**
   - Ask for the true position with `input()`. Split it into two floats. If that fails, say so and ask again rather than crashing. `q` quits.
   - Throw away old lines first. Serial data piles up while you're typing, and you don't want positions from before the tag was put down. With pyserial, `ser.reset_input_buffer()` does this.
   - Read lines and keep the ones `parse_line()` accepts, until you have `--samples` positions.
3. **Work out, per spot:**
   - **mean position** = average of the x's, average of the y's
   - **error** = straight-line distance from the mean position to the true position, `math.hypot(mean_x - true_x, mean_y - true_y)`. This is the accuracy.
   - **jitter** = average distance of each sample from the mean position. This is how much it wobbles.
4. **Save** one row per spot to `--out` with the `csv` module: `label, true_x, true_y, mean_x, mean_y, error, jitter, samples`. **Append** (open with `"a"`) so several runs can go into one file, and write the header row only if the file is new (`os.path.exists`).
5. **At the end**, print how many spots you saved and their average error.

### Test it without hardware: `--demo`
The fake tag in `live_plot.py` drives around in a loop, which doesn't suit this tool: you need a tag that *stays put*. Write a small fake in your script that returns the true position plus a little noise:
```python
class StillTag:
    """Pretends to be a tag sitting still at (x, y), with ~5 cm of noise."""
    def __init__(self):
        self.x, self.y = 0.0, 0.0
    def readline(self):
        time.sleep(0.02)
        x = self.x + random.gauss(0, 0.05)
        y = self.y + random.gauss(0, 0.05)
        return f"{x:.3f},{y:.3f},1.0,2.0,3.0\n".encode()
    def reset_input_buffer(self):
        pass
```
In `--demo` mode, set `tag.x, tag.y` to the true position the user types, so the fake tag is "placed" on that spot. Expected results: the **error is 1 cm or less** (averaging 50 noisy samples cancels most of the noise) and the **jitter is around 0.06 m**. Try a fake with a fixed offset (e.g. `self.x + 0.2`) and check that the error comes out at about 0.2 m.

`f"{x:.3f},{y:.3f},1.0,2.0,3.0"` has 3 distances, which matches the 3 anchors `parse_line()` expects.

### Commit and open the PR
```
git add tools/log_accuracy.py
git commit -m "Add accuracy test logger"
git push -u origin calibration-tools
gh pr create --fill
```
Also add a line about each tool to the usage section in the roadmap (milestone 3 for `calibrate.py`, milestone 6 for `log_accuracy.py`), so people know they exist.

## If you get stuck
| Problem | Likely cause |
|---|---|
| `could not open port 'COM5'` | Wrong COM number (check Device Manager → Ports), or the PlatformIO monitor still has the port open. |
| `ModuleNotFoundError: No module named 'serial'` | Run `pip install -r tools/requirements.txt`. Note the package is called `pyserial`, but you import `serial`. |
| `ModuleNotFoundError: No module named 'live_plot'` | Run the script as `python tools/log_accuracy.py`, so its folder is `tools/`. |
| Every line is skipped | Print the raw lines to see what's actually arriving. Check for upper/lower-case hex in the anchor address, and the number of commas. |
