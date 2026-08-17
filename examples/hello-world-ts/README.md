# hello-world-ts — your first MPX skill, in AssemblyScript

The same skill as `hello-world`, written in AssemblyScript (TypeScript) instead
of C. It compiles to a module that makes the *identical* host calls.

```bash
cd examples/hello-world-ts
mpx-cli deploy
```

## If you tried AssemblyScript before and it did nothing

That was a real bug in the SDK, not something you did. `mpx_env.ts` passed
`changetype<usize>(typedArray)` to the host, which yields the address of the
**view object**, not of its data — 32 bytes short. So the firmware read an
`ArrayBufferView` header as float data, and `robot_imu_read`, `servo_read`,
`servo_read_all`, `servo_write_all` and `servo_get_gain` all returned garbage
or crashed outright.

Gait names had a second, independent version of the same bug: they were an
`Array<u8>` of raw bytes with no NUL terminator, passed the same wrong way, and
compared on the robot with `strcmp()`. Every `robotGait()` call missed every
name — and because `robot_gait` is registered with a void signature, the
"unknown gait" error could not even be returned. A TypeScript skill compiled,
uploaded, reported success and did nothing at all.

Both are fixed: typed arrays use `.dataStart`, and gait names are plain strings
encoded with `String.UTF8.encode(name, true)`. The `Gait` enum also stopped at
37 while the firmware accepted 46 names, so eight gaits — including `STANFORD`,
`FRONTKICK` and `WIGGLE` — were unreachable from TypeScript. They work now.

## Notes for AssemblyScript specifically

- `changetype<usize>` is correct for an **ArrayBuffer** (the object *is* the
  data) and wrong for a **TypedArray** (use `.dataStart`). The distinction is
  invisible at compile time, which is what made this bug survive so long.
- Strings crossing to the host need `String.UTF8.encode(s, true)` — the `true`
  appends the NUL byte the firmware's `strcmp` requires.
