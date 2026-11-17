# Guide: anchor addresses and the 3-anchor setup

**Tasks:** D (unique anchor addresses) and E (switch the repo from 4 anchors to 3).
**Hardware needed:** none. Everything here is checked by building and by running the plot's demo mode.
**Time:** about 1–2 hours.
**Branch name:** `three-anchors`

Do [SETUP.md](SETUP.md) first if you haven't.

## Background (5-minute read)
The system is **3 anchors + 1 tag**. Each one is an ESP32 wired to a BU03 UWB radio module.

- The **anchors** sit at fixed, measured spots around the field.
- The **tag** goes on the robot. It asks each anchor in turn "how far am I from you?" by sending a *poll* addressed to that anchor. Only the anchor with that address replies. The tag turns the reply's timing into a distance, and with 3 distances it works out its (x, y).

The addresses are `0x81`, `0x82` and `0x83`: `ANCHOR_ADDR_BASE` (`0x80`) plus the anchor number. You can see the tag doing this in `uwb_tag/src/main.cpp`:
```cpp
for (int i = 0; i < NUM_ANCHORS; i++) {
    uint8_t addr = ANCHOR_ADDR_BASE + 1 + i;   // 0x81, 0x82, ...
```

Two problems right now:
1. **Every anchor uses the same address (`0x81`).** If all three answer the same poll, their replies collide and the tag measures garbage.
2. **The code is still set up for 4 anchors**, but we're using 3.

## Part 1: unique anchor addresses (task D)

### Where the anchor number comes from
Open `uwb_anchor/platformio.ini`. Each anchor has its own build *environment*:
```ini
[env:anchor1]
build_flags = -DANCHOR_ID=1
```
`-DANCHOR_ID=1` is like writing `#define ANCHOR_ID 1` at the top of the code. So when you build with `pio run -e anchor2`, `ANCHOR_ID` is `2`. That's how one piece of code becomes three different anchors.

### What to change
Open `uwb_anchor/src/main.cpp` and find:
```cpp
// TODO(milestone 4): every anchor currently gets the same address. Make it
// ANCHOR_ADDR_BASE + ANCHOR_ID so anchor1 = 0x81, anchor2 = 0x82 etc.
const uint8_t anchorAddress = ANCHOR_ADDR_BASE + 1;
```
Make the address depend on `ANCHOR_ID` instead of the fixed `1`, then delete the TODO comment.

### Check it
```
cd uwb_anchor
pio run -e anchor1 -e anchor2 -e anchor3
```
All three should say `SUCCESS`. (Once the boards are soldered, each anchor prints `listening as 81`, `82` or `83` at start-up, which confirms it on the hardware.)

Commit:
```
git add uwb_anchor/src/main.cpp
git commit -m "Give each anchor its own address from ANCHOR_ID"
```

## Part 2: switch from 4 anchors to 3 (task E)

Work through this checklist. Each item says what to change and why.

### 1. `lib/uwb_config/uwb_config.h`
- [ ] `#define NUM_ANCHORS 4` → `3`. Everything that loops over anchors uses this, so it's the main switch.
- [ ] `ANCHOR_POS`: delete the `// anchor 4` row so there are exactly 3. Leave the values as `0.0f` for now; they get filled in with tape-measured positions once the anchors are mounted.
- [ ] The comment `anchor1 = 0x81 ... anchor4 = 0x84` → `... anchor3 = 0x83`.

### 2. `uwb_anchor/platformio.ini`
- [ ] Delete the whole `[env:anchor4]` section (2 lines).

### 3. `tools/live_plot.py`
- [ ] `ANCHORS`: 3 entries instead of 4 (all `(0.0, 0.0)` for now, same as `ANCHOR_POS`).
- [ ] `DEMO_ANCHORS`: change the 4 corners to a 3-anchor triangle, e.g. `[(0.0, 0.0), (5.0, 0.0), (2.5, 4.0)]`, and update the comment above it.
- [ ] The docstring at the top says `"x,y,d1,d2,d3,d4"` → `"x,y,d1,d2,d3"`. Same in `parse_line()`'s docstring: `[d1..d4]` → `[d1..d3]`.

⚠️ **The trap:** `parse_line()` only accepts lines with exactly `2 + len(ANCHORS)` numbers. The demo's fake tag makes one distance per entry in `DEMO_ANCHORS`. **`ANCHORS` and `DEMO_ANCHORS` must have the same length**, or the demo silently draws nothing: every line gets rejected, with no error.

### 4. Docs
- [ ] `docs/FILE_LAYOUT.md`: "4 anchors" → "3 anchors", "anchor1 to anchor4" → "anchor1 to anchor3", `d1..d4` → `d1..d3`, `x,y,d1,d2,d3,d4` → `x,y,d1,d2,d3`. In the data-flow diagram, delete the `A4["Anchor 4"]` and `A4 --> R` lines.
- [ ] `docs/ROADMAP.md`, milestone 4: tick `[x]` the "Switch the repo to 3 anchors" item, and the anchor address item if Part 1 is done (keep "then flash `anchor1` to `anchor3`" for later).

### Leave these alone
- **`uwb_tag/src/main.cpp`:** Rayyan is working in this file (task C). Its code already uses `NUM_ANCHORS`, so it adapts automatically. Rayyan will update the CSV example comment there.
- **`uwb_tag/test/test_trilateration/test_main.cpp`:** the 4-anchor tests are still valid (`trilaterate2D` works with any number ≥ 3), and Rayyan is adding a 3-anchor test.

### Check it
1. **Firmware builds:**
   ```
   cd uwb_anchor
   pio run -e anchor1 -e anchor2 -e anchor3
   cd ../uwb_tag
   pio run -e esp32dev
   ```
   All `SUCCESS`. If `pio run` in `uwb_anchor` still lists `anchor4`, the ini section isn't fully deleted.
2. **Tests still pass** (needs gcc, see SETUP.md): `pio test -e native` gives `4 test cases: 4 succeeded`.
3. **The demo works:** from the repo root:
   ```
   python tools/live_plot.py --demo
   ```
   You should see **3 triangles** and the moving tag dot with its trail. If the anchors appear but the dot never moves, that's the trap above: check the two lists have the same length.
4. **Nothing still says 4 anchors:** search the repo (Ctrl+Shift+F in VS Code) for `anchor4`, `0x84`, `d4` and `NUM_ANCHORS 4`. The only hits should be in `uwb_tag/src/main.cpp`, which is Rayyan's.

Notice in the demo that the fake tag's loop goes outside the triangle near the top. That's a preview of a real effect: positions are most accurate **inside** the triangle the anchors make.

### Commit and open the PR
```
git add lib/uwb_config/uwb_config.h uwb_anchor/platformio.ini tools/live_plot.py docs/FILE_LAYOUT.md docs/ROADMAP.md
git commit -m "Switch to 3 anchors"
git push -u origin three-anchors
gh pr create --fill
```
Ask Rayyan to review it. **Try to get this merged early**: Rayyan's tag work depends on the CSV format changing to `x,y,d1,d2,d3`.

## If you get stuck
| Problem | Likely cause |
|---|---|
| `#error "Build with an anchor env"` | You ran `pio run` without `-e anchorN`, or `ANCHOR_ID` isn't in `build_flags`. |
| `too many initializers for 'const float [3][2]'` | `ANCHOR_POS` still has 4 rows but `NUM_ANCHORS` is 3. |
| Demo shows anchors but no moving dot | `ANCHORS` and `DEMO_ANCHORS` have different lengths. |
| `Not a PlatformIO project` | Run `pio` from inside `uwb_anchor/` or `uwb_tag/`. |
