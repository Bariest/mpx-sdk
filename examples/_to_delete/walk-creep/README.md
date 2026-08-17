# walk-creep — a creep gait, and why it still falls

A statically stable creep: one leg swings at a time, three always on the floor.
It lasts far longer than `trot-4leg` (1.1 s vs 0.23 s) — and it still goes over.

```bash
python tools/mjsim.py examples/walk-creep/build/walk_creep.wasm
```

**I did not get this walking robustly, and the reason is worth more than a
tuned parameter set.**

## The measurement that ends the argument

With the hip joints at 0 — where every example here leaves them — the stance
footprint is **116 mm long and 47 mm wide**. Computing the centre-of-mass margin
against the support polygon:

| Feet on the ground | CoM margin |
|---|---|
| all four | **+23.5 mm** |
| RB or LB lifted | +3.0 mm |
| **RF or LF lifted** | **−3.0 mm** |

A negative margin means the centre of mass is **outside** the support triangle.
The robot is not falling because the gait is mistimed or the servos are too
soft — it is falling because, in that instant, there is nothing under it.
**No cycle time, stride or lift value can fix a negative number.**

That is also why a 32-point sweep found nothing that survived a ±1.5° jitter,
and why one configuration "worked" in an early sweep: it was passing through
−3 mm quickly enough to get away with it. That is luck, not stability.

## What actually fixes it

Two things, neither of which is tuning:

1. **Use the hip joints.** Servos 1, 4, 7, 10 are abduction, and every example
   in this repo leaves them at 0. Splaying them widens the 47 mm track, which is
   what the margin is short of. Splaying alone took the gait from 1/4 to 2/4
   perturbed trials surviving — real progress, not yet enough.
2. **Shift the body before lifting.** A proper creep moves the centre of mass
   *into* the next support triangle before the foot leaves the ground, rather
   than lifting and hoping. That is the missing half.

## What this example is good for

As shipped, it is an honest demonstration that:

- physics catches what kinematics cannot (MPX Studio passes this skill),
- the failure has a *number* attached, not a vibe,
- and the number tells you which knob is the wrong one to turn.

## One bug that was real, and is fixed

The first version used the Taylor series for `atan`:

```c
x - x³/3 + x⁵/5 - x⁷/7
```

which is off by up to **3.5°** near |x| = 1 — squarely inside a leg IK's
working range. It is now a minimax polynomial accurate to **0.0007°** for the
same arithmetic cost. Both this example and `trot-4leg` were rebuilt.

That error alone moved the fall from 3.64 s to 1.10 s. Worth knowing if you
write your own trig: a WASM skill has no libm, and the obvious series is not
good enough for kinematics.
