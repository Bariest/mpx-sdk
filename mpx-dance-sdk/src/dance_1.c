/* dance_1.c — "Stanford Show"
 *
 * Same routine as dance_stanford_show, under a fresh name so an old
 * uploaded skill can't shadow it.
 *
 * IMPORTANT: needs the UPDATED MPX-Dog firmware (the build with the
 * Stanford-IK port).  On older firmware the names stanford / frontkick /
 * wiggle / buttshrug / wiggleL / wiggleR / buttshrugL / buttshrugR are
 * UNKNOWN — robot_gait() logs "unknown gait" and does nothing, which
 * makes the dance look frozen/weird.  Reflash MPX-Dog first.
 *
 * Build / upload:
 *     make SONG=dance_1 build
 *     make SONG=dance_1 upload MPX_HOST=<robot ip>
 */
#include "mpx_host.h"

#define SHOW_PERIOD     160     /* ms per gait phase (>150)            */
#define SHOW_HEIGHT      70
#define SHOW_UPHEIGHT    10
#define SHOW_STRIDE      10
#define SHOW_TILT        10
#define CYCLE_MS        (SHOW_PERIOD * 8)   /* one wag / ellipse cycle */

static void gait(const char *name) { robot_gait((int)name); }

static void hold_move(const char *name, int ms)
{
    gait(name);
    robot_delay_ms(ms);
    gait("none");
    robot_delay_ms(600);
}

static void auto_move(const char *name, int ms)
{
    gait(name);
    robot_delay_ms(ms);
    robot_delay_ms(400);
}

void on_start(void)
{
    MPX_LOG("dance_1: start\n");

    robot_set_config(SHOW_PERIOD, SHOW_HEIGHT, SHOW_UPHEIGHT,
                     SHOW_STRIDE, SHOW_TILT);

    gait("init");
    robot_delay_ms(1500);

    /* look around */
    hold_move("lookup",    1200);
    hold_move("lookdown",  1200);
    hold_move("lookleft",  1200);
    hold_move("lookright", 1200);
    hold_move("lookul",    1000);
    hold_move("looklr",    1000);

    /* head ellipse (auto-returns) */
    auto_move("headellipse", 400 + CYCLE_MS + 400);

    /* tail-up wiggle + side wiggles */
    hold_move("wiggle", 450 + 2 * CYCLE_MS);
    hold_move("wiggleL", 1400);
    hold_move("wiggleR", 1400);

    /* butt shrug + side shrugs */
    hold_move("buttshrug", 450 + 2 * CYCLE_MS);
    hold_move("buttshrugL", 1400);
    hold_move("buttshrugR", 1400);

    /* front kick, body circle, bow */
    auto_move("frontkick", 2200);
    auto_move("bodycycle", 350 + CYCLE_MS + 400);
    auto_move("bowback", 2 * (SHOW_PERIOD * 4) + 600 + 500);

    /* paw shake */
    hold_move("flegR", 1600);

    /* Stanford walk out, shimmy back */
    hold_move("stanford", 3000);
    hold_move("moveLB",   1500);
    hold_move("moveRB",   1500);

    /* party */
    hold_move("twerk", 2600);
    hold_move("step",  2600);

    /* finale */
    auto_move("frontkick", 2200);
    auto_move("bowback", 2 * (SHOW_PERIOD * 4) + 600 + 500);

    gait("init");
    robot_delay_ms(1200);
    gait("none");

    MPX_LOG("dance_1: done\n");
}
