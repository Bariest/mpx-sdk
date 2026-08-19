# Firmware coverage

Everything the robot's firmware can do about movement, and the SDK call that
reaches it.

This page exists because of one specific failure mode. If a capability lands in
the firmware and never reaches the SDK, nothing breaks — makers just hit a wall
and go and patch the firmware themselves. That is the outcome this SDK exists
to prevent, and it is invisible unless something checks for it.

So this page is **generated and enforced**. `tools/gen_coverage.py` reads
`mangdang/main/robot/robot.h`, compares it against `abi/coverage.json`, and
fails the build if the firmware grows a function nobody has classified. CI runs
it on every push.

**38 of 47 firmware functions are reachable from a skill.**
5 are boot-time or sandbox plumbing with no meaning inside a
skill. 4 are withheld on purpose, with the reason written down
below rather than left to be rediscovered.


### Built-in gaits

Movements the firmware already knows.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::send_gait_cmd()` | `mpx_gait()` · `mpx_gait_for()` · `mpx_gait_once()` · `mpx_gait_play()` |  |
| `robot::current_gait_cmd()` | `mpx_gait_current()` |  |
| `robot::get_config()` | `mpx_gait_config()` |  |
| `robot::set_config()` | `mpx_gait_config_set()` |  |
| `robot::gait_from_name()` | `mpx_gait()` | The one name table. A skill sends a name; this resolves it, including names other skills provide. |
| `robot::gait_name_count()` | `MPX_GAITS()` · `MPX_GAIT_COUNT()` | The SDK ships the same catalogue as a generated table, so a skill needs no call to enumerate. |
| `robot::gait_name_at()` | `mpx_gait_name()` |  |

### Continuous driving

Steering by velocity rather than by name.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::joy_input()` | `mpx_drive_at()` · `mpx_drive_for()` · `mpx_drive_mm_s()` · `mpx_stop()` |  |

### Body attitude

Roll, pitch and yaw with the feet planted.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::set_body_attitude()` | `mpx_body()` · `mpx_body_move()` |  |
| `robot::set_attitude_speed()` | `mpx_body_move()` | The speed travels with the pose it applies to. mpx_body_speed() was a persistent global that decided how the next mpx_body() anywhere in the file behaved; it is gone. |
| `robot::set_attitude_speed_xyz()` | `robot_set_attitude_speed_xyz()` | Per-axis attitude slew. No wrapper: one speed for the lean is what a skill wants, and mpx_body_move() takes it. Reachable as robot_set_attitude_speed_xyz() in mpx/abi.h. |

### Feet

Foot placement; the firmware solves the leg.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::front_right_ik()` | `mpx_foot_set()` · `mpx_feet_set()` · `mpx_stance_set()` |  |
| `robot::front_left_ik()` | `mpx_foot_set()` · `mpx_feet_set()` · `mpx_stance_set()` |  |
| `robot::rear_right_ik()` | `mpx_foot_set()` · `mpx_feet_set()` · `mpx_stance_set()` |  |
| `robot::rear_left_ik()` | `mpx_foot_set()` · `mpx_feet_set()` · `mpx_stance_set()` |  |

### Overlay

Adding to a frame something else produced, instead of replacing it.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::set_overlay()` | `mpx_overlay()` · `mpx_overlay_leg()` · `mpx_overlay_stance()` |  |
| `robot::get_overlay()` | `mpx_overlay_at()` |  |
| `robot::clear_overlay()` | `mpx_overlay_clear()` | Also called automatically when a skill ends, however it ends. |

### Joints

Direct joint angles; you solve the leg.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::set_servo_angle()` | `mpx_joint_set()` · `mpx_pose_set()` |  |
| `robot::flush()` | `mpx_frame_send()` · `mpx_pose_apply()` · `mpx_stance_apply()` |  |

### Servo bus

Taking the bus to talk to the driver boards.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::servo_lock()` | `mpx_bus_take()` |  |
| `robot::servo_unlock()` | `mpx_bus_release()` |  |
| `robot::servo_locked()` | `mpx_bus_held()` |  |

### Sensing

Reading the robot back.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::read_angle_cdeg()` | `mpx_joint_at()` |  |
| `robot::read_position()` | `robot_read_position()` | Raw 0-1023 in the ABSOLUTE frame. Deliberately has no mpx_ wrapper: mpx_joint_at() is the reading that matches mpx_joint_set(), and a wrapper next to it invited loops built across the two frames. Reachable as robot_read_position() from mpx/abi.h for diagnostics. |
| `robot::read_speed()` | `robot_read_speed()` |  |
| `robot::read_load()` | `robot_read_load()` |  |
| `robot::read_voltage()` | `robot_read_voltage()` |  |
| `robot::read_temperature()` | `robot_read_temperature()` |  |
| `robot::read_temperature_c()` | `mpx_read_temperature_c()` |  |
| `robot::read_moving()` | `robot_read_moving()` |  |
| `robot::read_current()` | `robot_read_current()` |  |
| `robot::ping_servo()` | `mpx_joint_ping()` |  |
| `robot::imu_read()` | `mpx_imu()` · `mpx_imu_tilt()` |  |
| `robot::imu_print()` | `mpx_imu_log()` |  |

### Calibration

Per-joint zero offsets.

| Firmware | Call it from a skill with | |
|---|---|---|
| `robot::get_offset()` | `mpx_offset()` |  |
| `robot::set_offset()` | `mpx_offset_set()` |  |
| `robot::reset_offsets()` | `mpx_offsets_reset()` |  |

## Withheld on purpose

**`robot::set_servo_speed()`** — Dead in the firmware. The driver boards' SPI frame has no speed field — robot::flush() builds it from position plus a fixed current cap and never reads goal_speed. The SDK used to wrap this as mpx_joint_speed(); it returned MPX_OK and changed nothing, which is worse than not existing. Move a joint at a chosen speed by stepping mpx_joint_set() on a ticker.

**`robot::set_all_servo_speed()`** — Dead in the firmware. The driver boards' SPI frame has no speed field — robot::flush() builds it from position plus a fixed current cap and never reads goal_speed. The SDK used to wrap this as mpx_joint_speed(); it returned MPX_OK and changed nothing, which is worse than not existing. Move a joint at a chosen speed by stepping mpx_joint_set() on a ticker.

**`robot::set_studio_mode()`** — Servo Studio is the human-in-the-loop calibration tool. A skill silently putting the robot into Studio mode would take the bus away from a person who is watching a joint move. Use the robot's web UI.

**`robot::studio_mode()`** — Paired with set_studio_mode. A skill that needs to know whether it may touch the bus should call mpx_bus_take() and read the return code.

## Not applicable

Firmware plumbing that runs before a skill exists or after it ends:
`init()`, `imu_init()`, `servo_owned_by_skill()`, `release_skill_bus_lock()`, `gait_to_name()`.

`release_skill_bus_lock()` is worth knowing about even though you cannot call
it: the sandbox releases your bus lock when your skill ends, however it ends.
Forgetting `mpx_bus_release()` leaves the robot recoverable.

## If something is missing

Open an issue naming the firmware function. If the firmware can do it and the
SDK cannot, that is a bug in the SDK — not a reason to fork the firmware.

See also: [reference](../REFERENCE.md) ·
[how motion works](../MOVEMENT.md)
