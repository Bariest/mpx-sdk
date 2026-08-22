# 07 — Find your robot's real joint limits

`mpx/leg.h` clamps every joint you set to `MPX_HIP_LIMIT_DEG` (25),
`MPX_SHOULDER_LIMIT_DEG` (50) and `MPX_KNEE_LIMIT_DEG` (60). Those come from
the envelope the firmware's own gaits use — every built-in movement, `jump`
included, stays well inside them:

| foot height (z, from standing) | shoulder | knee |
|---|---|---|
| +30 (the jump crouch) | +23.6° | −15.0° |
| 0 (standing) | 0° | 0° |
| −30 (its tallest) | −24.5° | +20.3° |

That is evidence from the software, not a measurement of **your** chassis. A
printed part, a mounted sensor, or a cable routed differently moves the real
limit. This finds it.

**Put the robot on a stand.** The skill drives one joint until it touches
something. A robot standing on that leg will fall over.

```bash
mpx-cli deploy examples/07-limits --param joint=2
mpx-cli logs -f
```

It steps outward, pausing at each angle, walking positive first and then
negative — a leg is rarely symmetric, and the stop in one direction is usually
the body while the other is thin air.

It watches for you as well as logging: if `mpx_joint_at()` reports the joint
did not reach what it was asked for, something is in the way, and it stops and
says so. Trust your eyes first — `mpx-cli stop` the moment it touches.

Take the last angle it reached, subtract a few degrees of margin, and put it
above your include:

```c
#define MPX_SHOULDER_LIMIT_DEG 42.0f
#include "mpx.h"
```

| joint | ids | what stops it, usually |
|---|---|---|
| hip | 1, 4, 7, 10 | the leg meets the body |
| shoulder | 2, 5, 8, 11 | the leg meets the body, or the next leg |
| knee | 3, 6, 9, 12 | the calf folds onto the thigh |
