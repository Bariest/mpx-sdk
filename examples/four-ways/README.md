# four-ways — every way to move this robot, in one file

There are four control paths. They are layered: each one gives you more
control and takes more responsibility. **Pick the highest one that does what
you need.**

```bash
cd examples/four-ways
mpx-cli deploy
```

Set `WHICH` at the top of `src/four_ways.c` to `1`–`4` to run one section at a
time. `0` runs all four back to back.

| | Path | You provide | Firmware provides | Use when |
|---|---|---|---|---|
| 1 | **Gaits** | a name | the whole walk | you want it to walk and don't care how |
| 2 | **Built-in IK** | foot positions | the joint angles | you want your own poses, not your own trigonometry |
| 3 | **Your own IK** | joint angles | nothing but the bus | you have kinematics the firmware doesn't |
| 4 | **Low-level servo** | angles **and** Kp/Kd | nothing | you're tuning the motors themselves |

## 1. Gaits

`robot_gait_enum(GAIT_ADVANCE)` and the robot walks. Since ABI v2 it returns a
code, so a bad gait is an error rather than silence. You cannot shape the
motion — and you cannot fall over from a maths mistake either.

## 2. Built-in IK

`robot_ik_fr/fl/rr/rl(x, th0, z)` — you give a **foot position**, the firmware
solves that leg.

- `x` mm forward(+)/back(−) · `th0` deg hip splay · `z` mm **down** from the hip
- about `z = -78` is a normal stand

This is the same Stanford kinematics the gait generator uses, so your poses sit
in its frame and **inherit your calibration offsets**. `robot_set_body_pose()`
tilts the whole body if that's all you want.

## 3. Your own IK

`robot_set_servo_angle()` takes a joint angle directly, so any kinematics you
write is welcome. The two things that bite everyone:

- the angle is **centidegrees, relative to centre** (±135° = ±13500)
- **nothing moves until `robot_flush()`** — set every joint for a frame, then
  flush **once**. Flushing per joint gives you a robot that judders.

Link lengths in the file (`L1 = 50`, `L2 = 56` mm) come from the MJCF, not a
guess. To close a loop, read back with `robot_read_angle_cdeg()` — it is in the
same frame `robot_set_servo_angle()` takes. `robot_read_position()` is the
*opposite* frame and a loop built on it diverges.

## 4. Low-level servo

Unitree-style joint control:

```
servo_lock()  →  servo_set_gain()  →  servo_stage()  →  servo_commit()  →  servo_unlock()
```

`servo_lock()` **parks the gait** and takes the bus so nothing else can write.
Gains persist on the driver board after you unlock, so the built-in gait uses
them too. Stock values are **Kp = 65, Kd = 800**.

Three things to know:

- `servo_stage()` takes the **absolute AT32 angle**, 0–270 with 135 = centre —
  *not* the ±135 relative frame used in section 3. **`abs = 135 + rel`.**
- **You cannot walk while holding the lock.** Set gains, unlock, then gait.
- Params 1–3 (`MIN_POSITION_ADC`, `MAX_POSITION_ADC`, `RANGE_POSITION_DEG`) are
  the board's calibration, not gains. `servo_set_gain()` refuses them with
  `MPX_ERR_READONLY` — they'd change what every angle means, permanently.

## Notes

A WASM skill has **no libm**, so the file carries its own `sin`/`sqrt`/`atan`.
The `atan` is a minimax polynomial, not the Taylor series — the obvious
`x - x³/3 + x⁵/5 - x⁷/7` is off by up to **3.5°** near |x| = 1, which is inside
a leg IK's working range.
