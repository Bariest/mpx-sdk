# hello-world — your first MPX skill

The smallest skill that visibly does something: says hello, walks forward for
two seconds, stands back up.

```bash
cd examples/hello-world
mpx-cli deploy
```

That one command builds it, uploads it to the robot over Wi-Fi and runs it.
If your robot is not on `192.168.2.1`, either pass `--ip` once or put
`MPX_HOST=<your-robot-ip>` in a `.env` file and never type it again.

## What a skill is

A skill is a WebAssembly module with **one exported function, `on_start()`**.
The robot calls it once; when it returns, the skill is over. There is no event
loop to register and nothing to initialise.

The robot stops any skill after **60 seconds**, whatever it is doing.

## The three lines that matter

```c
robot_gait_enum(GAIT_ADVANCE);   // start walking — returns immediately
robot_delay_ms(2000);            // the robot walks while you wait here
robot_gait_enum(GAIT_NONE);      // stop
```

Gaits are asynchronous. `robot_gait_enum()` sets the robot going and returns
straight away, so `robot_delay_ms()` is how you decide *how long* something
happens for. Leave the robot in a safe pose before you return — whatever
position it finishes in is where it stays.

`robot_gait_enum()` takes a value from the `Gait` enum rather than a string, so
a typo is a compile error rather than a robot that silently does nothing.

## Where to go next

| Example | What it teaches |
|---|---|
| `walk-with-gains` | Tune joint stiffness (Kp/Kd), then walk with it |
| `per-servo-gains` | Give each joint its own gains |
| `lowlevel-servo` | Drive individual joints directly, Unitree-style |

`HOST_FUNCTIONS.md` in the repo root lists everything the robot exposes, in
C, AssemblyScript and WAT.
