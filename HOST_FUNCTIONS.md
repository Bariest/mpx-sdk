# MPX-Dog Host Functions Reference

> **ABI v2 — skills built against v1 must be rebuilt.**
> Seventeen host functions (`robot_gait`, `robot_flush`, `robot_set_servo_angle`,
> `robot_delay_ms`, `robot_ik_*`, `robot_imu_read`, `print`, …) were registered
> with signatures declaring no result, so the error code each one computed was
> physically unreachable — a misspelled gait name and a watchdog cancellation
> both looked exactly like nothing happening. **They all return `int` now.**
> WAMR rejects the mismatch, so a v1 module traps on the first call to one of
> them. Rebuilding is one `mpx-cli deploy`. Call `mpx_abi_version()` to check
> what you are talking to.

All host functions are registered under the **`"env"`** module by the ESP32 firmware.
They are callable from within any WASM skill (C/C++, AssemblyScript, or raw WAT).

---

## Part 1 — Raw Imports

These are the low-level host functions. Each table shows the function name, its
WAMR signature (the type string the firmware uses to register it), and a description.

### SDK / Logging

| Function | WAMR Sig | Description |
|----------|----------|-------------|
| `print(ptr, len)` | `($i)` | Print a string to the robot's ESP log. `ptr` is a native pointer to the text, `len` is the byte length. |

### High-Level Gait Control

