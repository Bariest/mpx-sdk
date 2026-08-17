/* walk_with_gains.c — tune the servo gains, then walk with them.
 *
 * This is the answer to "can it walk using Kp/Kd?". Yes — but the two things
 * cannot happen at the same time, and the reason is worth understanding.
 *
 * servo_lock() PARKS THE GAIT. That is what makes low-level control safe: the
 * gait task stops writing to the bus so your skill has it to itself. The flip
 * side is that the built-in walk cannot run while you hold the lock.
 *
 * So the shape of this skill is:
 *
 *     lock  ->  write gains  ->  UNLOCK  ->  walk
 *
 * The gains live on the driver boards, not in this skill. Once written they
 * apply to everything that moves the joints afterwards, including the built-in
 * gait. Kp/Kd are the position loop the boards run underneath every command:
 *
 *   Kp  stiffness  — how hard the joint fights a position error.
 *                    Too low: the legs sag under the robot's weight and the
 *                    walk looks soft and late. Too high: buzzing, overshoot,
 *                    hot motors.
 *   Kd  damping    — how hard it fights velocity. Raise it to kill the
 *                    oscillation that comes with a high Kp.
 *
 * Tune with the Live scope in Servo Studio (/studio) — step the joint and
 * watch the overshoot — then put the numbers you liked in here.
 *
 *   cd examples/walk-with-gains
 *   mpx-cli deploy
 */
#include "mpx_host.h"

/* Start conservative. These are the stock-ish values; move one at a time. */
#define KP_POSITION   12.5f
#define KD_POSITION    0.35f

#define WALK_MS       3000     /* how long to walk forward */

void on_start(void)
{
    /* ── 1. Take the bus so we can write gains ─────────────────── */
    if (servo_lock() != 0) {
        MPX_LOG("servo_lock refused - close Servo Studio and retry\n");
        return;
    }

    int failed = mpx_servo_set_all_gains(KP_POSITION, KD_POSITION);
    if (failed) {
        MPX_LOG("gain writes failed on ");
        MPX_print_int(failed);
        MPX_LOG(" joints - walking anyway with whatever stuck\n");
    } else {
        MPX_LOG("gains set on all 12 joints\n");
    }

    /* Gains are in the boards' RAM and will vanish on power-off. That is what
     * you want while experimenting. Uncomment to make them permanent:
     *
     *     servo_save_config(0);      // 0 = all four boards
     */

    /* ── 2. HAND THE BUS BACK — the gait needs it ──────────────── */
    servo_unlock();

    /* Give the gait task a moment to pick the robot back up before we ask it
     * to walk; it resumes from the pose the legs are holding. */
    robot_delay_ms(300);

    /* ── 3. Walk. The servos are now tracking with the new gains ── */
    MPX_LOG("walking forward\n");
    robot_walk_forward(WALK_MS);     /* advance, wait, then stop */

    robot_delay_ms(500);
    MPX_LOG("turning\n");
    robot_turn_left(1500);

    robot_delay_ms(500);
    robot_gait_enum(GAIT_INIT);      /* settle back to the neutral stand */

    MPX_LOG("done\n");
}
