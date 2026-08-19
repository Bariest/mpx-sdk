/* mpx/leg.h — feet and joints: the layer where you shape the motion yourself.
 *
 * Two ways to move a leg, and the difference matters:
 *
 *   mpx_foot_to(leg, x, splay, z)   You say where the FOOT goes; the firmware
 *                                   works out the three joint angles. Uses the
 *                                   same kinematics and the same calibration
 *                                   as the built-in gaits, so your poses sit
 *                                   in the same frame as theirs.
 *
 *   mpx_joint_to(joint, degrees)    You say what the JOINT does. Total
 *                                   freedom, and total responsibility for the
 *                                   trigonometry.
 *
 * Prefer feet. Reach for joints when you are writing your own kinematics or
 * doing something the leg model does not describe.
 *
 * ── ANGLES ─────────────────────────────────────────────────────────────────
 * Everything in this file is DEGREES, RELATIVE TO CENTRE, range +/-135.
 * Zero means centred, and the robot is calibrated so that all twelve joints
 * centred is exactly the standing pose. Positive and negative are symmetric
 * about that.
 *
 * mpx/bus.h speaks a different frame — absolute 0..270 with 135 at centre —
 * because that is what the driver boards use. Everything there is named
 * _abs_ so you cannot mix the two by accident. If you need to convert,
 * MPX_ABS_FROM_REL / MPX_REL_FROM_ABS in that file do it.
 *
 * ── FRAMES ─────────────────────────────────────────────────────────────────
 * Writes are buffered. Nothing moves until the frame is sent:
 *
 *     mpx_foot_to(MPX_FL, 0, 0, -78);
 *     mpx_foot_to(MPX_FR, 0, 0, -78);
 *     mpx_frame_send();                 <- one send, after all the writes
 *
 * One send per frame, not one per joint. Sending per joint puts each servo on
 * the bus in its own transaction and the legs arrive at different times, which
 * is exactly what a judder is.
 *
 * mpx_ticker_wait() (mpx/sys.h) sends for you, so in a timed loop
 * mpx_frame_send() does not appear at all.
 */
#ifndef MPX_LEG_H
#define MPX_LEG_H

#include "mpx/abi.h"
#include "mpx/geometry.h"
#include "mpx/sys.h"
#include "mpx/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 *  Naming
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    MPX_FR = 0,   /**< Front right. */
    MPX_FL = 1,   /**< Front left.  */
    MPX_RR = 2,   /**< Rear right.  */
    MPX_RL = 3,   /**< Rear left.   */
} mpx_leg_t;

/** The three joints in a leg, hip outwards. */
typedef enum {
    MPX_HIP      = 0,   /**< Rotates the leg sideways (abduction).   */
    MPX_SHOULDER = 1,   /**< Swings the leg fore and aft.            */
    MPX_KNEE     = 2,   /**< Bends the leg.                          */
} mpx_part_t;

/** The twelve joints. Values are the firmware's servo ids, 1-12. */
typedef enum {
    MPX_FR_HIP = 1,  MPX_FR_SHOULDER = 2,  MPX_FR_KNEE = 3,
    MPX_FL_HIP = 4,  MPX_FL_SHOULDER = 5,  MPX_FL_KNEE = 6,
    MPX_RR_HIP = 7,  MPX_RR_SHOULDER = 8,  MPX_RR_KNEE = 9,
    MPX_RL_HIP = 10, MPX_RL_SHOULDER = 11, MPX_RL_KNEE = 12,
} mpx_joint_t;

/** The joint at `part` on `leg` — so a leg can be a loop variable. */
static inline mpx_joint_t mpx_joint(mpx_leg_t leg, mpx_part_t part)
{
    return (mpx_joint_t)((int)leg * 3 + (int)part + 1);
}

/** Pass instead of a joint to mean "all twelve".
 *
 *  This replaces an entire family. There used to be mpx_gains_all,
 *  mpx_current_all, mpx_max_effort_all and mpx_bus_relax_all — four names, and
 *  four chances to find that the one you wanted did not exist. One sentinel
 *  covers all of them and every future one, and it reads at the call site as
 *  what it is:
 *
 *      mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KP_POSITION, 65.0f);
 *
 *  Zero, because joints are 1..12 and zero was never a joint. */
#define MPX_ALL_JOINTS ((mpx_joint_t)0)

