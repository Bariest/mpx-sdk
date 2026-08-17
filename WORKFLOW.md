# MPX — how you actually work, end to end

Written for a maker. Every command here has been run and verified except where
it says **needs hardware**.

---

## 0. Do this once: flash the firmware

Everything built in this session is **source on your disk, not on the robot yet.**
Until you flash, none of it exists as far as the robot is concerned.

```bash
cd C:\esp\projects\mangdang
idf.py build
idf.py -p COM<n> flash monitor
```

**One thing to expect:** this is **ABI v2**, and it is a deliberate breaking
change. Any `.wasm` you built before today will load and then trap on its first
call to a changed function. Rebuild it — `mpx-cli deploy` — and it works. All
five bundled examples are already rebuilt.

**What must still be true after flashing** (check these before trusting anything else):

| Check | Expect |
|---|---|
| `mpx-cli deploy examples/walk-with-gains` | robot walks exactly as before — **no mirrored joints** |
| `GET /v1/robot/status` | the same 12 calibration offsets as now |
| Servo Studio degree readout | unchanged |
| a skill calling `servo_stage()` without `servo_lock()`, Studio open | returns **-2** (it used to succeed) |
| `servo_set_gain(1, 1, 500)` | returns **-4**, read-only |

The gait command path is byte-identical to what you had, so **your NVS
calibration stays valid and nothing needs recalibrating.**

---

## 1. Day one — make it move, install nothing

**Open the MPX Studio artifact.** No toolchain, no Docker, no CLI.

1. Pick a preset (start with *Custom inverse kinematics*) or write your own JavaScript
2. Press **▶ Simulate** — instant. You get:
   - all four legs drawn to scale from your MJCF
   - one angle chart per joint
   - the support polygon with the centre of mass in it
   - a verdict: *"Looks safe to run — CoM stays 23 mm inside the support polygon at its worst"*
3. Press **⬇ Download .wasm** — a real module, ~1.7 KB
4. Put it on the robot: the **Skills** tab in the robot's web UI, or `mpx-cli upload my_motion.wasm`

**What this catches before anything moves:** joints past ±135°, a servo id that
does not exist, a misspelled gait name, NaN from a divide-by-zero, a loop that
never ends, and a pose that would tip the robot over.

**What it cannot catch:** a wrong link length or a mis-measured constant. It
proves the skill runs, terminates and stays in range — not that your kinematics
describe reality.

**The limit worth knowing:** Studio bakes the *motion* you authored, so the
module replays it. It cannot read the IMU or close a loop. For that, go to §2.

---

## 2. Real skill development

```bash
# Open C:\esp\projects\mpx-sdk in VS Code → "Reopen in Container"
# WASI SDK, WABT and AssemblyScript are already inside. Nothing to install.

mpx-cli init my_skill --lang c     # or --lang ts, --lang wat
cd my_skill
# edit src/my_skill.c
mpx-cli deploy
```

`deploy` is build + upload + run in one step. Paths come from `manifest.json`, so
you never retype the skill name.

**It now tells you the truth:**

```
✅ walk.wasm — ok (1.2s)                            exit 0
❌ bad.wasm — trapped during execution (1.2s)       exit 1
❌ garbage.wasm — load failed — not a valid .wasm   exit 1
```

All three used to print a green tick and exit 0.

**Set the robot's address once** and stop typing `--ip`:

```
# .env next to your project
MPX_HOST=192.168.1.42
```

**Useful forms:**

```bash
mpx-cli deploy --no-run        # push without executing
mpx-cli deploy --no-wait       # fire and forget
mpx-cli logs -f                # follow the robot's own log over Wi-Fi
mpx-cli ls                     # what is on the robot
```

---

## 3. Writing your own inverse kinematics

The two-step loop that actually works:

**Step 1 — prototype in Studio.** JavaScript has `Math.acos` and `Math.atan2`;
a freestanding WASM skill does not, so you would otherwise be writing your own
`sqrt` before you can test an idea. Iterate here until the motion looks right.
Seconds per iteration, zero risk.

**Step 2 — port to C** once the shape is correct. The maths is identical; only
the syntax changes. Then `mpx-cli deploy`.

**Your real numbers, from your MJCF** — use these, not estimates:

```c
#define L1 50.0f     /* thigh, mm  — lf2 -> lf3   */
#define L2 56.0f     /* calf,  mm  — lf3 -> foot  */
/* hips: LF(+50,+23.5)  RF(+50,-23.5)  RB(-66,-23.5)  LB(-66,+23.5) mm */
```

