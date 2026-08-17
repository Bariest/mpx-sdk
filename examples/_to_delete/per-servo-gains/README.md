# per-servo-gains — different gains on different joints

```bash
cd examples/per-servo-gains
mpx-cli deploy
```

## Yes, gains are per-servo — they always were

`servo_set_gain(id, param, value)` takes an **id**. `mpx_servo_set_all_gains()`
is only a convenience loop over all twelve; it is not the real API.

Tune one joint and leave the other eleven untouched:

```c
servo_lock();
servo_set_gain(2, MPX_PARAM_KP_POSITION, 22.0f);
servo_set_gain(2, MPX_PARAM_KD_POSITION,  0.60f);
servo_unlock();
```

Nothing else on the robot changes. Each servo stores its own copy of all ten
parameters on its driver board.

## Why you'd want different gains per joint

The three joints in a leg do different jobs:

| Joints | Role | Wants |
|---|---|---|
| 1, 4, 7, 10 — abduction | holds the leg in plane, light load | lower Kp; a stiff one just buzzes |
| 2, 5, 8, 11 — thigh | carries the robot's weight all stride | the highest Kp of the three |
| 3, 6, 9, 12 — calf | absorbs landing shock | real Kd, or it rings every footfall |

The table at the top of the source has that shape. **The numbers are a starting
point, not a tuned set for your robot** — find yours with the Live scope in
Servo Studio, then paste them in.

Set any field to `-1` to leave that one parameter alone, so you can adjust a
single gain on a single joint without disturbing the rest.

## Is the lock necessary?

**Yes, and the firmware enforces it** — `servo_set_gain()` returns `-2` if you
skip it. Not bureaucracy; the reason is physical.

A gain write is a **request/reply pair** on the SPI bus: the ESP32 sends a
config frame, and the AT32 clocks its answer out on the *next* transaction. The
gait task is meanwhile pushing servo frames at ~200 Hz. Without the lock the
gait's frame lands between your request and its reply, and you don't just lose
the answer — the pending config reply gets decoded as the next feedback frame.
A real 31.03 °C reading comes back as **1540.8 °C**. That is a documented
failure mode of this hardware, not a hypothetical.

There is a second reason even with the bus mutex in place: each config exchange
holds the bus for about a millisecond. Twenty-four writes is 25–50 ms of stalled
gait ticks. Do that mid-stride and the robot hitches, and changing joint
stiffness while a leg is loaded is a good way to drop it.

So: lock, write, unlock. It costs milliseconds and you can do it standing still.

## Do you even need a skill?

Often not. **Servo Studio** (`/studio`, or Settings → Servo Studio) does exactly
this — per-servo, with the values read back in the Actual column, and a
**Set All Servos** dialog when you do want the same value everywhere. Use a
skill when you want the tune to be repeatable, versioned, or bundled with a
behaviour.

## Persistence

Writes land in the boards' **RAM** and vanish on power-off — deliberately, so a
bad experiment cannot brick your stance. `servo_save_config(0)` commits all four
boards to flash; `servo_save_config(id)` commits just the board that servo lives
on. Servos 1–3 share a board, 4–6 the next, and so on.