| Function | WAMR Sig | Description |
|----------|----------|-------------|
| `robot_gait(name)` | `($)` | Start a named gait (see [Gait names](#gait-names) below). |
| `robot_get_mode()` | `()i` | Returns the current gait mode as an integer (maps to [`GaitCmd`](#gait-enum) enum). |
| `mpx_abi_version()` | `()i` | The host ABI version (2). Check it before assuming anything else here works. |
| `robot_set_attitude_speed(dps)` | `(i)i` | Roll/pitch/yaw slew speed in deg/s. `0` = snap instantly; `>0` glides. Persists until changed. |
| `robot_set_attitude_speed_xyz(r, p, y)` | `(iii)i` | Per-axis slew speed, so yaw can glide slowly while roll/pitch snap. |
| `robot_set_body_pose(roll, pitch, yaw)` | `(fff)` | Hold a Stanford-IK body attitude in degrees. Safe limits: roll +/-25, pitch +/-20, yaw +/-30. |

#### Gait names

| Name | Description |
|------|-------------|
| `"none"` | Stop all gait |
| `"init"` | Return to init/stand pose |
| `"step"` | Step in place |
| `"roll"` | Roll body |
| `"pitch"` | Pitch body |
| `"stretch"` | Stretch legs |
| `"advance"` | Walk forward |
| `"back"` | Walk backward |
| `"left"` | Sidestep left |
| `"right"` | Sidestep right |
| `"turnL"` | Turn left |
| `"turnR"` | Turn right |
| `"twerk"` | Twerk! |
| `"jump"` | Jump |
| `"jumpfwd"` | Jump forward |
| `"testspeed"` | Speed test |
| `"lookup"` | Look up (gait-based) |
| `"lookdown"` | Look down (gait-based) |
| `"lookleft"` | Look left (gait-based) |
| `"lookright"` | Look right (gait-based) |
| `"lookul"` | Look upper-left |
| `"lookur"` | Look upper-right |
| `"lookll"` | Look lower-left |
| `"looklr"` | Look lower-right |
| `"flegL"` | Foreleg lift left |
| `"flegR"` | Foreleg lift right |
| `"blegL"` | Backleg lift left |
| `"blegR"` | Backleg lift right |
| `"heightup"` | Height up |
| `"heightdown"` | Height down |
| `"balance"` | Balance body |
| `"bowback"` | Bow backward |
| `"bodycycle"` | Cycle body motion |
| `"headellipse"` | Head ellipse motion |
| `"moveLF"` | Move left front leg |
| `"moveRF"` | Move right front leg |
| `"moveLB"` | Move left back leg |
| `"moveRB"` | Move right back leg |
| `"stanford"` | Stanford trot |
| `"frontkick"` | Front kick, then return to stand |
| `"wiggle"` | Rear-up tail wiggle |
| `"buttshrug"` | Distinct front-up butt shrug |
| `"wiggleL"` / `"wiggleR"` | Hold a one-sided wiggle |
| `"buttshrugL"` / `"buttshrugR"` | Hold a one-sided butt shrug |

### Configuration

| Function | WAMR Sig | Description |
|----------|----------|-------------|
| `robot_set_config(period, height, up_height, stride, tilt)` | `(iiiii)` | Set all gait parameters at once. |
| `robot_get_period()` | `()i` | Get gait period in milliseconds. |
| `robot_get_height()` | `()i` | Get body height in millimetres. |
| `robot_get_up_height()` | `()i` | Get foot lift height in millimetres. |
| `robot_get_stride()` | `()i` | Get stride length in millimetres. |
| `robot_get_tilt()` | `()i` | Get max tilt angle in degrees. |

**Parameter reference:**

| Param | Range / Typical | Description |
|-------|-----------------|-------------|
| `period` | 60–200 ms | Gait period per phase |
| `height` | 50–100 mm | Body height from ground |
| `up_height` | 5–20 mm | How high the foot lifts during swing |
| `stride` | 5–30 mm | Step length |
| `tilt` | 0–20° | Maximum body tilt during gait |

### Low-Level Servo Control

| Function | WAMR Sig | Description |
|----------|----------|-------------|
| `robot_set_servo_angle(id, centideg)` | `(ii)` | Set a servo's target angle. `id` is 1–12, `centideg` is angle × 100 (e.g. 4500 = 45.00°). |
| `robot_flush()` | `()` | Send all buffered servo commands to the bus. |
| `robot_set_servo_speed(id, speed)` | `(ii)` | Set servo movement speed. `0` = max speed, higher = slower. |
| `robot_read_position(id)` | `(i)i` | Read raw position (0–1023) in the **AT32 frame** — the same frame `servo_read()` and Servo Studio report. Returns -1 on error. **This is not the frame `robot_set_servo_angle()` takes**; see the note below. |
| `robot_read_angle_cdeg(id)` | `(i)i` | Read the measured angle in the **same frame `robot_set_servo_angle()` accepts**: signed centidegrees from centre. Returns `INT32_MIN` on a bad id. Use this to close a control loop. |
| `robot_read_speed(id)` | `(i)i` | Read signed speed. Returns -1 on error. |
| `robot_read_load(id)` | `(i)i` | Read signed load. Returns -1 on error. |
| `robot_read_voltage(id)` | `(i)i` | Read voltage (0.1 V units). Returns -1 on error. |
| `robot_read_temperature(id)` | `(i)i` | Read temperature (°C). Returns -1 on error. |
| `robot_read_moving(id)` | `(i)i` | Read moving status: 0 = stopped, 1 = moving. Returns -1 on error. |
| `robot_read_current(id)` | `(i)i` | Read signed current (mA). Returns -1 on error. |

#### Two position frames — read this before closing a loop

There are two raw position frames and they run in **opposite directions**:

- **Gait frame** — what `robot_set_servo_angle()` takes. Signed degrees
  relative to centre; positive increases the commanded value.
- **AT32 frame** — what the driver boards, `servo_read()`, `servo_read_all()`
  and Servo Studio all report. 0–270° absolute, 135° = centre.

`robot_read_position()` is in the AT32 frame, so comparing it against an angle
you commanded with `robot_set_servo_angle()` gives an error term with the wrong
sign — a naive read → correct → write loop **diverges**. Use
`robot_read_angle_cdeg()`, which reports in the commanded frame:

```c
int err_cdeg = target_cdeg - robot_read_angle_cdeg(SERVO_FR_KNEE);
robot_set_servo_angle(SERVO_FR_KNEE, target_cdeg + err_cdeg / 4);   // converges
```

#### Servo ID map

| ID | Leg | Joint |
|----|-----|-------|
| 1 | Front Right | Hip |
| 2 | Front Right | Shoulder |
| 3 | Front Right | Knee |
| 4 | Front Left | Hip |
| 5 | Front Left | Shoulder |
| 6 | Front Left | Knee |
| 7 | Rear Right | Hip |
| 8 | Rear Right | Shoulder |
| 9 | Rear Right | Knee |
| 10 | Rear Left | Hip |
| 11 | Rear Left | Shoulder |
| 12 | Rear Left | Knee |

### Calibration

| Function | WAMR Sig | Description |
|----------|----------|-------------|
| `robot_set_offset(id, centideg)` | `(ii)` | Set angular offset for a servo (centidegrees, e.g. 150 = 1.50°). |
| `robot_get_offset(id)` | `(i)i` | Get angular offset in centidegrees. |
| `robot_ping_servo(id)` | `(i)i` | Ping a servo. Returns model number (>0) on success, ≤0 on failure. |

### Utility

| Function | WAMR Sig | Description |
|----------|----------|-------------|
| `robot_delay_ms(ms)` | `(i)` | Block the WASM thread for a real-time delay. **This is the only reliable way to pause** — pure-WASM busy-loops run at near-zero wall time inside the interpreter. |

### Inverse Kinematics (per-leg)

| Function | WAMR Sig | Description |
|----------|----------|-------------|
| `robot_ik_fr(x, th0, z)` | `(fff)` | Front-right leg IK target. |
| `robot_ik_fl(x, th0, z)` | `(fff)` | Front-left leg IK target. |
| `robot_ik_rr(x, th0, z)` | `(fff)` | Rear-right leg IK target. |
| `robot_ik_rl(x, th0, z)` | `(fff)` | Rear-left leg IK target. |

All IK functions take three `float` parameters:

| Param | Unit | Description |
|-------|------|-------------|
| `x` | mm | Forward/backward position |
| `th0` | degrees | Hip rotation angle |
| `z` | mm | Height |

### IMU

| Function | WAMR Sig | Description |
|----------|----------|-------------|
| `robot_imu_read(buffer_ptr)` | `(i)` | Read the latest IMU 6-DOF sample into a buffer (must be ≥ 24 bytes). Layout: `[ax, ay, az, gx, gy, gz]` as `float`. |
| `robot_imu_print()` | `()` | Print the latest IMU data to the robot's ESP log. |

**IMU data layout** (6 floats = 24 bytes):

| Index | Field | Unit | Description |
|-------|-------|------|-------------|
| 0 | `ax` | g | Accelerometer X |
| 1 | `ay` | g | Accelerometer Y |
| 2 | `az` | g | Accelerometer Z |
| 3 | `gx` | dps | Gyroscope X |
| 4 | `gy` | dps | Gyroscope Y |
| 5 | `gz` | dps | Gyroscope Z |

---

## Part 2 — High-Level Abstractions

These are convenience wrappers provided by the SDK headers
([`mpx_host.h`](mpx-cli/src/mpx_cli/commands/resource/mpx_host.h) for C,
[`mpx_env.ts`](mpx-cli/src/mpx_cli/commands/resource/mpx_env.ts) for AssemblyScript).
They compile down to the raw host functions above — no firmware changes needed.

### Gait Enum

A type-safe enum replaces raw string names:

| Enum | Value | String |
|------|-------|--------|
| `GAIT_NONE` / `Gait.NONE` | 0 | `"none"` |
| `GAIT_INIT` / `Gait.INIT` | 1 | `"init"` |
| `GAIT_STEP` / `Gait.STEP` | 2 | `"step"` |
| `GAIT_ROLL` / `Gait.ROLL` | 3 | `"roll"` |
| `GAIT_PITCH` / `Gait.PITCH` | 4 | `"pitch"` |
| `GAIT_STRETCH` / `Gait.STRETCH` | 5 | `"stretch"` |
| `GAIT_ADVANCE` / `Gait.ADVANCE` | 6 | `"advance"` |
| `GAIT_BACK` / `Gait.BACK` | 7 | `"back"` |
| `GAIT_LEFT` / `Gait.LEFT` | 8 | `"left"` |
| `GAIT_RIGHT` / `Gait.RIGHT` | 9 | `"right"` |
| `GAIT_TURN_L` / `Gait.TURN_L` | 10 | `"turnL"` |
| `GAIT_TURN_R` / `Gait.TURN_R` | 11 | `"turnR"` |
| `GAIT_TWERK` / `Gait.TWERK` | 12 | `"twerk"` |
| `GAIT_JUMP` / `Gait.JUMP` | 13 | `"jump"` |
| `GAIT_JUMP_FWD` / `Gait.JUMP_FWD` | 14 | `"jumpfwd"` |
| `GAIT_TEST_SPD` / `Gait.TEST_SPD` | 15 | `"testspeed"` |
| `GAIT_LOOK_UP` / `Gait.LOOK_UP` | 16 | `"lookup"` |
| `GAIT_LOOK_DOWN` / `Gait.LOOK_DOWN` | 17 | `"lookdown"` |
| `GAIT_LOOK_LEFT` / `Gait.LOOK_LEFT` | 18 | `"lookleft"` |
| `GAIT_LOOK_RIGHT` / `Gait.LOOK_RIGHT` | 19 | `"lookright"` |
| `GAIT_LOOK_UL` / `Gait.LOOK_UL` | 20 | `"lookul"` |
| `GAIT_LOOK_UR` / `Gait.LOOK_UR` | 21 | `"lookur"` |
| `GAIT_LOOK_LL` / `Gait.LOOK_LL` | 22 | `"lookll"` |
| `GAIT_LOOK_LR` / `Gait.LOOK_LR` | 23 | `"looklr"` |
| `GAIT_FORELEG_LIFT_L` / `Gait.FORELEG_LIFT_L` | 24 | `"flegL"` |
| `GAIT_FORELEG_LIFT_R` / `Gait.FORELEG_LIFT_R` | 25 | `"flegR"` |
| `GAIT_BACKLEG_LIFT_L` / `Gait.BACKLEG_LIFT_L` | 26 | `"blegL"` |
| `GAIT_BACKLEG_LIFT_R` / `Gait.BACKLEG_LIFT_R` | 27 | `"blegR"` |
| `GAIT_HEIGHT_UP` / `Gait.HEIGHT_UP` | 28 | `"heightup"` |
| `GAIT_HEIGHT_DOWN` / `Gait.HEIGHT_DOWN` | 29 | `"heightdown"` |
| `GAIT_BALANCE` / `Gait.BALANCE` | 30 | `"balance"` |
| `GAIT_BOW_BACK` / `Gait.BOW_BACK` | 31 | `"bowback"` |
| `GAIT_BODY_CYCLE` / `Gait.BODY_CYCLE` | 32 | `"bodycycle"` |
| `GAIT_HEAD_ELLIPSE` / `Gait.HEAD_ELLIPSE` | 33 | `"headellipse"` |
| `GAIT_MOVE_LF` / `Gait.MOVE_LF` | 34 | `"moveLF"` |
| `GAIT_MOVE_RF` / `Gait.MOVE_RF` | 35 | `"moveRF"` |
| `GAIT_MOVE_LB` / `Gait.MOVE_LB` | 36 | `"moveLB"` |
| `GAIT_MOVE_RB` / `Gait.MOVE_RB` | 37 | `"moveRB"` |

**C:** `robot_gait_enum(GAIT_ADVANCE);`
**AS:** `robotGait(Gait.ADVANCE);`

### Degree-Based Servo Control

Set servo angles in degrees instead of centidegrees:

| Helper | Description |
|--------|-------------|
| `robot_set_servo_deg(id, deg)` / `setServoDeg(id, deg)` | Set angle in degrees (auto × 100). |
| `robot_set_servo(id, deg, speed)` / `setServo(id, deg, speed)` | Set angle + speed in one call. |

### Config Struct

A struct bundles the five config parameters:

```c
// C
robot_config_t cfg = { .period = 100, .height = 70, .up_height = 10, .stride = 10, .tilt = 10 };
robot_set_config_ex(cfg);
robot_config_t current = robot_get_config_ex();
```

```ts
// AssemblyScript
const cfg = new RobotConfig(100, 70, 10, 10, 10);
setRobotConfig(cfg);
const current = getRobotConfig();
```

### Choreography Helpers

One-liner actions that combine gait + delay + stop:

| C | AssemblyScript | Behaviour |
|---|---------------|-----------|
| `robot_walk_forward(ms)` | `walkForward(ms)` | Walk forward for N ms |
| `robot_walk_backward(ms)` | `walkBackward(ms)` | Walk backward for N ms |
| `robot_turn_left(ms)` | `turnLeft(ms)` | Turn left for N ms |
| `robot_turn_right(ms)` | `turnRight(ms)` | Turn right for N ms |
| `robot_strafe_left(ms)` | `strafeLeft(ms)` | Sidestep left for N ms |
| `robot_strafe_right(ms)` | `strafeRight(ms)` | Sidestep right for N ms |
| `robot_jump()` | `jump()` | Single jump |
| `robot_stand()` | `stand()` | Stand to init pose |
| `robot_dance(ms)` | `dance(ms)` | Twerk dance for N ms |
| `robot_step_in_place(ms)` | `stepInPlace(ms)` | Step in place for N ms |
| `robot_look_up(ms)` | `lookUp(ms)` | Look up for N ms |
| `robot_look_down(ms)` | `lookDown(ms)` | Look down for N ms |
| `robot_look_left(ms)` | `lookLeft(ms)` | Look left for N ms |
| `robot_look_right(ms)` | `lookRight(ms)` | Look right for N ms |
| `robot_look_upper_left(ms)` | `lookUpperLeft(ms)` | Look upper-left for N ms |
| `robot_look_upper_right(ms)` | `lookUpperRight(ms)` | Look upper-right for N ms |
| `robot_look_lower_left(ms)` | `lookLowerLeft(ms)` | Look lower-left for N ms |
| `robot_look_lower_right(ms)` | `lookLowerRight(ms)` | Look lower-right for N ms |
| `robot_foreleg_lift_left(ms)` | `forelegLiftL(ms)` | Lift left foreleg for N ms |
| `robot_foreleg_lift_right(ms)` | `forelegLiftR(ms)` | Lift right foreleg for N ms |
| `robot_backleg_lift_left(ms)` | `backlegLiftL(ms)` | Lift left back leg for N ms |
| `robot_backleg_lift_right(ms)` | `backlegLiftR(ms)` | Lift right back leg for N ms |
| `robot_height_up(ms)` | `heightUp(ms)` | Raise body height for N ms |
| `robot_height_down(ms)` | `heightDown(ms)` | Lower body height for N ms |
| `robot_balance(ms)` | `balance(ms)` | Balance on the spot for N ms |
| `robot_bow_back(ms)` | `bowBack(ms)` | Bow backward for N ms |
| `robot_body_cycle(ms)` | `bodyCycle(ms)` | Cycle body for N ms |
| `robot_head_ellipse(ms)` | `headEllipse(ms)` | Head ellipse motion for N ms |
| `robot_move_lf(ms)` | `moveLF(ms)` | Move left front leg for N ms |
| `robot_move_rf(ms)` | `moveRF(ms)` | Move right front leg for N ms |
| `robot_move_lb(ms)` | `moveLB(ms)` | Move left back leg for N ms |
| `robot_move_rb(ms)` | `moveRB(ms)` | Move right back leg for N ms |
| `robot_stanford_walk(ms)` | - | Stanford trot for N ms |
| `robot_front_kick()` | - | Perform one front kick |
| `robot_wiggle(ms)` | - | Wiggle for N ms |
| `robot_butt_shrug(ms)` | - | Butt shrug for N ms (separate trajectory) |

### Angle-Only Body Attitude

The firmware performs the inverse kinematics, so a skill only supplies angles:

```c
robot_roll(12.0f);                 // roll only
robot_delay_ms(1000);
robot_pitch(-10.0f);               // pitch only
robot_delay_ms(1000);
robot_yaw(20.0f);                  // yaw only
robot_delay_ms(1000);
robot_attitude(8.0f, -6.0f, 15.0f); // combined roll, pitch, yaw
robot_delay_ms(1500);
robot_reset_attitude();
```

Each call holds the requested pose until another attitude or movement API is
called. Out-of-range angles are clamped by firmware for servo safety.

### Full Pose Helper

Set all 12 servos at once and flush:

```c
// C
robot_apply_pose((robot_pose_t){
    .fr_shoulder = -30, .fr_knee = 60,
    .fl_shoulder =  30, .fl_knee = 60,
    // ...
});
```

```ts
// AssemblyScript
applyPose(new RobotPose(
    0, -30, 60,   // FR: hip, shoulder, knee
    0,  30, 60,   // FL
    0,  30, 60,   // RR
    0, -30, 60,   // RL
));
```

### Integer Printing

Print an integer directly to the log (handles negatives):

**C only:** `MPX_print_int(42);`

### IK Helpers

Set per-leg IK targets from a structured object:

```ts
// AssemblyScript only
const target = new ikTarget(10.0, 5.0, -30.0);
ikFR(target);
ikFL(target);
ikRR(target);
ikRL(target);
```

### IMU Data

Read IMU data into a structured object:

```ts
// AssemblyScript only
const imu = readImu();
// imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz
```

---

## SDK Headers

The canonical declarations live in the SDK source tree:

| Language | File |
|----------|------|
| C / C++ | [`mpx-cli/src/mpx_cli/commands/resource/mpx_host.h`](mpx-cli/src/mpx_cli/commands/resource/mpx_host.h) |
| AssemblyScript | [`mpx-cli/src/mpx_cli/commands/resource/mpx_env.ts`](mpx-cli/src/mpx_cli/commands/resource/mpx_env.ts) |
| WAT | [`mpx-cli/src/mpx_cli/commands/resource/host_functions_wat.md`](mpx-cli/src/mpx_cli/commands/resource/host_functions_wat.md) |

When you run `mpx-cli init`, the appropriate header is copied into your
project's `include/` directory automatically.

---

# Low-Level Servo Control (AT32 driver boards)

Direct joint control in the shape `unitree_legged_sdk` uses: a command per
joint, a state per joint. This bypasses the gait and the IK layer entirely.

## How it differs from Unitree — and why

Unitree streams `{q, dq, tau, Kp, Kd}` to every joint each control tick. On the
AT32 boards those values live on **two different buses**, and the split is not
cosmetic:

| | Unitree | MPX / AT32 |
|---|---|---|
| target position | per tick | per tick (`servo_stage`) |
| torque / current | per tick | per tick, as a **limit** not a demand |
| Kp / Kd | per tick | **config write**, ~1 ms, gait must be parked |
| measured velocity | reported | **not measured** — no `dq` field exists |

So the working pattern is: lock the bus, set gains once, then stream commands.

```
servo_lock()                    → parks the gait
servo_set_gain(...)  × N        → slow config path, once
loop {
    servo_write_all(cmds, 12)   → fast path, 4 SPI frames
    servo_read_all(states)      → feedback rides the same frames
}
servo_unlock()                  → gait resumes
```

## Units

Angles are the **raw AT32 angle, 0–270°, with 135° at mechanical centre.**

This is *not* the frame `robot_set_servo_angle()` uses (±90° relative to
centre, in centidegrees, with your calibration offsets applied and clamped to
the IK's reachable range). Nothing here applies offsets and nothing clamps to a
safe envelope — a bad `q_deg` drives the joint straight into its hard stop.
Start with a low `tau_ma`.

## Bus ownership

`servo_lock()` parks the gait so nothing else issues SPI transactions. It
returns `-1` if Servo Studio has the bus — a human at the console outranks a
skill.

The sandbox force-releases the lock when your skill's entry point returns,
traps, or is killed by the watchdog, so a crashed skill cannot leave the robot
parked. Call `servo_unlock()` anyway: the gait resumes sooner.

## Functions

| Function | WAMR sig | Returns |
|---|---|---|
| `servo_lock()` | `()i` | `0` ok, `-1` Studio holds the bus |
| `servo_unlock()` | `()i` | `0` |
| `servo_is_locked()` | `()i` | `1` / `0` |
| `servo_set_gain(id, param, value)` | `(iif)i` | `0` / `-1` / `-2` / `-3` |
| `servo_get_gain(id, param, out)` | `(iii)i` | writes one `f32` |
| `servo_save_config(id)` | `(i)i` | `id` 0 = all four boards |
| `servo_restore_config(id)` | `(i)i` | factory defaults, RAM only |
| `servo_stage(id, q_deg, tau_ma, kp, kd)` | `(iffff)i` | buffered, no bus traffic |
| `servo_commit()` | `()i` | pushes all four boards |
| `servo_write_all(cmd_ptr, count)` | `(ii)i` | stage + commit in one call |
| `servo_direct(id, mode, q_deg, tau_ma)` | `(iiff)i` | mode 0 idle / 1 position / 2 torque |
| `servo_read(id, out_ptr)` | `(ii)i` | 4 × `f32` |
| `servo_read_all(out_ptr)` | `(i)i` | 12 × 4 × `f32` |
| `servo_poll()` | `()i` | refresh the cache while parked |
| `servo_scan()` | `()i` | bitmask, bit 0 = servo 1 |

Error codes: `0` success, `-1` bad argument or bad pointer, `-2` bus not
locked **by you**, `-3` the board did not answer, `-4` read-only parameter.

Two clarifications the earlier version of this document got wrong:

- **`-2` means *you* do not hold the bus**, not that the bus is free. If Servo
  Studio holds it, these calls fail — a skill that never called `servo_lock()`
  used to be able to drive every joint for as long as somebody had Studio open
  in a browser, interleaving with Studio's own SPI traffic.
- **`servo_scan()` and `servo_poll()` do need the lock.** Only `servo_read()`
  and `servo_read_all()` are lock-free, because they are served from the
  feedback cache rather than putting traffic on the bus.

**`-4` is new.** `servo_set_gain()` now refuses to write params 1–3
(`MIN_POSITION_ADC`, `MAX_POSITION_ADC`, `RANGE_POSITION_DEG`). Those are the
board's calibration, not control gains: change one and every angle command
afterwards means something different — including the gait's, and including the
NVS offsets calibrated against the old mapping — and `servo_save_config()`
burns it into the driver board's flash where a reboot will not clear it. Read
them with `servo_get_gain()`; change them from Servo Studio, with a human
watching the joint move.

`servo_save_config()` and `servo_restore_config()` also validate their id now.
They take `0` for "every board" or 1–12 for one board; anything else returns
`-1`. Previously *any* invalid id — `13`, `-5`, `9999` — silently meant "every
board", so a typo could factory-reset all twelve servos' calibration and
report success.

## Parameter IDs

Same addresses Servo Studio shows.

| ID | Name | Notes |
|----|------|-------|
| 0 | `reverse_position_sensor` | calibration — per servo |
| 1 | `min_position_adc` | calibration — per servo |
| 2 | `max_position_adc` | calibration — per servo |
| 3 | `range_position_deg` | calibration — per servo |
| 4 | `reverse_motor` | calibration — per servo |
| 5 | `kp_position` | position loop P gain |
| 6 | `kd_position` | position loop D gain |
| 7 | `kp_current` | current loop P gain |
| 8 | `kff_current` | current feed-forward |
| 9 | `max_pwm_duty` | 0..1 |

Writes land in the board's **RAM**. Call `servo_save_config()` to survive a
power cycle. Overwriting IDs 0–4 destroys that servo's individual calibration.

## Struct layout

Both arrays are packed 4 × `f32` per joint, index 0 = servo 1.

**Command** — `q_deg`, `tau_ma`, `kp`, `kd`
**State** — `q_deg`, `tau_ma`, `temp_c`, `q_raw`

`temp_c` is `NaN` when that servo has never answered. `q_raw` is the same
position on the SCS 0–1023 scale, which is what the calibration parameters and
the older `robot_read_position()` speak.

> **`kp`/`kd` in the command struct are experimental.** The fields exist in the
> AT32 wire frame, but the stock board firmware ignores them and uses its
> stored `sms_config` gains. Send `0` unless your board firmware is known to
> honour per-frame gains, and use `servo_set_gain()` for real tuning.

## Example — C

```c
#include "mpx_host.h"

void on_start(void) {
    if (servo_lock() != 0) { MPX_LOG("Studio has the bus\n"); return; }

    /* Gains: slow config path, so do it once. */
    mpx_servo_set_all_gains(12.5f, 0.35f);
    servo_set_gain(2, MPX_PARAM_KFF_CURRENT, 0.42f);

    /* Commands: fast path, four SPI frames for the whole robot. */
    mpx_servo_cmd_t cmd[12];
    for (int i = 0; i < 12; ++i) {
        cmd[i].q_deg = 135.0f;   /* centre */
        cmd[i].tau_ma = 250.0f;  /* gentle current cap while testing */
        cmd[i].kp = cmd[i].kd = 0.0f;
    }
    servo_write_all(cmd, 12);
    robot_delay_ms(500);

    /* Feedback rides on the same frames. */
    mpx_servo_state_t st[12];
    servo_read_all(st);
    for (int i = 0; i < 12; ++i) {
        if (st[i].temp_c > 60.0f) { mpx_servo_all_off(); break; }
    }

    servo_unlock();
}
```

## Example — AssemblyScript

```ts
import { servo_lock, servo_unlock, setAllServoGains, ServoCmd,
         writeAllServos, readAllServos, allServosOff } from "./mpx_env";

export function on_start(): void {
    if (servo_lock() != 0) return;
    setAllServoGains(12.5, 0.35);

    const cmds = new Array<ServoCmd>(12);
    for (let i = 0; i < 12; i++) cmds[i] = new ServoCmd(135, 250, 0, 0);
    writeAllServos(cmds);

    const st = readAllServos();
    for (let i = 0; i < 12; i++) {
        if (st[i].tempC > 60) { allServosOff(); break; }
    }
    servo_unlock();
}
```

WAT imports are in
[`host_functions_wat.md`](mpx-cli/src/mpx_cli/commands/resource/host_functions_wat.md).
