/* dance_stanford_show.c
 *
 * MPX-Dog dance built on the FIRMWARE's Stanford-IK moves instead of the
 * raw servo pokes in moves.h.  Every move below runs through the exact
 * StanfordQuadruped inverse kinematics inside the robot (smooth ease-in,
 * hold, and ease-back-to-stand are all handled by the firmware), so the
 * dance looks like the reference Mini Pupper choreography.
 *
 * No sound / beat sync — pure movement timetable (the board has no audio).
 *
 * Requires the MPX-Dog firmware with the Stanford gait port (gait names:
 * stanford, frontkick, wiggle, buttshrug, wiggleL/R, buttshrugL/R,
 * lookup/lookdown/lookleft/lookright/lookul/lookur/lookll/looklr,
 * headellipse, bodycycle, bowback, flegL/R, blegL/R, heightup/heightdown,
 * balance, moveLF/RF/LB/RB, plus the legacy step/twerk/advance/...).
 *
 * Build / upload (same as the other songs):
 *     make SONG=dance_stanford_show build
 *     make SONG=dance_stanford_show upload MPX_HOST=<robot ip>
 *     make SONG=dance_stanford_show run    MPX_HOST=<robot ip>
 */
#include "mpx_host.h"

/* ── tempo ──────────────────────────────────────────────────────────
 * Period >= 150 for ALL period-based motion, as requested: the legacy
 * gaits (step, twerk, advance...) pace their phases with it, and the
 * Stanford wiggle/shrug wag cycle and head-ellipse sweep run at
 * period*8, so one big knob slows the whole show down.               */
#define SHOW_PERIOD     160     /* ms per gait phase  (>150)           */
#define SHOW_HEIGHT      70     /* mm — the calibrated stand           */
#define SHOW_UPHEIGHT    10
#define SHOW_STRIDE      10
#define SHOW_TILT        10

/* One wag / ellipse cycle in the firmware = period * 8 ms.           */
#define CYCLE_MS        (SHOW_PERIOD * 8)

/* ── tiny helpers ──────────────────────────────────────────────────── */
static void gait(const char *name) { robot_gait((int)name); }

/* Hold-style move (looks, wiggles, shrugs, lifts, balance, height):
 * firmware eases in (~0.35-0.45 s), holds while active, and eases back
 * to the stand (~0.3 s) when we send "none".                          */
static void hold_move(const char *name, int ms)
{
    gait(name);
    robot_delay_ms(ms);
    gait("none");
    robot_delay_ms(600);        /* covers the park-to-stand ramp */
}

/* Auto-return move (frontkick, headellipse, bodycycle, bowback):
 * fires once and comes back to the stand by itself — just wait it out. */
static void auto_move(const char *name, int ms)
{
    gait(name);
    robot_delay_ms(ms);
    robot_delay_ms(400);        /* settle in the stand */
}

/* ── the show ──────────────────────────────────────────────────────── */
void on_start(void)
{
    MPX_LOG("stanford show: start\n");

    /* slow tempo for the whole routine (period > 150 everywhere) */
    robot_set_config(SHOW_PERIOD, SHOW_HEIGHT, SHOW_UPHEIGHT,
                     SHOW_STRIDE, SHOW_TILT);

    /* settle into the calibrated stand */
    gait("init");
    robot_delay_ms(1500);

    /* 1 ── wake up: look around the room */
    hold_move("lookup",    1200);
    hold_move("lookdown",  1200);
    hold_move("lookleft",  1200);
    hold_move("lookright", 1200);
    hold_move("lookul",    1000);
    hold_move("looklr",    1000);

    /* 2 ── head draws its ellipse, then comes back on its own */
    auto_move("headellipse", 400 + CYCLE_MS + 400);

    /* 3 ── tail up and wag (Stanford wiggle), two full wag cycles */
    hold_move("wiggle", 450 + 2 * CYCLE_MS);

    /* one-sided wiggles, left then right */
    hold_move("wiggleL", 1400);
    hold_move("wiggleR", 1400);

    /* 4 ── nose up butt shrug, two wag cycles + the side versions */
    hold_move("buttshrug", 450 + 2 * CYCLE_MS);
    hold_move("buttshrugL", 1400);
    hold_move("buttshrugR", 1400);

    /* 5 ── rear up like a horse (auto-returns after ~1.8 s) */
    auto_move("frontkick", 2200);

    /* 6 ── body draws a circle, orientation fixed */
    auto_move("bodycycle", 350 + CYCLE_MS + 400);

    /* 7 ── polite bow (auto-returns) */
    auto_move("bowback", 2 * (SHOW_PERIOD * 4) + 600 + 500);

    /* 8 ── shake a paw */
    hold_move("flegR", 1600);

    /* 9 ── Stanford trot: a few steps forward, then shimmy back
     *      diagonally to roughly where we started                    */
    hold_move("stanford", 3000);
    hold_move("moveLB",   1500);
    hold_move("moveRB",   1500);

    /* 10 ── party: slow twerk, then step in place */
    hold_move("twerk", 2600);
    hold_move("step",  2600);

    /* 11 ── finale: one more kick and a bow */
    auto_move("frontkick", 2200);
    auto_move("bowback", 2 * (SHOW_PERIOD * 4) + 600 + 500);

    /* back to the calibrated stand */
    gait("init");
    robot_delay_ms(1200);
    gait("none");

    MPX_LOG("stanford show: done\n");
}
