# lowlevel-servo — Unitree-style joint control

Direct control of the AT32 driver boards from a WASM skill: set the control
gains, stream joint commands, read state back.

```bash
cd examples/lowlevel-servo
mpx-cli deploy
```

## What it does

1. `servo_lock()` — takes the servo bus, which parks the gait
2. `servo_scan()` — reports which of the twelve joints answer
3. sets `kp_position` / `kd_position` on all twelve, plus `kp_current`,
   `kff_current` and `max_pwm_duty` on servo 2 (RAM only, not saved)
4. sweeps the four thigh joints ±12° around centre while the other eight hold
5. reads temperature back each step and cuts power if any joint exceeds 60 °C
6. `servo_unlock()` — hands the bus back, gait resumes

## Read this before widening the numbers

`q_deg` is the **raw AT32 angle, 0–270°, 135° = mechanical centre**. It is not
the frame `robot_set_servo_angle()` uses: no calibration offsets are applied
and no IK limit is enforced, so a bad angle drives the joint into its hard
stop. The example stays within ±12° of centre with a 250 mA current cap on
purpose. Watch it move before you widen either.

`tau_ma` is a **ceiling, not a demand** — a lightly loaded joint draws only
what it needs to hold station.

## Gains are not a per-tick parameter

Unitree sends `Kp`/`Kd` with every joint command. Here they are config writes:
about a millisecond each, over a request/reply exchange that cannot interleave
with gait traffic. Set them once after `servo_lock()`, then stream commands.

The `kp`/`kd` fields in `mpx_servo_cmd_t` do exist in the AT32 wire frame, but
the stock board firmware ignores them. Leave them `0` unless your board
firmware is known to honour per-frame gains.

Gain writes land in the board's RAM and vanish on power-off, which is what you
want while experimenting. `servo_save_config(0)` commits all four boards.

See [`HOST_FUNCTIONS.md`](../../HOST_FUNCTIONS.md) for the full reference.
