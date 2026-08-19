/* ═══ LAYER 2 — FEET ══════════════════════════════════════════════════════
 *
 * You say where a foot should be. The firmware solves the leg.
 *
 * Drop to this layer when you want poses the built-in gaits do not have, but
 * you do not want to own the trigonometry. It is the same inverse kinematics
 * the gait generator uses, so your poses sit in the same frame as the
 * built-in movements and inherit the same calibration.
 *
 *     mpx-cli deploy examples/02-feet
 *
 *     Based on:  mpx/leg.h       (mpx_foot_set, mpx_frame_send)
 *                mpx/geometry.h  (the robot's real dimensions)
 *                mpx/leg.h       (mpx_foot_move, mpx_feet_move — same calls,
 *                                 plus a speed)
 */
#include "mpx.h"

/* Coordinates, for one leg, relative to ITS OWN hip:
 *
 *        +x forward          x   mm, forward positive
 *          ↑                 splay  degrees, sideways swing at the hip
 *          │                 z   mm, UP positive — so a foot on the floor
 *     hip ─┼──→ +y left          is NEGATIVE. MPX_STAND_Z_MM is -70.
 *          │
 *          ↓ foot at z ≈ -70
 */

MPX_EXPORT void on_start(void)
{
    MPX_REQUIRE_ABI();
    MPX_LOG("layer 2 — you place the feet");

    const float STAND = MPX_STAND_Z_MM;     /* -70, from the firmware's own model */

    /* ── A. One pose ─────────────────────────────────────────────────────
     * All four feet in the same place relative to their own hips.
     *
     * NOTHING MOVES UNTIL mpx_frame_send(). mpx_feet_set() only writes the
     * twelve joint targets into a buffer; sending is a separate step so a
     * whole frame leaves as ONE bus transaction instead of twelve.
     *
     * This is not optional and it is not "cleaner if you do". While your skill
     * runs, the firmware's gait task keeps rewriting that same buffer with the
     * neutral stand every 15 ms. Set feet without sending and your numbers are
     * overwritten before they ever reach a motor — no error, no warning, a
     * robot that just stands there.
     *
     * mpx_foot_move()/mpx_feet_move() (section E) and mpx_ticker_wait() send
     * for you, which is why they do not appear next to those. */
    mpx_feet_set(0.0f, 0.0f, STAND);
    mpx_frame_send();               /* ← NOTHING MOVES UNTIL YOU SEND */
    mpx_sleep(600);

    /* ── B. Crouch and rise ──────────────────────────────────────────────
     * Less negative z is a crouch: the foot is closer to the hip. */
    for (int i = 0; i <= 40; ++i) {
        float z = STAND + 18.0f * mpx_sind((float)i * 4.5f);   /* 0..180 deg */
        mpx_feet_set(0.0f, 0.0f, z);
        mpx_frame_send();
        mpx_sleep(25);
    }

    /* ── C. Body sway, feet planted ──────────────────────────────────────
     * Moving all four feet backwards together moves the BODY forwards.
     * The robot rides over its feet without stepping. */
    for (int i = 0; i < 90; ++i) {
        float sway = 18.0f * mpx_sind((float)i * 4.0f);
        mpx_feet_set(sway, 0.0f, STAND);
        mpx_frame_send();
        mpx_sleep(20);
    }

    /* ── D. One leg at a time ────────────────────────────────────────────
     * Plant three, lift the fourth. Lift a leg the robot is standing on and
     * it will fall over — that is the responsibility this layer hands you. */
    mpx_feet_set(0.0f, 0.0f, STAND);
    mpx_frame_send();
    mpx_sleep(400);

    for (int i = 0; i <= 60; ++i) {
        float t     = (float)i / 60.0f;
        float lift  = 25.0f * mpx_sind(t * 180.0f);     /* up and back down  */
        float reach = 20.0f * mpx_sind(t * 360.0f);     /* forward and back  */
        mpx_foot_set(MPX_FR, reach, 0.0f, STAND + lift);
        mpx_frame_send();
        mpx_sleep(20);
    }

    /* ── E. The same moves, but you name the SPEED ───────────────────────
     * Everything above hand-wrote its own loop: pick a z, sleep 25 ms, pick
     * the next z. That IS speed on this robot. THERE IS NO SPEED REGISTER —
     * the driver boards take a position and a current cap and have no speed
     * field, so a joint always drives as hard as its position loop asks. A
     * plain mpx_foot_set() is therefore always full speed: it creates the whole
     * error at once. Slower means feeding the target in gradually.
     *
     * Which is what those loops were doing by hand — so the SDK does it for
     * you. Same call, one more argument. */
    mpx_feet_move(0.0f, 0.0f, STAND - 18.0f, 40.0f);   /* crouch at 40 mm/s */
    mpx_feet_move(0.0f, 0.0f, STAND,         40.0f);   /* and back up       */

    /* One foot, at a speed — no loop, no bookkeeping. It starts from wherever
     * this skill last put that foot, which the SDK remembers for you. */
    mpx_foot_move(MPX_FR, 30.0f, 0.0f, -50.0f, 40.0f);   /* reach out */
    mpx_foot_move(MPX_FR,  0.0f, 0.0f, STAND,  40.0f);   /* and back  */

    /* mm/s is real: 36 mm of travel at 40 mm/s takes about 900 ms, and the
     * SAME call with 80.0f takes half as long. Pass 0 to mean "as fast as it
     * goes", which is exactly mpx_foot_set().
     *
     * With four feet the speed applies to whichever travels FURTHEST and the
     * rest are slowed to match, so the pose lands in one piece. Per-foot speed
     * would finish four feet at four different moments.
     *
     * _move BLOCKS and sends its own frames, so it replaces a loop like the
     * ones above rather than going inside one. When you need several waypoints
     * or your own easing, mpx/motion.h works in times instead of speeds. */

    mpx_feet_stand();
    mpx_frame_send();
    MPX_LOG("done");
}

MPX_EXPORT void on_stop(int reason) { (void)reason; mpx_stand(); }
