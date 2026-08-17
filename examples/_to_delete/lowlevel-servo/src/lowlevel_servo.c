/* lowlevel_servo.c — Unitree-style low-level joint control on the AT32 boards.
 *
 * Demonstrates the whole shape of the API:
 *   1. take the bus (parks the gait)
 *   2. probe which joints are alive
 *   3. set the control gains — slow config path, done once
 *   4. stream position commands — fast path, 4 SPI frames per update
 *   5. read state back and abort on an over-temperature joint
 *   6. release the bus
 *
 * SAFETY: q_deg here is the RAW AT32 angle, 0..270, 135 = mechanical centre.
 * No calibration offsets are applied and no IK limit is enforced, so a bad
 * angle drives the joint into its hard stop. This example stays within +/- 12
 * degrees of centre and caps current at 250 mA. Widen both only once you have
 * watched it move.
 *
 *   cd examples/lowlevel-servo
 *   mpx-cli deploy
 */
#include "mpx_host.h"

#define CENTRE_DEG   135.0f
#define SWING_DEG     12.0f    /* deliberately small — see the note above */
#define CURRENT_MA   250.0f    /* a ceiling, not a demand                 */
#define TEMP_LIMIT_C  60.0f
#define STEPS         60
#define STEP_MS       25

/* Cheap sine over one period, good enough for a demo sweep and far cheaper
 * than pulling in libm on a 128 KB WASM budget.
 *
 * Starts and ends at 0 on purpose: the robot is parked at centre before the
 * sweep begins, so a wave that started at its extreme would snap the thighs
 * several degrees on the very first command. */
static float wave(int step, int steps)
{
    float u = 4.0f * (float)step / (float)steps;   /* 0..4 over one period */
    float t;
    if (u <= 1.0f)      t = u;                     /*  0 ->  1 */
    else if (u <= 3.0f) t = 2.0f - u;              /*  1 -> -1 */
    else                t = u - 4.0f;              /* -1 ->  0 */
    return t * (1.0f - 0.25f * (t < 0 ? -t : t));  /* soften the corners */
}

void on_start(void)
{
    if (servo_lock() != 0) {
        MPX_LOG("servo_lock refused - Servo Studio has the bus\n");
        return;
    }

    /* ── Which joints are actually there? ──────────────────────── */
    int found = servo_scan();
    if (found <= 0) {
        MPX_LOG("no servos answered - check board power and CS wiring\n");
        servo_unlock();
        return;
    }

    /* ── Gains: config path, so set them once, not per tick ────── */
    int failed = mpx_servo_set_all_gains(12.5f, 0.35f);
    if (failed) {
        MPX_LOG("gain writes failed on ");
        MPX_print_int(failed);
        MPX_LOG(" joints\n");
    }
    servo_set_gain(2, MPX_PARAM_KP_CURRENT, 1.8f);
    servo_set_gain(2, MPX_PARAM_KFF_CURRENT, 0.42f);
    servo_set_gain(2, MPX_PARAM_MAX_PWM_DUTY, 0.85f);
    /* Not saved to flash: these stay in RAM and vanish on power-off, which is
     * what you want while experimenting. Call servo_save_config(0) to keep. */

    /* ── Settle at centre before moving ────────────────────────── */
    mpx_servo_all_centre(CURRENT_MA);
    robot_delay_ms(400);

    /* ── Stream commands ───────────────────────────────────────── */
    mpx_servo_cmd_t cmd[12];
    mpx_servo_state_t st[12];

    for (int step = 0; step < STEPS; ++step) {
        float q = CENTRE_DEG + SWING_DEG * wave(step, STEPS);

        for (int i = 0; i < 12; ++i) {
            /* Only the thigh joints (2, 5, 8, 11) move; the rest hold. */
            int id = i + 1;
            int is_thigh = (id % 3) == 2;
            cmd[i].q_deg  = is_thigh ? q : CENTRE_DEG;
            cmd[i].tau_ma = CURRENT_MA;
            cmd[i].kp = 0.0f;   /* 0 = use the board's stored gains */
            cmd[i].kd = 0.0f;
        }

        if (servo_write_all(cmd, 12) != 0) {
            MPX_LOG("a driver board stopped answering - aborting\n");
            break;
        }

        /* Feedback rides on the same frames the command went out on, so this
         * read costs nothing extra. */
        servo_read_all(st);
        for (int i = 0; i < 12; ++i) {
            /* NaN compares false, so an unknown temperature never trips this. */
            if (st[i].temp_c > TEMP_LIMIT_C) {
                MPX_LOG("servo ");
                MPX_print_int(i + 1);
                MPX_LOG(" too hot - cutting power\n");
                mpx_servo_all_off();
                servo_unlock();
                return;
            }
        }

        robot_delay_ms(STEP_MS);
    }

    /* ── Park gently, hand the bus back ────────────────────────── */
    mpx_servo_all_centre(CURRENT_MA);
    robot_delay_ms(300);
    servo_unlock();
    MPX_LOG("low-level demo done\n");
}
