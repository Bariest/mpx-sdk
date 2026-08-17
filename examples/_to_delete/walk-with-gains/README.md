# walk-with-gains — tune Kp/Kd, then walk with them

Answers the obvious question: *can it walk using these gains?* Yes — but not
while your skill holds the servo bus.

```bash
cd examples/walk-with-gains
mpx-cli deploy
```

## The one thing to understand

`servo_lock()` **parks the gait.** That is what makes low-level control safe —
the gait task stops writing so your skill owns the bus. The cost is that the
built-in walk cannot run at the same time.

So this skill goes:

```
servo_lock()  →  write gains  →  servo_unlock()  →  robot_walk_forward()
```

The gains live on the driver boards, not in the skill. Once written they apply
to **everything** that moves the joints afterwards, including the built-in gait
and the app's joystick.

## What you'll see

1. gains written to all twelve joints
2. bus released, gait resumes
3. robot walks forward 3 s
4. turns left 1.5 s
5. settles back to the neutral stand

## What Kp and Kd actually do

The boards run a position loop underneath every command.

| | too low | too high |
|---|---|---|
| **Kp** stiffness | legs sag under load, walk looks soft and late | buzzing, overshoot, hot motors |
| **Kd** damping | oscillation after each step | sluggish, fights fast motion |

Raise Kp until the tracking is crisp, then raise Kd until the ringing stops.

The honest way to do this is the **Live scope in Servo Studio** (`/studio`):
select a joint, start the scope, step it with the Degrees slider, and watch the
overshoot on the Angle plot. Then put the numbers you liked into this skill.

## Gains are not saved by default

Writes land in the boards' RAM and vanish on power-off — deliberately, so a bad
experiment cannot brick your stance. Uncomment `servo_save_config(0)` in the
source to commit all four boards to flash.

## If you want the *skill* to generate the walk

That is a different thing: a real Unitree-style low-level gait, where the skill
computes foot trajectories, runs the IK itself, and streams `servo_write_all()`
at 50–100 Hz while holding the lock the whole time. It never releases the bus,
so it never uses the built-in gait at all.

That is buildable on this API — `servo_write_all` exists for exactly it — but it
drives the joints with no IK clamp behind it, so the first run needs the robot
held off the ground. See [`../lowlevel-servo`](../lowlevel-servo) for the
streaming pattern.
