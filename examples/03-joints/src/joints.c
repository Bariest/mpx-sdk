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
 *     Based on:  mpx/leg.h    (mpx_joint_set, mpx_frame_send)
 *                mpx/math.h   (there is no libm in the sandbox)
 *                mpx/leg.h    (mpx_joint_move — mpx_joint_set plus a speed)
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
    mpx_joint_set(MPX_FR_SHOULDER, 20.0f);
    mpx_joint_set(MPX_FR_KNEE,    -25.0f);
    mpx_frame_send();                       /* ← once per frame */
    mpx_sleep(700);

    /* ── B. Solving a leg yourself ───────────────────────────────────────
     * A WASM skill has no libm, so mpx/math.h provides sin, sqrt, atan2 and
     * the rest. Use them. Rolling your own atan from the Taylor series is off
     * by up to 3.5 degrees inside a leg's working range.
     *
     * THERE USED TO BE AN mpx_ik2() HERE AND IT WAS WRONG. It solved with the
     * Stanford calf (60 mm) and a hardcoded centring, while mpx_foot_set()
     * goes through the firmware's planar IK (56 mm calf, neutral computed from
     * NEUTRAL_Z, per-leg sign flips). Side by side they disagree by 12 degrees
     * at the standing pose and 74 degrees at reach — and the leg still moved,
     * so nothing announced it.
     *
     * If you want the joint angles for a foot position, do not model the leg.
     * ASK THE ROBOT: place the foot, then read the joints back. Whatever the
     * firmware does, this agrees with it, forever. */
    mpx_foot_set(MPX_FR, 20.0f, 0.0f, MPX_STAND_Z_MM + 14.0f);
    mpx_frame_send();
    mpx_sleep(300);

    const float sh_lift = mpx_joint_at(MPX_FR_SHOULDER);
    const float kn_lift = mpx_joint_at(MPX_FR_KNEE);
    mpx_log_f("shoulder at lift", sh_lift);
    mpx_log_f("knee at lift",     kn_lift);

    /* Now they are just numbers, and layer 3 can interpolate them however it
     * likes — including in ways layer 2 cannot express. */
    for (int i = 0; i <= 80; ++i) {
        float t = (float)i / 80.0f;
        float k = mpx_ease(MPX_EASE_INOUT, mpx_sind(t * 180.0f));

        mpx_joint_set(MPX_FR_SHOULDER, sh_lift * k);
        mpx_joint_set(MPX_FR_KNEE,     kn_lift * k);
        mpx_frame_send();
        mpx_sleep(16);                                   /* ~60 fps       */
    }

    /* ── C. Closing a loop ───────────────────────────────────────────────
     * mpx_joint_at() reads the MEASURED angle in the SAME frame
     * mpx_joint_set() takes. That matters: the raw reading underneath runs
     * the opposite way, and a loop built on it diverges instead of
     * converging — silently, and at speed. */
    float target = -20.0f;
    for (int i = 0; i < 25; ++i) {
        float measured = mpx_joint_at(MPX_FR_KNEE);
        float error    = target - measured;

        mpx_joint_set(MPX_FR_KNEE, target + error * 0.25f);   /* gentle P term */
        mpx_frame_send();

        mpx_trace_f("error", error);        /* mpx-cli trace, to watch it settle */
        mpx_sleep(40);
    }

    /* ── D. Naming the SPEED instead of writing the loop ─────────────────
     * Look at what B and C did: nudge the target, send, sleep 16 ms, repeat.
     * That IS speed here. THERE IS NO SPEED REGISTER — the frame to the driver
     * boards carries { mode, position, torque, kp, kd } and nothing else, so a
     * joint always drives as hard as its position loop asks. A bare
     * mpx_joint_set() is always full speed: the whole error appears at once and
     * the motor spends everything closing it.
     *
     * mpx_joint_move() is mpx_joint_set() with the speed you want. It reads
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
