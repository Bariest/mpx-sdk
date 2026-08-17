# trot-4leg — the worked example for physics simulation

A diagonal trot (FR+RL, then FL+RR) with inverse kinematics on all four legs.

```bash
# from the repo root, inside the dev container
python tools/mjsim.py examples/trot-4leg/build/trot_4leg.wasm
```

**First time only** — the container mounts this repo and nothing else, so a
model sitting at `C:\output_mjcf` on your host is invisible from inside it.
Copy it in once:

```bash
xcopy /E /I C:\output_mjcf\output_mjcf model     # Windows, before opening the container
cp -r /path/to/output_mjcf model/                 # or from inside
```

After that `mjsim.py` finds it on its own — no `--model` needed.

## It falls over — on purpose

```
DEFAULT MOTOR TUNING
❌ THE ROBOT FELL at 0.23s
   body height 113 mm → 57 mm

Did the joints actually get there?
  FR shoulder     2.1°      5.2°
  FR knee         1.6°      5.5°
  ...
```

This is the whole point of running physics. MPX Studio checks this same skill
kinematically and passes it: every angle is inside ±135°, nothing is NaN, it
terminates. **And it still falls over**, because a diagonal trot puts only two
feet on the ground and this one moves the body faster than the stance pair can
carry it.

Note the second half of the report: **mean tracking error is about 2°**, so the
servos *are* reaching the angles asked of them. The gait is wrong, not the
tuning. That distinction is exactly what a kinematic preview cannot make and a
physics run can.

## Fixing it, if you want the exercise

Any of these, in rough order of how much they help:

- slow the cycle down — `DT_MS` 12 → 20
- shorten the stride — `STRIDE` 22 → 14
- lower the body — `STAND_Z` -78 → -70
- switch to a **creep** gait: lift one leg at a time, so three are always down

Re-run `mjsim.py` after each. When it stops falling, put it on the robot.

## Control experiment

`examples/hello-world` and a plain stance both stay upright in the same harness
(115 mm, 0 mm travelled). If a change makes *standing* fall over, suspect the
model or the harness — not your gait.
