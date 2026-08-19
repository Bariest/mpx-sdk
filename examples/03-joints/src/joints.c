/* ═══ LAYER 3 — JOINTS ════════════════════════════════════════════════════
 *
 * You solve the leg. The firmware just carries the numbers to the servos.
 *
 * Drop to this layer when you need kinematics the built-in solver does not
 * do — a different leg model, a constraint of your own, a closed loop on
 * measured angles. You get exactly what you ask for, including a leg folded
 * into the body if that is what you asked for.
 *
 *     mpx-cli deploy examples/03-joints
 *
 *     Based on:  mpx/leg.h    (mpx_joint_to, mpx_frame_send)
 *                mpx/math.h   (there is no libm in the sandbox)
 *                mpx/leg.h    (mpx_joint_move — mpx_joint_to plus a speed)
 *                mpx/motion.h (poses, when joints must move together)
 */
#include "mpx.h"

/* Twelve joints. Degrees FROM CENTRE, +/-135.
 *
 *   MPX_FR MPX_FL MPX_RR MPX_RL          front/rear, right/left
 *   MPX_HIP MPX_SHOULDER MPX_KNEE        hip outwards
 *   MPX_FR_KNEE ...                      or name one directly
 *
 * Centre means something physical: all twelve at 0 IS the standing pose.
 */

MPX_EXPORT void on_start(void)
{
    MPX_REQUIRE_ABI();
    MPX_LOG("layer 3 — you solve the leg");

    /* Claiming the joints is optional, and worth doing here: it makes a fight
     * with the gait generator an error you can see rather than a robot that
     * twitches. MPX_ERR_BUSY means something else already has them. */
    if (mpx_take(MPX_OWN_JOINTS) != MPX_OK) {
        MPX_LOG("joints are busy — is another skill running?");
        return;
    }

    /* ── A. One joint, and the rule that catches everyone ────────────────
     * NOTHING MOVES until mpx_frame_send(). Set every joint you want for
     * this frame, then send ONCE. Sending per joint gives you a robot that
     * judders, because each send is a separate bus transaction. */
    mpx_joint_to(MPX_FR_SHOULDER, 20.0f);
    mpx_joint_to(MPX_FR_KNEE,    -25.0f);
    mpx_frame_send();                       /* ← once per frame */
    mpx_sleep(700);

    /* ── B. Your own inverse kinematics ──────────────────────────────────
     * A WASM skill has no libm, so mpx/math.h provides sin, sqrt, atan2 and
     * the rest. Use them. Rolling your own atan from the Taylor series is
     * off by up to 3.5 degrees inside a leg's working range.
     *
     * mpx_ik2() solves a two-link leg. The link lengths come from
     * mpx/geometry.h, generated from the firmware's own kinematics headers —
     * so they are the numbers the built-in gait uses, not an estimate. */
    for (int i = 0; i <= 80; ++i) {
        float t = (float)i / 80.0f;
        float x = 22.0f * mpx_sind(t * 360.0f);          /* fore and aft  */
        float z = MPX_STAND_Z_MM + 14.0f * mpx_sind(t * 180.0f);

        float shoulder, knee;
        mpx_ik2(x, z, &shoulder, &knee);                 /* -> degrees    */

        mpx_joint_to(MPX_FR_SHOULDER, shoulder);
        mpx_joint_to(MPX_FR_KNEE,     knee);
        mpx_frame_send();
        mpx_sleep(16);                                   /* ~60 fps       */
    }

    /* ── C. Closing a loop ───────────────────────────────────────────────
     * mpx_joint_at() reads the MEASURED angle in the SAME frame
     * mpx_joint_to() takes. That matters: the raw reading underneath runs
     * the opposite way, and a loop built on it diverges instead of
     * converging — silently, and at speed. */
    float target = -20.0f;
    for (int i = 0; i < 25; ++i) {
        float measured = mpx_joint_at(MPX_FR_KNEE);
        float error    = target - measured;

        mpx_joint_to(MPX_FR_KNEE, target + error * 0.25f);   /* gentle P term */
        mpx_frame_send();

        mpx_trace_f("error", error);        /* mpx-cli trace, to watch it settle */
        mpx_sleep(40);
    }

    /* ── D. Naming the SPEED instead of writing the loop ─────────────────
     * Look at what B and C did: nudge the target, send, sleep 16 ms, repeat.
     * That IS speed here. THERE IS NO SPEED REGISTER — the frame to the driver
     * boards carries { mode, position, torque, kp, kd } and nothing else, so a
     * joint always drives as hard as its position loop asks. A bare
     * mpx_joint_to() is always full speed: the whole error appears at once and
     * the motor spends everything closing it.
     *
     * mpx_joint_move() is mpx_joint_to() with the speed you want. It reads
     * where the joint actually is, works out the time, runs the loop, and
     * sends its own frames — so it replaces a loop, it does not go inside one. */
    mpx_joint_move(MPX_FR_KNEE, -25.0f, 60.0f);   /* 60 deg/s */
    mpx_joint_move(MPX_FR_KNEE,   0.0f, 60.0f);   /* back, same speed */

    mpx_joint_move(MPX_FR_SHOULDER, 20.0f, 15.0f);  /* slow and deliberate */
    mpx_joint_move(MPX_FR_SHOULDER,  0.0f,  0.0f);  /* 0 = as fast as it goes */

    /* WHEN THIS IS THE WRONG TOOL. Two mpx_joint_move() calls run one after
     * the other, so the shoulder finishes before the knee starts. When joints
     * must move TOGETHER — which is most of a leg — you want one motion over a
     * shared time, and that is mpx/motion.h:
     *
     *     mpx_pose_t here = mpx_pose_now();
     *     mpx_pose_t bent = mpx_pose_with(here, MPX_FR_KNEE, -25.0f);
     *     mpx_pose_glide(here, bent, 1200, MPX_EASE_INOUT);
     *
     * That is the whole rule. One thing moving: a speed. Several things that
     * must land together: a time. */

    mpx_release();
    mpx_stand();
    MPX_LOG("done");
}

MPX_EXPORT void on_stop(int reason)
{
    (void)reason;
    mpx_release();          /* harmless if we never took it */
    mpx_stand();
}
