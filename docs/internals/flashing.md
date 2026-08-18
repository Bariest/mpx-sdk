# Building and flashing the firmware

```bash
cd ../mangdang
idf.py build
idf.py -p COM<n> flash monitor
```

The SDK in this repo describes **ABI v3**. A robot on older firmware will load
a v3 module and then fail to instantiate it, because the module imports symbols
that firmware does not provide.

## What to check after flashing

| Check | Expect |
|---|---|
| `mpx-cli deploy examples/01-hello` | walks forward for two seconds, stands |
| `mpx-cli deploy examples/02-gaits` | the drive section curves, rather than turning in steps |
| `mpx-cli deploy examples/05-timeline` | a bow with a paw lift; `--param tempo=2` visibly speeds it up |
| `GET /v1/robot/status` | the same twelve calibration offsets as before |
| Servo Studio degree readout | unchanged |
| A skill calling `mpx_bus_stage()` without `mpx_bus_take()` | returns `-2` |
| `mpx_gain_set(1, MPX_PARAM_RANGE_POSITION_DEG, 300)` | returns `-4`, read-only |
| A skill that claims `MPX_OWN_FEET` then calls `mpx_gait()` | returns `-7` |
| A skill with no `mpx_take()` call at all | behaves exactly as it did on v2 |

The gait command path is unchanged, so **NVS calibration stays valid and
nothing needs recalibrating.**

## What v3 changed in the firmware

| File | Change |
|---|---|
| `sdk/wasm_host_functions.h` | `MPX_ABI_VERSION` → 3; `MPX_ERR_BUSY`; 15 new symbols; the control-domain enum |
| `sdk/wasm_host_functions.cc` | the 15 implementations; one `control_allows()` guard added to each of the seven existing write paths |
| `wasm/wasm_sandbox.h` / `.cc` | per-run clock, parameter table, the optional `on_stop` call, gait halt on watchdog kill |
| `network/http_server.cc` | `POST /v1/skills/run` accepts an optional `params` string |

Nothing was removed and no existing behaviour changed for a skill that does not
call the new functions. The arbitration guards are no-ops until something calls
`mpx_control_take()`.

## If a skill traps immediately after flashing

It was built against the old ABI. `mpx-cli deploy` again. Adding
`MPX_REQUIRE_ABI()` to the top of `on_start()` turns that into one clear log
line instead of an unexplained trap.
