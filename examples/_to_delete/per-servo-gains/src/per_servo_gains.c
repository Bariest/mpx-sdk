/* per_servo_gains.c — different gains on different joints.
 *
 * servo_set_gain() has always been per-servo: it takes an id. The
 * mpx_servo_set_all_gains() helper is only a convenience loop over all twelve,
 * not the real API. Anything you can do to one joint you can do to any subset.
 *
 * This matters physically. On a quadruped the three joints in a leg do
 * different jobs, so they rarely want the same tuning:
 *
 *   Abduction (1,4,7,10)  mostly holds the leg in plane. Light load, so a
 *                         softer Kp is fine and a stiff one just buzzes.
 *   Thigh     (2,5,8,11)  carries the robot's weight through the whole stride.
 *                         Wants the highest Kp of the three.
 *   Calf      (3,6,9,12)  takes the landing shock. Wants real Kd or it rings
 *                         on every footfall.
 *
 * The numbers below are a STARTING SHAPE, not a tuned set for your robot.
 * Find yours with the Live scope in Servo Studio, then paste them in.
 *
 *   cd examples/per-servo-gains
 *   mpx-cli deploy
 */
#include "mpx_host.h"

/* -1 in any field means "leave this one alone", so you can tune a single
 * parameter on a single joint without disturbing anything else. */
typedef struct {
    int   id;
    float kp;        /* MPX_PARAM_KP_POSITION */
    float kd;        /* MPX_PARAM_KD_POSITION */
    float kp_cur;    /* MPX_PARAM_KP_CURRENT  */
    float kff_cur;   /* MPX_PARAM_KFF_CURRENT */
} joint_tune_t;

static const joint_tune_t TUNE[] = {
    /*  id    kp     kd    kp_cur  kff_cur                        */
    {    1,  9.0f, 0.25f,  1.6f,  0.35f },   /* FR abduction      */
    {    2, 18.0f, 0.55f,  1.8f,  0.42f },   /* FR thigh — loaded */
    {    3, 14.0f, 0.70f,  1.8f,  0.42f },   /* FR calf — shock   */

    {    4,  9.0f, 0.25f,  1.6f,  0.35f },   /* FL abduction      */
    {    5, 18.0f, 0.55f,  1.8f,  0.42f },   /* FL thigh          */
    {    6, 14.0f, 0.70f,  1.8f,  0.42f },   /* FL calf           */

    {    7,  9.0f, 0.25f,  1.6f,  0.35f },   /* RR abduction      */
    {    8, 18.0f, 0.55f,  1.8f,  0.42f },   /* RR thigh          */
    {    9, 14.0f, 0.70f,  1.8f,  0.42f },   /* RR calf           */

    {   10,  9.0f, 0.25f,  1.6f,  0.35f },   /* RL abduction      */
    {   11, 18.0f, 0.55f,  1.8f,  0.42f },   /* RL thigh          */
    {   12, 14.0f, 0.70f,  1.8f,  0.42f },   /* RL calf           */
};
#define TUNE_COUNT ((int)(sizeof(TUNE) / sizeof(TUNE[0])))

/* Write one parameter unless it is marked "leave alone". Returns 1 on failure
 * so the caller can just sum the return values. */
static int put(int id, int param, float v)
{
    if (v < 0.0f) return 0;                       /* -1 = skip */
    if (servo_set_gain(id, param, v) == 0) return 0;
    MPX_LOG("  write failed: servo ");
    MPX_print_int(id);
    MPX_LOG(" param ");
    MPX_print_int(param);
    MPX_LOG("\n");
    return 1;
}

static int apply(const joint_tune_t *t)
{
    int bad = 0;
    bad += put(t->id, MPX_PARAM_KP_POSITION, t->kp);
    bad += put(t->id, MPX_PARAM_KD_POSITION, t->kd);
    bad += put(t->id, MPX_PARAM_KP_CURRENT,  t->kp_cur);
    bad += put(t->id, MPX_PARAM_KFF_CURRENT, t->kff_cur);
    return bad;
}

void on_start(void)
{
    /* The lock is REQUIRED for gain writes and the firmware enforces it:
     * servo_set_gain() returns -2 if you skip this. See the README for why. */
    if (servo_lock() != 0) {
        MPX_LOG("servo_lock refused - close Servo Studio and retry\n");
        return;
    }

    /* ── Case 1: a different tune per joint ────────────────────── */
    int bad = 0;
    for (int i = 0; i < TUNE_COUNT; ++i) {
        bad += apply(&TUNE[i]);
    }

    /* ── Case 2: touch ONE joint only ──────────────────────────────
     * Nothing above is special. To tune just servo 2 and leave the other
     * eleven exactly as they are, delete the loop and keep this: */
    /*
    servo_set_gain(2, MPX_PARAM_KP_POSITION, 22.0f);
    servo_set_gain(2, MPX_PARAM_KD_POSITION,  0.60f);
    */

    if (bad) {
        MPX_LOG("finished with ");
        MPX_print_int(bad);
        MPX_LOG(" failed writes\n");
    } else {
        MPX_LOG("all gains applied\n");
    }

    /* Read one back to prove it took — the boards are the source of truth,
     * not this file. */
    float kp = 0.0f;
    if (servo_get_gain(2, MPX_PARAM_KP_POSITION, &kp) == 0) {
        MPX_LOG("servo 2 kp_position now ");
        MPX_print_int((int)(kp * 100.0f));   /* centi-units: 1800 = 18.00 */
        MPX_LOG(" /100\n");
    }

    /* RAM only. Uncomment to make it survive a power cycle:
     *
     *     servo_save_config(0);      // 0 = all four boards
     */

    servo_unlock();
    MPX_LOG("done - gait resumed\n");
}