/* Link lengths, joint limits and hip positions live in mpx/geometry.h, which
 * is generated from the firmware's own kinematics headers. Do not redefine
 * them here: a 4 mm error in a link length is invisible until the foot lands
 * somewhere you did not ask for.
 *
 * ONE CAVEAT, AND IT IS REAL. The firmware carries TWO leg models:
 *
 *   Stanford exact IK   stanford_kinematics.h, SK_L2 = 60 mm
 *                       -> the built-in gaits, and what geometry.h reports
 *   planar IK           robot.cc calculate_ik, L2 = 56 mm
 *                       -> what mpx_foot_to() below actually calls
 *
 * So MPX_CALF_MM is right for the walking gaits and 4 mm long for foot
 * placement. It matters in one place: reach. MPX_REACH_MM is 110, but the
 * planar IK straightens at 106 and clamps its cos() argument beyond that
 * rather than failing — the foot stops moving out while your numbers keep
 * growing. Stay inside ~100 mm from the hip and the difference never shows. */

/* ═══════════════════════════════════════════════════════════════════════════
 *  Feet — you place, the firmware solves
 *
 *      x      mm, forward positive, back negative
 *      splay  degrees, sideways swing at the hip
 *      z      mm, measured DOWN from the hip, so it is negative.
 *             MPX_STAND_Z_MM (-70) is standing; less negative is a crouch.
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Place one foot. z is UP-positive, so a foot below its hip is negative.
 *
 *  THE SIGN IS FLIPPED HERE, ON PURPOSE, AND THIS IS THE ONLY PLACE IT HAPPENS.
 *
 *  The firmware's planar IK (robot.cc, calculate_ik) takes z as DISTANCE DOWN,
 *  positive: its own idle loop calls front_right_ik(0, 0, +70) to stand. The
 *  SDK says z is up-positive everywhere else — geometry.h, motion.h, every
 *  example — so one of the two has to convert, and it is cheaper to do it at
 *  this boundary than to carry two conventions through four layers.
 *
 *  This used to pass z straight through, which meant every foot call asked for
 *  a pose 180 degrees from the one written. atan2(x, zd) flips by pi when zd
 *  goes negative, so mpx_feet_stand() at z = -70 resolved to servo2 = servo3 =
 *  -180 and then clamped at the joint limit. It did not error and it did not
 *  warn; the legs just went somewhere else.
 *
 *  mpx/abi.h's robot_ik_fr() and friends are the raw host imports and still
 *  speak the firmware's convention. That is what "raw" means there.
 */
typedef struct {
    float x;      /**< mm, forward positive.                       */
    float splay;  /**< degrees, sideways at the hip.               */
    float z;      /**< mm, UP positive, so below the hip is negative. */
} mpx_footpos_t;

/* Where this skill last put each foot.
 *
 * This is what lets mpx_foot_move() below take a speed without you handing it
 * a starting point. It is safe to keep here because a skill is exactly ONE
 * source file — mpx-cli compiles a single <slug>.c — so there is exactly one
 * copy of this, not one per translation unit.
 *
 * It starts at the standing pose because that is where a skill starts. If a
 * built-in gait has been walking the robot around since, call mpx_feet_to()
 * once to say where the feet are before asking for a move at a speed. */
static __attribute__((unused)) mpx_footpos_t mpx_foot_last_[4] = {
    { 0.0f, 0.0f, MPX_STAND_Z_MM }, { 0.0f, 0.0f, MPX_STAND_Z_MM },
    { 0.0f, 0.0f, MPX_STAND_Z_MM }, { 0.0f, 0.0f, MPX_STAND_Z_MM },
};

static inline int mpx_foot_to(mpx_leg_t leg, float x_mm, float splay_deg, float z_mm)
{
    mpx_foot_last_[leg].x     = x_mm;
    mpx_foot_last_[leg].splay = splay_deg;
    mpx_foot_last_[leg].z     = z_mm;
    return mpx_foot((int)leg, x_mm, splay_deg, -z_mm);
}

/** Put all four feet in the same place relative to their own hips. */
static inline int mpx_feet_to(float x_mm, float splay_deg, float z_mm)
{
    int rc, worst = MPX_OK;
    for (int l = 0; l < 4; ++l) {
        rc = mpx_foot_to((mpx_leg_t)l, x_mm, splay_deg, z_mm);   /* one sign flip, in one place */
        if (rc != MPX_OK) worst = rc;
    }
    return worst;
}