**To close a loop, read back with the right function:**

```c
int err = target_cdeg - robot_read_angle_cdeg(SERVO_FR_KNEE);
robot_set_servo_angle(SERVO_FR_KNEE, target_cdeg + err / 4);
robot_flush();
```

`robot_read_angle_cdeg()` is in the **same frame** `robot_set_servo_angle()`
takes. `robot_read_position()` is the *opposite* frame — a loop built on it
diverges instead of converging. This is the single easiest way to get hurt, and
it is why the new function exists.

---

## 4. When it does not do what you expected

```bash
mpx-cli logs -f
```

Everything the firmware knows now reaches you over Wi-Fi: the gait name that did
not match, the servo id out of range, the WAMR trap message. It used to go to a
UART nobody watching over Wi-Fi could see.

**Check the return codes** — since ABI v2 every host function has one:

| Code | Meaning |
|---|---|
| `0` | ok |
| `-1` | bad argument — id, index or pointer |
| `-2` | **you** do not hold the servo bus |
| `-3` | the driver board did not answer |
| `-4` | read-only calibration parameter |
| `-5` | your skill was cancelled mid-call |
| `-6` | right call, wrong time |

`mpx_strerror(code)` turns any of them into text for logging.

---

## 5. Tuning gains on real hardware

Two ways in:

**Servo Studio** — the robot's web UI, Settings → Servo Testing. Live scope, step
a joint, watch the overshoot, tune Kp/Kd until you like it.

**From a skill** — `examples/walk-with-gains` is the worked example:

```
servo_lock()  →  servo_set_gain(...)  →  servo_unlock()  →  gait
```

`servo_lock()` parks the gait, so you cannot tune and walk at the same time.

**Three parameters are now read-only from a skill** — `MIN_POSITION_ADC`,
`MAX_POSITION_ADC`, `RANGE_POSITION_DEG` return `-4`. They are the board's
calibration, not control gains: change one and every angle command afterwards
means something different, and `servo_save_config()` burns it into the driver
board's flash where a reboot will not clear it. Change those from Servo Studio,
with a human watching the joint move.

---

## 6. Publishing and installing

```bash
mpx-cli login <username>
mpx-cli publish .                  # from your skill directory
mpx-cli search backflip
mpx-cli install jojo~backflip      # gateway → robot, one step
mpx-cli install jojo~backflip --run
```

`install` downloads the artifact, checks it really is a WASM module before
uploading, and records the provenance on the **robot** — so a second phone or
laptop sees it as installed and can uninstall it. That record used to live only
in one browser's localStorage.

> **One open item:** your gateway's artifact endpoint is unknown to me — it is not
> in either repo. `install` tries `--url`, then an artifact URL in the manifest,
> then `/v1/skills/{id}/artifact`, then `/download`, and prints every URL it
> tried if all miss. If your gateway serves them elsewhere, that message tells
> you exactly what to add. **needs your gateway to confirm**

---

## 7. Validating a gait in MuJoCo

Keep this for what it is good at.

```bash
cd C:\output_mjcf\output_mjcf
python walk.py
```

**Use MuJoCo when momentum decides the outcome** — jumps, fast trots, recovery
from a shove. Not for authoring; the browser loop is faster and safe.

`robot.xml` now has the real hardware travel on all 12 joints
(`range="-2.3562 2.3562"` = ±135°) and a matching `ctrlrange`. Before this it
had **no joint limits at all** — `ctrlrange` was ±180°, so MuJoCo would have
simulated poses the robot cannot physically reach and reported them as fine.

---

## The shape of it

```
   idea
     │
     ├─ Studio (browser)        seconds, safe, no install     ── most iterations
     │     └─ download .wasm ──────────────────┐
     │                                          │
     ├─ mpx-cli deploy (C / TS / WAT)  ─────────┤       ── when you need sensors
     │     └─ mpx-cli logs -f                   │
     │                                          ▼
     ├─ MuJoCo                            THE ROBOT
     │     (only for dynamic gaits)             │
     │                                          │
     └─ mpx-cli publish ──► marketplace ──► mpx-cli install
```

**Rule of thumb:** simulate in the browser, deploy with the CLI, reach for
MuJoCo only when momentum matters, and read `mpx-cli logs` the moment something
surprises you.
