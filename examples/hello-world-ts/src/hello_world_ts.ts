/**
 * hello_world_ts.ts — the same first skill, in AssemblyScript.
 *
 *     cd examples/hello-world-ts
 *     mpx-cli deploy
 *
 * A skill is one exported function, on_start(), called once by the robot.
 * The helpers here come from mpx_env.ts, which `mpx-cli init --lang ts`
 * vendors into your project's include/ directory.
 */
import { robotGait, Gait, robot_delay_ms, print } from "../include/mpx_env";

function say(msg: string): void {
    // The host takes a pointer and a length. Encoding with `true` appends the
    // NUL byte the firmware's string handling expects.
    const buf = String.UTF8.encode(msg, true);
    print(changetype<usize>(buf), buf.byteLength - 1);
}

export function on_start(): void {
    say("hello from AssemblyScript");

    robotGait(Gait.ADVANCE);      // start walking — returns immediately
    robot_delay_ms(2000);         // the robot walks while we wait here
    robotGait(Gait.NONE);         // stop
    robot_delay_ms(300);

    robotGait(Gait.INIT);         // back to the standing pose
    robot_delay_ms(800);

    say("done");
}