/** The neutral four-feet-planted stance. */
static inline int mpx_feet_stand(void)
{
    return mpx_feet_to(0.0f, 0.0f, MPX_STAND_Z_MM);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Joints — you solve, the firmware passes it on
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Set one joint, in degrees relative to centre. Clamped to the joint limit
 *  rather than refused: a pose that overshoots by a degree because of an
 *  easing curve should sit at the limit, not abandon the frame. */
static inline int mpx_joint_to(mpx_joint_t j, float deg)
{
    deg = mpx_clamp(deg, -MPX_JOINT_LIMIT_DEG, MPX_JOINT_LIMIT_DEG);
    return robot_set_servo_angle((int)j, (int)(deg * 100.0f));
}

/** Read one joint back, in degrees relative to centre.
 *
 *  THE reading. Same frame as mpx_joint_to(), so an error term across the two
 *  has the right sign and a control loop converges.
 *
 *  There used to be an mpx_joint_raw() next to this, returning the driver
 *  board's 0..1023 in the ABSOLUTE frame. Two functions named alike, one line
 *  apart, differing invisibly in FRAME — and a loop built on the wrong one
 *  drives away from its target instead of towards it. Three separate pages of
 *  documentation existed to warn people off it and nothing in the SDK ever
 *  called it. A footgun that needs three warnings is not a feature.
 *
 *  The capability is not lost: robot_read_position() is in mpx/abi.h, with the
 *  raw host imports, where reaching for it is a deliberate act. */
static inline float mpx_joint_at(mpx_joint_t j)
{
    return (float)robot_read_angle_cdeg((int)j) * 0.01f;
}

/* Looking for mpx_joint_speed()? There is no speed register on this robot, so
 * it never did anything and is gone. mpx_joint_move() and mpx_foot_move()
 * below are the real thing. mpx_body_speed() in mpx/robot.h is a genuine
 * deg/s limit: body attitude is slewed by the firmware's gait task, which is
 * not the servo bus.
 */


/* ═══════════════════════════════════════════════════════════════════════════
 *  Sending a frame
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Send every buffered write. One per frame. */
static inline int mpx_frame_send(void) { return robot_flush(); }

/** Send this frame and hold the rate. Returns MPX_ERR_CANCELLED if the skill
 *  was stopped, which is your cue to leave the loop. */
static inline int mpx_ticker_wait(mpx_ticker_t *t)
{
    int rc = mpx_frame_send();
    if (rc != MPX_OK) return rc;
    t->frame++;
    return mpx_sleep_to(t->start_ms + t->frame * t->period_ms);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Moving at a speed
 *
 *  There is no speed register on this robot. The SPI frame to the driver
 *  boards carries { mode, position, torque, kp, kd } and nothing else, so a
 *  joint always drives as hard as its position loop asks. That is why a plain
 *  mpx_joint_to() or mpx_foot_to() is ALWAYS full speed: it creates the whole
 *  error at once and the motor spends everything closing it.
 *
 *  A slower move is the same command fed in gradually. These two write that
 *  loop for you. They are the calls you already know with one more argument:
 *
 *      mpx_foot_to  (MPX_FR, 30.0f, 0.0f, -50.0f);          now
 *      mpx_foot_move(MPX_FR, 30.0f, 0.0f, -50.0f, 40.0f);   at 40 mm/s
 *
 *      mpx_joint_to  (MPX_FR_KNEE, -25.0f);                 now (then send)
 *      mpx_joint_move(MPX_FR_KNEE, -25.0f, 60.0f);          at 60 deg/s
 *
 *  They BLOCK until the move is done and send their own frames, so they do not
 *  belong inside a frame loop — they replace one. A speed of 0 means as fast
 *  as it goes, which is exactly the _to version.
 *
 *  For choreography — several waypoints, custom easing, four feet on their own
 *  paths — use mpx/motion.h, which works in times rather than speeds because
 *  that is what makes separate limbs land together.
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Milliseconds to cover `travel` at `per_s`, floored at one frame.
 *  0 or less means instant, and the callers apply the target in one frame. */
static inline int mpx_move_ms_(float travel, float per_s)
{
    int ms;
    if (per_s <= 0.0f) return 0;
    ms = (int)(travel / per_s * 1000.0f);
    return ms < 20 ? 20 : ms;
}

/** Move one foot to (x, splay, z) at `mm_s` millimetres per second.
 *
 *  Starts from wherever this skill last put that foot. Splay is degrees at the
 *  hip, so it is measured as the arc the foot actually sweeps — a 10 degree
 *  swing at standing height is 15 mm of travel, not "10" of anything. */
static inline int mpx_foot_move(mpx_leg_t leg, float x_mm, float splay_deg,
                                float z_mm, float mm_s)
{
    mpx_footpos_t a = mpx_foot_last_[leg];
    float dx  = x_mm - a.x;
    float dz  = z_mm - a.z;
    float rad = 0.5f * (mpx_abs(a.z) + mpx_abs(z_mm));
    float ds  = mpx_rad(splay_deg - a.splay) * rad;
    int   ms  = mpx_move_ms_(mpx_sqrt(dx * dx + dz * dz + ds * ds), mm_s);
    mpx_ticker_t t;

    if (ms == 0) {
        int rc = mpx_foot_to(leg, x_mm, splay_deg, z_mm);
        return rc != MPX_OK ? rc : mpx_frame_send();
    }

    t = mpx_ticker(50);
    for (;;) {
        unsigned e = mpx_ticker_elapsed(&t);
        float    f = e >= (unsigned)ms ? 1.0f : (float)e / (float)ms;
        float    k = mpx_ease(MPX_EASE_INOUT, f);
        int      rc;

        mpx_foot_to(leg, mpx_lerp(a.x, x_mm, k),
                         mpx_lerp(a.splay, splay_deg, k),
                         mpx_lerp(a.z, z_mm, k));
        rc = mpx_ticker_wait(&t);           /* sends the frame, then sleeps */
        if (rc != MPX_OK) return rc;
        if (mpx_ticker_elapsed(&t) >= (unsigned)ms) break;
    }
    mpx_foot_to(leg, x_mm, splay_deg, z_mm);   /* land exactly on it */
    return mpx_frame_send();
}

/** Move all four feet to the same place at `mm_s`, arriving together. */
static inline int mpx_feet_move(float x_mm, float splay_deg, float z_mm, float mm_s)
{
    float worst = 0.0f;
    int   l, ms;
    mpx_footpos_t a[4];
    mpx_ticker_t  t;

    for (l = 0; l < 4; ++l) {
        float dx, dz, rad, ds, d;
        a[l] = mpx_foot_last_[l];
        dx   = x_mm - a[l].x;
        dz   = z_mm - a[l].z;
        rad  = 0.5f * (mpx_abs(a[l].z) + mpx_abs(z_mm));
        ds   = mpx_rad(splay_deg - a[l].splay) * rad;
        d    = mpx_sqrt(dx * dx + dz * dz + ds * ds);
        if (d > worst) worst = d;           /* the furthest foot sets the clock */
    }
    ms = mpx_move_ms_(worst, mm_s);

    if (ms == 0) {
        int rc = mpx_feet_to(x_mm, splay_deg, z_mm);
        return rc != MPX_OK ? rc : mpx_frame_send();
    }

    t = mpx_ticker(50);
    for (;;) {
        unsigned e = mpx_ticker_elapsed(&t);
        float    f = e >= (unsigned)ms ? 1.0f : (float)e / (float)ms;
        float    k = mpx_ease(MPX_EASE_INOUT, f);
        int      rc;

        for (l = 0; l < 4; ++l)
            mpx_foot_to((mpx_leg_t)l, mpx_lerp(a[l].x, x_mm, k),
                                      mpx_lerp(a[l].splay, splay_deg, k),
                                      mpx_lerp(a[l].z, z_mm, k));
        rc = mpx_ticker_wait(&t);
        if (rc != MPX_OK) return rc;
        if (mpx_ticker_elapsed(&t) >= (unsigned)ms) break;
    }
    mpx_feet_to(x_mm, splay_deg, z_mm);
    return mpx_frame_send();
}

/** Move one joint to `deg` at `dps` degrees per second.
 *
 *  Starts from the MEASURED angle, so it is honest about where the joint
 *  really is — one bus read, which is nothing next to the move itself. */
static inline int mpx_joint_move(mpx_joint_t j, float deg, float dps)
{
    float from = mpx_joint_at(j);
    int   ms   = mpx_move_ms_(mpx_abs(deg - from), dps);
    mpx_ticker_t t;

    if (ms == 0) {
        int rc = mpx_joint_to(j, deg);
        return rc != MPX_OK ? rc : mpx_frame_send();
    }

    t = mpx_ticker(50);
    for (;;) {
        unsigned e = mpx_ticker_elapsed(&t);
        float    f = e >= (unsigned)ms ? 1.0f : (float)e / (float)ms;
        int      rc;

        mpx_joint_to(j, mpx_lerp(from, deg, mpx_ease(MPX_EASE_INOUT, f)));
        rc = mpx_ticker_wait(&t);
        if (rc != MPX_OK) return rc;
        if (mpx_ticker_elapsed(&t) >= (unsigned)ms) break;
    }
    mpx_joint_to(j, deg);
    return mpx_frame_send();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Telemetry
 *
 *  Every read is a round trip to a driver board — roughly a millisecond. Do
 *  not put twelve of them inside a 60 Hz loop; sample one joint per frame, or
 *  read between moves.
 * ═══════════════════════════════════════════════════════════════════════════ */

static inline int   mpx_joint_load    (mpx_joint_t j) { return robot_read_load((int)j); }
static inline int   mpx_joint_current (mpx_joint_t j) { return robot_read_current((int)j); }
static inline int   mpx_joint_moving  (mpx_joint_t j) { return robot_read_moving((int)j); }
static inline float mpx_joint_temp_c  (mpx_joint_t j) { return mpx_read_temperature_c((int)j); }
static inline float mpx_battery_v     (void)          { return (float)robot_read_voltage(1) * 0.1f; }

/** Model number if the joint answers, <= 0 if it does not. A quick way to
 *  find a servo that has come unplugged. */
static inline int mpx_joint_ping(mpx_joint_t j) { return robot_ping_servo((int)j); }

/* ═══════════════════════════════════════════════════════════════════════════
 *  Calibration
 *
 *  An offset shifts what "centred" means for one joint. It is persistent and
 *  it changes what every later angle command means — including the built-in
 *  gaits'. Change these deliberately, with the robot in front of you, and
 *  prefer the robot's Servo Studio page where you can watch the joint move.
 * ═══════════════════════════════════════════════════════════════════════════ */

static inline float mpx_offset(mpx_joint_t j)
{
    return (float)robot_get_offset((int)j) * 0.01f;
}

static inline int mpx_offset_set(mpx_joint_t j, float deg)
{
    return robot_set_offset((int)j, (int)(deg * 100.0f));
}

/** Clear every calibration offset. Recalibration follows. */
static inline int mpx_offsets_reset(void) { return mpx_reset_offsets(); }

/* ═══════════════════════════════════════════════════════════════════════════
 *  Two-link inverse kinematics, in your module
 *
 *  The firmware's IK is better in almost every case — it inherits the
 *  calibration and matches the built-in gaits. This is here for when you are
 *  learning what IK does, or building on top of it.
 *
 *  Returns MPX_OK, or MPX_ERR_ARG if the target was out of reach, in which
 *  case the angles returned are for the closest reachable point rather than
 *  garbage.
 * ═══════════════════════════════════════════════════════════════════════════ */

static inline int mpx_ik2(float x_mm, float z_mm,
                          float *shoulder_deg, float *knee_deg)
{
    float d2, d, knee, sh, s;
    int   reachable = 1;

    d2 = x_mm * x_mm + z_mm * z_mm;
    d  = mpx_sqrt(d2);

    if (d > MPX_REACH_MM - 1.0f) {          /* pull the target into reach */
        reachable = 0;
        s = (MPX_REACH_MM - 1.0f) / (d > 0.0f ? d : 1.0f);
        x_mm *= s; z_mm *= s;
        d2 = x_mm * x_mm + z_mm * z_mm;
        d  = mpx_sqrt(d2);
    }

    knee = mpx_acos((d2 - MPX_THIGH_MM * MPX_THIGH_MM - MPX_CALF_MM * MPX_CALF_MM)
                    / (2.0f * MPX_THIGH_MM * MPX_CALF_MM));
    sh   = mpx_atan2(x_mm, -z_mm)
         + mpx_acos((d2 + MPX_THIGH_MM * MPX_THIGH_MM - MPX_CALF_MM * MPX_CALF_MM)
                    / (2.0f * MPX_THIGH_MM * (d > 0.0f ? d : 1.0f)));

    if (shoulder_deg) *shoulder_deg = mpx_deg(sh)   - 45.0f;
    if (knee_deg)     *knee_deg     = mpx_deg(knee) - 90.0f;
    return reachable ? MPX_OK : MPX_ERR_ARG;
}

#ifdef __cplusplus
}
#endif
#endif /* MPX_LEG_H */
