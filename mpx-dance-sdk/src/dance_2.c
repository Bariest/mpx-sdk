/* dance_2.c — "Party Mix"
 *
 * A different routine from dance_1: more body work, spins, diagonal
 * shimmies, and a double-kick finale.  Same slow tempo (period 170).
 *
 * IMPORTANT: needs the UPDATED MPX-Dog firmware (Stanford-IK build) —
 * see the note in dance_1.c.
 *
 * Build / upload:
 *     make SONG=dance_2 build
 *     make SONG=dance_2 upload MPX_HOST=<robot ip>
 */
#include "mpx_host.h"

#define SHOW_PERIOD     170     /* ms per gait phase (>150)            */
#define SHOW_HEIGHT      70
#define SHOW_UPHEIGHT    10
#define SHOW_STRIDE      10
#define SHOW_TILT        10
#define CYCLE_MS        (SHOW_PERIOD * 8)

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
    MPX_LOG("dance_2: start\n");

    robot_set_config(SHOW_PERIOD, SHOW_HEIGHT, SHOW_UPHEIGHT,
                     SHOW_STRIDE, SHOW_TILT);

    gait("init");
    robot_delay_ms(1500);

    /* warm-up: balance pose, then body low / high */
    hold_move("balance",    1600);
    hold_move("heightdown", 1300);
    hold_move("heightup",   1300);

    /* body circle (auto-returns) */
    auto_move("bodycycle", 350 + CYCLE_MS + 400);

    /* paw play: front-left shake, rear-right lift */
    hold_move("flegL", 1400);
    hold_move("blegR", 1400);

    /* quick corner looks, like checking the crowd */
    hold_move("lookul", 900);
    hold_move("looklr", 900);
    hold_move("lookur", 900);
    hold_move("lookll", 900);

    /* one wag cycle each way */
    hold_move("wiggle",    450 + CYCLE_MS);
    hold_move("buttshrug", 450 + CYCLE_MS);

    /* spin a little: legacy turns run slow at period 170 */
    hold_move("turnL", 2000);
    hold_move("turnR", 2000);

    /* diagonal shimmy: forward-left, forward-right, then back home */
    hold_move("moveLF", 1500);
    hold_move("moveRF", 1500);
    hold_move("moveLB", 1500);
    hold_move("moveRB", 1500);

    /* Stanford trot a few steps */
    hold_move("stanford", 2500);

    /* head ellipse + twerk */
    auto_move("headellipse", 400 + CYCLE_MS + 400);
    hold_move("twerk", 2400);

    /* double-kick finale + bow */
    auto_move("frontkick", 2200);
    auto_move("frontkick", 2200);
    auto_move("bowback", 2 * (SHOW_PERIOD * 4) + 600 + 500);

    gait("init");
    robot_delay_ms(1200);
    gait("none");

    MPX_LOG("dance_2: done\n");
}
