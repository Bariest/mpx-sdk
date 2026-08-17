/* tricks.c — MPX-Dog WASM skill: custom moves built from roll/pitch/yaw
 *            plus the attitude glide-speed API.
 *
 * Build:   mpx-cli build src/tricks.c
 * Upload:  mpx-cli upload src/tricks.wasm
 * Run:     mpx-cli run tricks.wasm
 *
 * Angle-only body API (each pose HOLDS until the next call):
 *   robot_roll(deg)   robot_pitch(deg)   robot_yaw(deg)
 *   robot_attitude(roll,pitch,yaw)        robot_reset_attitude()
 * Speed API (how fast poses glide to the target, degrees/second):
 *   robot_set_attitude_speed(dps)                 all 3 axes
 *   robot_set_attitude_speed_xyz(r,p,y)           per axis (0 = instant)
 * Clamps: roll +/-25, pitch +/-20, yaw +/-30.
 */

#include <string.h>
#include "mpx_host.h"

static void say(const char *s) { print((int)s, (int)strlen(s)); }

/* ── SHAKE HAND: offer a paw (lean + rear up) then pump it ──────── */
static void shake_hand(void)
{
    say("trick: shake hand\n");
    robot_set_attitude_speed(0);            /* instant snap for the pumps */
    robot_attitude(-12.0f, 15.0f, 0.0f);
    robot_delay_ms(700);
    for (int i = 0; i < 4; ++i) {
        robot_attitude(-12.0f, 18.0f, 0.0f);
        robot_delay_ms(250);
        robot_attitude(-12.0f,  8.0f, 0.0f);
        robot_delay_ms(250);
    }
    robot_reset_attitude();
    robot_delay_ms(600);
}

/* ── NOD "YES": gentle nose up/down ─────────────────────────────── */
static void nod_yes(void)
{
    say("trick: nod yes\n");
    robot_set_attitude_speed_xyz(0, 120, 0);   /* only pitch glides */
    for (int i = 0; i < 3; ++i) {
        robot_pitch( 15.0f); robot_delay_ms(450);
        robot_pitch(-15.0f); robot_delay_ms(450);
    }
    robot_set_attitude_speed(0);
    robot_reset_attitude();
    robot_delay_ms(500);
}

/* ── SHAKE "NO": smooth yaw sweep side to side ──────────────────── */
static void shake_no(void)
{
    say("trick: shake no\n");
    robot_set_attitude_speed_xyz(0, 0, 60);    /* only yaw glides, 60 deg/s */
    for (int i = 0; i < 3; ++i) {
        robot_yaw(-22.0f); robot_delay_ms(900);
        robot_yaw( 22.0f); robot_delay_ms(900);
    }
    robot_set_attitude_speed(0);
    robot_reset_attitude();
    robot_delay_ms(500);
}

/* ── PLAY BOW: nose/front down, hold ────────────────────────────── */
static void play_bow(void)
{
    say("trick: play bow\n");
    robot_set_attitude_speed(60);
    robot_pitch(-18.0f);
    robot_delay_ms(1500);
    robot_set_attitude_speed(0);
    robot_reset_attitude();
    robot_delay_ms(500);
}

/* ── DIP: slow dramatic lean — nose down + roll to the side ─────── */
static void dip(void)
{
    say("trick: dip\n");
    robot_set_attitude_speed(45);              /* slow, deliberate glide */
    robot_attitude(-15.0f, -18.0f, 0.0f);      /* lean left + nose down  */
    robot_delay_ms(1600);                      /* hold the dip           */
    robot_attitude(0.0f, 12.0f, 0.0f);         /* rise back up, nose high*/
    robot_delay_ms(900);
    robot_set_attitude_speed(0);
    robot_reset_attitude();
    robot_delay_ms(500);
}

/* ── HEADBANG: fast pitch oscillation, nose slamming up/down ────── */
static void headbang(void)
{
    say("trick: headbang\n");
    robot_set_attitude_speed_xyz(0, 50, 0);   /* fast pitch, others off */
    for (int i = 0; i < 6; ++i) {
        robot_pitch( 18.0f); robot_delay_ms(170);
        robot_pitch(-18.0f); robot_delay_ms(170);
    }
    robot_set_attitude_speed(0);
    robot_reset_attitude();
    robot_delay_ms(400);
}

/* ── SWAGGER: groovy side-to-side body roll ─────────────────────── */
static void swagger(void)
{
    say("trick: swagger\n");
    robot_set_attitude_speed_xyz(70, 0, 0);    /* only roll glides */
    for (int i = 0; i < 4; ++i) {
        robot_roll( 18.0f); robot_delay_ms(500);
        robot_roll(-18.0f); robot_delay_ms(500);
    }
    robot_set_attitude_speed(0);
    robot_reset_attitude();
    robot_delay_ms(500);
}

/* ── LOOK AROUND: sweep the head to each corner ─────────────────── */
static void look_around(void)
{
    say("trick: look around\n");
    robot_set_attitude_speed(50);
    robot_attitude(0.0f,  12.0f, -25.0f);
    robot_delay_ms(900);
    robot_attitude(0.0f,  12.0f,  25.0f);
    robot_delay_ms(900);
    robot_attitude(0.0f, -12.0f,   0.0f);
    robot_delay_ms(900);
    robot_set_attitude_speed(0);
    robot_reset_attitude();
    robot_delay_ms(500);
}

/* Entry point. */
void on_start(void)
{
    say("tricks skill starting\n");
    robot_stand();
    robot_delay_ms(300);

    dip();
    headbang();
    // shake_no();
    // swagger();
    // shake_hand();
    // nod_yes();
    // play_bow();
    // look_around();

    say("tricks done\n");
}
