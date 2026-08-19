/* ═══ LAYER 4 — MOTORS ════════════════════════════════════════════════════
 *
 * You own the motor's control loop: the target angle AND the gains that
 * decide how hard it fights to get there.
 *
 * This is the bottom. Drop here when stiffness is part of the motion —
 * landing softly, going compliant so a leg can be pushed, holding a pose
 * against a load. Nothing above this layer can express any of that.
 *
 * THREE THINGS TO KNOW BEFORE YOU RUN IT:
 *
 *   1. Taking the bus PARKS THE GAIT. You cannot walk while you hold it.
 *   2. Gains PERSIST on the driver boards after you let go — which is how a
 *      skill tunes the motors and then lets the firmware's own gait walk with
 *      that tuning. The sandbox restores them when your skill ends, so you do
 *      not have to; mpx_gain_save() is how you keep one on purpose.
 *   3. Some parameters are calibration, not gains, and are refused.
 *
 *     mpx-cli deploy examples/04-motors
 *
 *     Based on:  mpx/bus.h      (mpx_bus_take, mpx_gain_set, mpx_bus_move)
 *                mpx/params.h   (the driver-board slots, generated)
 */
#include "mpx.h"

MPX_EXPORT void on_start(void)
{
    MPX_REQUIRE_ABI();
    MPX_LOG("layer 4 — you own the motor");

    /* Servo Studio may hold the bus; a refusal is normal, not a failure. */
    if (mpx_bus_take() != MPX_OK) {
        MPX_LOG("could not take the bus — is Servo Studio open?");
        return;
    }

    /* ── A. Stiffness ────────────────────────────────────────────────────
     * Kp is how hard the motor pulls towards its target; Kd damps the
     * approach. Stock is Kp 65, Kd 800.
     *
     * ONE function sets every gain: mpx_gain_set(joint, PARAM, value).
     * MPX_ALL_JOINTS in the joint slot means all twelve. The parameter name
     * carries the meaning that a pile of wrapper functions used to —
     * MPX_PARAM_KP_CURRENT says which loop as well as which gain. */
    mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KP_POSITION, 65.0f);
    mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KD_POSITION, 800.0f);
    mpx_gain_set(MPX_FR_KNEE, MPX_PARAM_KP_POSITION, 95.0f);   /* one stiffer */

    float kp = 0.0f;
    if (mpx_gain_get(MPX_FR_KNEE, MPX_PARAM_KP_POSITION, &kp) == MPX_OK)
        mpx_log_f("FR knee Kp", kp);

    /* ── A2. The rest of the control loop ────────────────────────────────
     * Kp/Kd shape the POSITION loop — where the joint goes. Underneath it the
     * board runs a CURRENT loop, deciding how the motor makes the torque the
     * position loop asked for. Reach for these when a joint arrives in the
     * right place but arrives badly.
     *
     * READ THE SCALE BEFORE YOU TYPE A NUMBER HERE. The current loop is in
     * amps per count, not degrees. Stock is Kp 0.0006 and Kff 0.00022, and a
     * useful change is one step of 0.0001. The number 65 is correct for the
     * POSITION loop above and about 100,000x too large here — the joint sings,
     * gets hot, and you conclude the robot is broken. Scale from the stock
     * constants rather than typing an absolute value. */
    mpx_gain_set(MPX_FR_KNEE, MPX_PARAM_KP_CURRENT,  MPX_KP_CURRENT_STOCK  * 1.5f);
    mpx_gain_set(MPX_FR_KNEE, MPX_PARAM_KFF_CURRENT, MPX_KFF_CURRENT_STOCK * 1.2f);

    /* ...and the same on all twelve, when you want the whole robot: */
    mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KP_CURRENT, MPX_KP_CURRENT_STOCK);

    /* A torque CEILING rather than a gain, but the same one call reaches it.
     * Clamped to 0..1 inside mpx_gain_set, so the limit holds whatever writes it. */
    mpx_gain_set(MPX_FR_KNEE, MPX_PARAM_MAX_PWM_DUTY_CYCLE, 0.6f);

    /* Calibration, not gains. These decide what every angle MEANS, and
     * mpx_gain_save() would burn a mistake into the driver board's flash
     * where a reboot will not clear it. Refused from a skill on purpose —
     * change them from Servo Studio, watching the joint move. */
    if (mpx_gain_set(MPX_FR_KNEE, MPX_PARAM_RANGE_POSITION_DEG, 300.0f)
            == MPX_ERR_READONLY)
        MPX_LOG("calibration params are read-only from a skill, as intended");

    /* The two REVERSE_* slots flip a direction. A skill that writes one leaves
     * a single joint driving opposite to the other eleven — a robot tearing at
     * its own legs, surviving the skill that caused it. The firmware refuses
     * them for that reason. */
    if (mpx_gain_set(MPX_FR_KNEE, MPX_PARAM_REVERSE_MOTOR, 1.0f) == MPX_ERR_READONLY)
        MPX_LOG("reverse_motor is refused too — it is a direction, not a gain");

    /* You never have to find out the hard way. Ask first: */
    for (int i = 0; i < MPX_PARAM_COUNT; ++i)
        if (MPX_PARAMS[i].read_only)
            mpx_log_s(MPX_PARAMS[i].name);      /* the five a skill cannot write */

    /* ── B. Driving joints directly ──────────────────────────────────────
     * Same discipline as one layer up: stage what you want, send once.
     * mpx_bus_move() does both in one call for a single joint. Staging is
     * for FRAMES: several joints that should arrive together. Everything is
     * relative degrees, +/-135 with 0 at centre — the same convention as the
     * rest of the SDK, and now the only one. */
    for (int i = 0; i <= 60; ++i) {
        float deg = 12.0f * mpx_sind((float)i * 6.0f);

        mpx_bus_stage(MPX_FR_SHOULDER, deg);
        mpx_bus_stage(MPX_FR_KNEE,     0.0f);
        mpx_bus_send();                     /* one transaction for the frame */
        mpx_sleep(16);
    }

    /* ── C. Stiffness as part of the motion ──────────────────────────────
     * The thing only this layer can do: change how hard the joint fights,
     * mid-move. Per-frame kp/kd override the persistent gains — here the
     * knee goes soft as it arrives, so it settles instead of slamming. */
    for (int i = 0; i <= 40; ++i) {
        float t    = (float)i / 40.0f;
        float soft = 90.0f - 60.0f * t;     /* stiff -> compliant */

        /* _ex when the frame needs its own current cap or gains — here the
           knee goes soft as it arrives, so it settles instead of slamming. */
        mpx_bus_stage_ex(MPX_FR_KNEE, -18.0f * t, 400.0f, soft, 900.0f);
        mpx_bus_send();
        mpx_trace_f("kp", soft);
        mpx_sleep(20);
    }

    /* ── D. Put it back ──────────────────────────────────────────────────
     * Gains outlive your skill: leaving one joint at Kp 95 would make every
     * built-in gait afterwards walk slightly wrong with nothing on screen to
     * explain it. The sandbox restores whatever was there before this skill
     * ran, on every exit — clean, trapped, or watchdog-killed.
     *
     * Doing it here anyway is not wasted. It puts the robot back BEFORE the
     * mpx_stand() below, rather than after the skill ends, so the stand
     * happens at the gains you expect. */
    mpx_gains_stock();
    mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_MAX_PWM_DUTY_CYCLE, 1.0f);
    mpx_bus_release();

    mpx_stand();
    MPX_LOG("done");
}

MPX_EXPORT void on_stop(int reason)
{
    (void)reason;
    /* Nothing to undo by hand. The sandbox force-releases the bus so a crashed
     * skill cannot leave the gait parked, clears any overlay, and restores the
     * gains — the three pieces of state that outlive a module. Leaving the
     * robot somewhere sensible is still ours. */
    mpx_stand();
}
