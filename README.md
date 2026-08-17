# MPX SDK

**Write your own behaviours for the MPX-Dog / Mini Pupper quadruped, in C,
TypeScript or WebAssembly text, and push them over Wi-Fi in one command.**

A *skill* is a small WebAssembly module the robot runs in a sandbox. You write
one function; the robot calls it. That's the whole model.

```c
#include "mpx_host.h"

void on_start(void) {
    MPX_LOG("hello");
    robot_gait_enum(GAIT_ADVANCE);   // start walking
    robot_delay_ms(2000);            // for two seconds
    robot_gait_enum(GAIT_INIT);      // then stand
}
```

```bash
mpx-cli deploy
```

That builds it, uploads it over Wi-Fi, runs it, and tells you what happened.

---

## Contents

1. [Set up](#1-set-up)
2. [Your first skill](#2-your-first-skill)
3. [Talking to your robot](#3-talking-to-your-robot)
4. [Four ways to move the robot](#4-four-ways-to-move-the-robot)
5. [Things that will bite you](#5-things-that-will-bite-you)
6. [When it doesn't work](#6-when-it-doesnt-work)
7. [Command reference](#7-command-reference)
8. [Sharing a skill](#8-sharing-a-skill)
9. [Simulating before you flash](#9-simulating-before-you-flash-optional)

---

## 1. Set up

### The easy way — the dev container

Open this folder in VS Code and choose **Reopen in Container**. Everything is
already inside: Python, the WASI SDK (C/C++), WABT (WAT), AssemblyScript
(TypeScript), and `mpx-cli` itself.

```bash
mpx-cli build --show-toolchains     # confirm it's all there
```

### The manual way

You need Python 3.10+, then whichever compiler matches your language:

| Language | Needs | Get it |
|---|---|---|
| **C / C++** | WASI SDK | <https://github.com/WebAssembly/wasi-sdk/releases> → `/opt/wasi-sdk` |
| **TypeScript** | AssemblyScript | `npm install -g assemblyscript` |
| **WAT** | WABT | <https://github.com/WebAssembly/wabt/releases> → `/opt/wabt` |

Then:

```bash
pip install -e mpx-cli
mpx-cli build --show-toolchains
```

### Find your robot

Power it on. It makes a Wi-Fi network called **MPX-Dog** — join it, and the
robot is at **192.168.2.1**. Open that in a browser and you get the control UI.

If you've put the robot on your own Wi-Fi instead, use the address it reports
there.

**Set the address once** so you never type `--ip` again — put a `.env` next to
your project:

```
MPX_HOST=192.168.2.1
```

---

## 2. Your first skill

```bash
mpx-cli init my_skill --lang c      # or --lang ts, --lang wat
cd my_skill
```

You get:

```
my_skill/
├── manifest.json      name and version — the CLI reads paths from this
├── Makefile           convenience wrapper
├── README.md
├── include/
│   └── mpx_host.h     every function the robot exposes
└── src/
    └── my_skill.c     ← edit this
```

Edit `src/my_skill.c`, then:

```bash
mpx-cli deploy
```

One command: **build → upload → run**. You never retype the skill name because
`manifest.json` already has it.

```
✅ Compiled my_skill.c → my_skill.wasm
   📦 Size: 1.2 KB
📤 Uploading my_skill.wasm to 192.168.2.1:80...
✅ Uploaded to 'my_skill.wasm'
▶️  Running 'my_skill.wasm' on 192.168.2.1:80...
✅ my_skill.wasm — ok (2.3s)
```

If the skill fails you get the reason and a non-zero exit code — not a green
tick:

```
❌ my_skill.wasm — trapped during execution (0.4s)
   'mpx-cli logs' shows what the robot logged.
```

### The examples

```bash
cd examples/hello-world    && mpx-cli deploy    # the smallest thing that moves
cd examples/hello-world-ts && mpx-cli deploy    # the same, in TypeScript
cd examples/four-ways      && mpx-cli deploy    # every control path, annotated
```

Or point at the folder instead of stepping into it — same result:

```bash
mpx-cli deploy examples/four-ways
```

`deploy` takes a **project directory**, a **source file**, or nothing at all
(meaning "the project I'm standing in"). All three build, upload and run the
same skill.

**`examples/four-ways` is the one to read.** It's a single file covering all
four ways to move the robot, with the gotchas written down.

---

## 3. Talking to your robot

Your skill calls **host functions** the firmware provides. They're declared in
`include/mpx_host.h`, and `HOST_FUNCTIONS.md` documents all 49 in C,
AssemblyScript and WAT.

The ones you'll use first:

```c
MPX_LOG("text");                     /* → mpx-cli logs                    */
robot_gait_enum(GAIT_ADVANCE);       /* start a named gait                */
robot_delay_ms(500);                 /* wait — the ONLY correct way       */
robot_set_servo_angle(id, centideg); /* set one joint                     */
robot_flush();                       /* actually send it                  */
robot_read_angle_cdeg(id);           /* read a joint back                 */
```

**Servo ids are 1–12**, front-right first:

| | Hip | Shoulder | Knee |
|---|---|---|---|
| Front right | 1 | 2 | 3 |
| Front left | 4 | 5 | 6 |
| Rear right | 7 | 8 | 9 |
| Rear left | 10 | 11 | 12 |

There are named constants — `SERVO_FR_KNEE` and so on — so you don't have to
remember that.

**Every host function returns a code.** `0` is success:

| Code | Meaning |
|---|---|
| `MPX_OK` (0) | fine |
| `MPX_ERR_ARG` (−1) | bad id, index or pointer |
| `MPX_ERR_NOT_LOCKED` (−2) | **you** don't hold the servo bus |
| `MPX_ERR_NO_REPLY` (−3) | the driver board didn't answer |
| `MPX_ERR_READONLY` (−4) | that parameter is calibration, not a gain |
| `MPX_ERR_CANCELLED` (−5) | your skill was stopped mid-call |
| `MPX_ERR_STATE` (−6) | right call, wrong time |

`mpx_strerror(code)` turns any of them into text.

---

## 4. Four ways to move the robot

They're layered. Each gives more control and takes more responsibility.
**Use the highest one that does what you need.**

| | Path | You provide | Robot provides |
|---|---|---|---|
| 1 | **Gaits** | a name | the whole walk |
| 2 | **Built-in IK** | foot positions | the joint angles |
| 3 | **Your own IK** | joint angles | nothing but the bus |
| 4 | **Low-level servo** | angles **and** Kp/Kd | nothing |

```c
/* 1 — the robot walks */
robot_gait_enum(GAIT_ADVANCE);

/* 2 — you place the foot, firmware solves the leg */
robot_ik_fr(x_mm, splay_deg, -78.0f);

/* 3 — your kinematics, firmware just passes it on */
robot_set_servo_angle(SERVO_FR_KNEE, (int)(my_angle_deg * 100));
robot_flush();

/* 4 — you own the motor's control loop too */
servo_lock();
servo_set_gain(SERVO_FR_KNEE, MPX_PARAM_KP_POSITION, 95.0f);
servo_stage(SERVO_FR_KNEE, 135.0f, 400.0f, 0.0f, 0.0f);
servo_commit();
servo_unlock();
```

**`examples/four-ways/` runs all four**, commented, with `WHICH` at the top to
try one at a time. Read that file before writing anything serious.

### Gait names

`none · init · step · advance · back · left · right · turnL · turnR · jump ·
jumpfwd · twerk · stretch · balance · stanford · frontkick · wiggle ·
buttshrug` — and about thirty more. Full list in `HOST_FUNCTIONS.md`.

Use `robot_gait_enum(GAIT_ADVANCE)` rather than a string: a typo becomes a
compile error instead of a robot that quietly does nothing.

---

## 5. Things that will bite you

These are the mistakes everyone makes once.

**Nothing moves until `robot_flush()`.** Set every joint for a frame, then
flush **once**. Flushing after each joint gives you a robot that judders.

```c
robot_set_servo_angle(SERVO_FR_SHOULDER, 1200);
robot_set_servo_angle(SERVO_FR_KNEE,     -800);
robot_flush();                    /* ← one per frame, not per joint */
robot_delay_ms(16);
```

**Angles are centidegrees, relative to centre.** `4500` is 45.00°. The range is
**±135°** (`±13500`). Centre is `0`.

**There are two angle frames and they run opposite ways.**

- `robot_set_servo_angle()` and `robot_read_angle_cdeg()` — **relative**, ±135° from centre
- `servo_stage()`, `servo_read()` and Servo Studio — **absolute**, 0–270° with 135 = centre
- `abs = 135 + rel`

`robot_read_position()` is in the *absolute* frame. Building a closed loop on
it against `robot_set_servo_angle()` makes the error term the wrong sign, and
the loop diverges instead of converging. **Use `robot_read_angle_cdeg()` to
close a loop.**

**Your skill is killed after 60 seconds**, whatever it's doing.

**`robot_delay_ms()` is the only way to wait.** A busy-loop never yields, so the
robot can't service anything else while you spin.

**There is no libm.** A freestanding WASM skill has no `sin`, `sqrt` or
`atan2` — write your own. And if you write `atan`, use a minimax polynomial,
not the Taylor series: `x − x³/3 + x⁵/5 − x⁷/7` is off by up to **3.5°** near
|x| = 1, which is well inside a leg IK's working range. `examples/four-ways`
has versions that work.

**Leave the robot somewhere safe.** Whatever pose you finish in is where it
stays. End with `robot_gait_enum(GAIT_INIT)`.

**`servo_lock()` parks the gait.** You cannot tune gains and walk at the same
time. Lock → set gains → unlock → walk.

**Some parameters are read-only from a skill.** `MIN_POSITION_ADC`,
`MAX_POSITION_ADC` and `RANGE_POSITION_DEG` return `MPX_ERR_READONLY`. They're
the board's calibration — change one and every angle command afterwards means
something different, permanently, including the built-in gait's. Change those
from Servo Studio, watching the joint move.

---

## 6. When it doesn't work

```bash
mpx-cli logs -f
```

The robot's own log, over Wi-Fi, live. Everything the firmware knows ends up
here: the gait name that didn't match, the servo id out of range, the trap
message.

| Symptom | Likely cause |
|---|---|
| `no on_start export` | your entry point isn't named `on_start`, or wasn't exported |
| `load failed — not a valid .wasm` | the file is corrupt, or isn't a module |
| `trapped during execution` | out-of-bounds access, or a divide by zero |
| `timed out (60 s)` | your loop doesn't finish — check its bounds |
| Traps on the first host call | built against an older SDK. `mpx-cli deploy` again |
| Runs, but nothing moves | you forgot `robot_flush()` |
| Joints move the wrong way | you mixed the two angle frames — see §5 |
| `a skill is already running` | only one at a time; wait, or reboot the robot |

**Check the ABI at the top of your skill** and you'll never debug that fifth
one:

```c
if (mpx_abi_version() != MPX_ABI_VERSION) {
    MPX_LOG("rebuild me against this robot's SDK");
    return;
}
```

---

## 7. Command reference

### Everyday

| Command | What it does |
|---|---|
| `mpx-cli init <name> [--lang c\|ts\|wat]` | scaffold a new skill |
| **`mpx-cli deploy`** | **build + upload + run — the usual loop** |
| `mpx-cli deploy --no-run` | push without executing |
| `mpx-cli deploy --no-wait` | don't wait for the result |
| `mpx-cli logs [-f]` | the robot's log; `-f` follows it |
| `mpx-cli build [src] [--validate]` | compile only |
| `mpx-cli ls` | what's on the robot |
| `mpx-cli run [name]` | run something already uploaded |
| `mpx-cli delete <name>` | remove a skill |

### Marketplace

| Command | What it does |
|---|---|
| `mpx-cli login <user>` | sign in |
| `mpx-cli search <term>` | find skills |
| `mpx-cli install <skill_id>` | download one and put it on your robot |
| `mpx-cli publish .` | share yours |

### Where paths come from

Run from inside a skill directory and **you can leave every path off**. The CLI
walks up to find `manifest.json` and derives the rest:

```
manifest slug: my_skill
   source →  src/my_skill.c
   build  →  build/my_skill.wasm
   robot  →  my_skill.wasm
```

Pass a path explicitly and it always wins.

### Settings

| Variable | Default | Meaning |
|---|---|---|
| `MPX_HOST` | `192.168.2.1` | robot address |
| `MPX_PORT` | `80` | robot port |

Put them in `.env` next to your project, or pass `--ip` / `--port`.

---

## 8. Sharing a skill

```bash
mpx-cli login yourname
mpx-cli publish .
```

Others install it with:

```bash
mpx-cli install yourname~your_skill
```

The robot keeps its own record of what's installed, so you can see and remove
it from any device — phone, laptop, or the robot's web UI.

Limits: a skill must be **under 256 KB**, and the sandbox gives it **128 KB of
linear memory** and **60 seconds** to run.

---

## 9. Simulating before you flash *(optional)*

If you have a MuJoCo model of the robot, you can run a skill through physics
before it becomes torque:

```bash
python tools/mjsim.py build/my_skill.wasm
```

It runs the *same* `.wasm` the robot runs, then reports whether the robot fell,
how far it travelled, and how closely each joint tracked what you asked for. It
writes an HTML replay you can scrub through — no GPU needed. Add `--view` for
MuJoCo's 3D viewer (that one needs a display, so run it outside the container).

Worth it for gaits and anything where the robot might tip over. For ordinary
skills, `mpx-cli deploy` is faster.

---

## Reference

| File | What's in it |
|---|---|
| `HOST_FUNCTIONS.md` | all 49 host functions, in C / AssemblyScript / WAT |
| `WORKFLOW.md` | the end-to-end workflow, including firmware |
| `examples/four-ways/README.md` | the four control paths, explained |
| `include/mpx_host.h` | the C header, heavily commented |
| `abi/host_functions.json` | the ABI, machine-readable |

---

## Getting help

Run `mpx-cli logs -f` first — it usually says what's wrong.

If something in this SDK behaves differently from what's written here, treat
the documentation as the thing that's wrong, and please report it.
