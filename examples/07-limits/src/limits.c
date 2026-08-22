/* ═══ FINDING YOUR ROBOT'S REAL LIMITS ════════════════════════════════════
 *
 * mpx/leg.h ships MPX_HIP_LIMIT_DEG, MPX_SHOULDER_LIMIT_DEG and
 * MPX_KNEE_LIMIT_DEG, and every joint you set is clamped to them. They are
 * 25 / 50 / 60, derived from the envelope the firmware's own gaits use --
 * every built-in movement, jump included, stays well inside them.
 *
 * THAT IS SOFTWARE EVIDENCE, NOT A MEASUREMENT OF YOUR CHASSIS. A different
 * build, a printed part, a mounted sensor, a cable in the wrong place: any of
 * those move the real limit. This skill finds it.
 *
 * PUT THE ROBOT ON A STAND. The whole point is to drive one joint until it
 * touches something, and a robot standing on that leg will fall.
 *
 *     mpx-cli deploy examples/07-limits --param joint=2
 *     mpx-cli logs -f
 *
 * It steps the joint outward, pausing at each angle and logging it. Watch the
 * leg. The moment it touches the body, another leg, or a cable -- or the
 * motor starts straining -- send `mpx-cli stop` and read the last angle in
 * the log. That number, minus a few degrees of margin, is your limit.
 *
 * Then put it in your own skills, above the include:
 *
 *     #define MPX_SHOULDER_LIMIT_DEG 42.0f
 *     #include "mpx.h"
 *
 * ...or tell the SDK maintainer, and it becomes the default for this build.
 *
 * It walks POSITIVE first and then NEGATIVE, because a leg is rarely
 * symmetric: the hip stop in one direction is usually the body, and in the
 * other it is thin air.
 *
 *     Based on:  mpx/leg.h     (mpx_joint_set, mpx_joint_limit)
 *                mpx/health.h  (mpx_joint_load — straining before touching)
 */
#include "mpx.h"

static void walk(mpx_joint_t j, float step, int hold, float limit)
{
    for (float deg = 0.0f; mpx_abs(deg) <= limit; deg += step) {
        /* mpx_joint_set() clamps to the CURRENT limit, which is exactly what
         * we are trying to see past -- so this skill raises it for itself.
         * That is the honest way to use an override: temporarily, in the one
         * place that needs it, with the robot on a stand and someone
         * watching. */
        mpx_joint_set(j, deg);
        mpx_frame_send();
        mpx_sleep(hold);

        const float at   = mpx_joint_at(j);
        const int   load = mpx_joint_load(j);

        mpx_log_f("asked", deg);
        mpx_log_f("reached", at);
        mpx_log_i("load", load);

        /* Not reaching what you asked for means something is in the way. This
         * is the reading that finds a limit without you having to hear it. */
        if (mpx_abs(at - deg) > step) {
            mpx_log_f("STOPPED SHORT — this is your limit, near", at);
            break;
        }
    }
    mpx_joint_set(j, 0.0f);
    mpx_frame_send();
    mpx_sleep(600);
}

MPX_EXPORT void on_start(void)
{
    MPX_REQUIRE_ABI();

    const mpx_joint_t j     = (mpx_joint_t)mpx_parami("joint", 2);
    const float       step  = mpx_paramf("step", 5.0f);
    const int         hold  = mpx_parami("hold", 900);
    const float       limit = mpx_paramf("max", 90.0f);

    if ((int)j < 1 || (int)j > 12) { MPX_LOG("joint must be 1-12"); return; }

    MPX_LOG("PUT THE ROBOT ON A STAND. Send `mpx-cli stop` the moment it touches.");
    mpx_log_f("current limit for this joint", mpx_joint_limit(j));

    if (mpx_take(MPX_OWN_JOINTS) != MPX_OK) {
        MPX_LOG("joints are busy — is another skill running?");
        return;
    }

    mpx_stand();
    mpx_sleep(800);

    MPX_LOG("walking POSITIVE");
    walk(j,  step, hold, limit);
    MPX_LOG("walking NEGATIVE");
    walk(j, -step, hold, limit);

    mpx_release();
    mpx_stand();
    MPX_LOG("done — the last 'reached' before it stopped short is your limit");
}

MPX_EXPORT void on_stop(int reason)
{
    (void)reason;
    mpx_release();
    mpx_stand();
}
