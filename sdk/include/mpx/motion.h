/* mpx/motion.h — authoring a movement.
 *
 * The other headers let you command the robot. This one lets you compose a
 * movement out of poses and time, which is what most people actually came
 * here to do.
 *
 * You describe a few key moments — where the feet are at 0 ms, at 600 ms, at
 * 1400 ms — and how to travel between them. Everything in between is
 * interpolated, sent at a steady frame rate, and cancelled cleanly if the
 * skill is stopped. You never write the frame loop.
 *
 *     mpx_stance_key_t bow[] = {
 *         { 0,    mpx_stance_stand(),                    MPX_EASE_LINEAR },
 *         { 700,  mpx_stance_front(20, -55),             MPX_EASE_INOUT  },
 *         { 1600, mpx_stance_stand(),                    MPX_EASE_OUT    },
 *     };
 *     mpx_stance_play(bow, 3, mpx_play(50, 1));
 *
 * ── STANCE OR POSE ─────────────────────────────────────────────────────────
 * Two kinds of keyframe, and the choice matters more than it looks:
 *
 *   mpx_stance_t   where the four FEET are. Goes through the firmware's
 *                  kinematics, so it inherits the calibration and lands in the
 *                  same frame as the built-in gaits. Interpolating feet gives
 *                  you straight-line foot paths, which is what looks natural.
 *                  START HERE.
 *
 *   mpx_pose_t     what the twelve JOINTS are doing. Interpolating joints
 *                  gives you arcs at the foot, which is right for a wave or a
 *                  stretch and wrong for anything that has to stay on the
 *                  floor.
 *
 * ── EASING ─────────────────────────────────────────────────────────────────
 * The `ease` on a key describes how you ARRIVE at it from the previous one.
 * The first key's ease is unused. Linear everywhere reads as mechanical;
 * MPX_EASE_INOUT on the middle keys and MPX_EASE_OUT on the last is a good
 * default and most of what makes motion look deliberate.
 */
#ifndef MPX_MOTION_H
#define MPX_MOTION_H

#include "mpx/sys.h"
#include "mpx/math.h"
#include "mpx/leg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 *  Stances — where the feet are
 * ═══════════════════════════════════════════════════════════════════════════ */

/* mpx_footpos_t lives in mpx/leg.h, next to the feet it describes. */

typedef struct {
    mpx_footpos_t leg[4];   /**< Indexed by mpx_leg_t: FR, FL, RR, RL. */
} mpx_stance_t;

/** All four feet in the same place relative to their own hips. */
static inline mpx_stance_t mpx_stance_all(float x, float splay, float z)
{
    mpx_stance_t s;
    for (int i = 0; i < 4; ++i) { s.leg[i].x = x; s.leg[i].splay = splay; s.leg[i].z = z; }
    return s;
}

/** The neutral standing stance. */
static inline mpx_stance_t mpx_stance_stand(void)
{
    return mpx_stance_all(0.0f, 0.0f, MPX_STAND_Z_MM);
}


/* mpx_stance_crouch(d) and mpx_stance_front(reach, z) were here. Both were
 * mpx_stance_all()/mpx_stance_with() with the numbers hidden behind a name:
 *
 *     mpx_stance_crouch(18.0f)
 *       -> mpx_stance_all(0.0f, 0.0f, MPX_STAND_Z_MM + 18.0f)
 *     mpx_stance_front(22.0f, -52.0f)
 *       -> s = mpx_stance_stand();
 *          s = mpx_stance_with(s, MPX_FR, 22.0f, 0.0f, -52.0f);
 *          s = mpx_stance_with(s, MPX_FL, 22.0f, 0.0f, -52.0f);
 *
 * Spelled out, the call site says which feet moved and by how much. The name
 * never did. */

/** Lift one foot to `lift_mm` above the standing height. */
static inline mpx_stance_t mpx_stance_lift(mpx_stance_t s, mpx_leg_t leg,
                                           float lift_mm, float forward_mm)
{
    s.leg[leg].z += lift_mm;
    s.leg[leg].x += forward_mm;
    return s;
}

/** One foot placed absolutely, the other three untouched.
 *
 *  For BUILDING a stance to put in a timeline. To simply move one foot at a
 *  speed, you want mpx_foot_move() in mpx/leg.h — it needs none of this.
 *  mpx_stance_lift() is the relative version of the same idea. */
static inline mpx_stance_t mpx_stance_with(mpx_stance_t s, mpx_leg_t leg,
                                           float x_mm, float splay_deg, float z_mm)
{
    s.leg[leg].x     = x_mm;
    s.leg[leg].splay = splay_deg;
    s.leg[leg].z     = z_mm;
    return s;
}

/** Interpolate between two stances. t is 0..1; ease shapes the travel. */
static inline mpx_stance_t mpx_stance_lerp(mpx_stance_t a, mpx_stance_t b,
                                           float t, mpx_ease_t e)
{
    mpx_stance_t o;
    float k = mpx_ease(e, t);
    for (int i = 0; i < 4; ++i) {
        o.leg[i].x     = mpx_lerp(a.leg[i].x,     b.leg[i].x,     k);
        o.leg[i].splay = mpx_lerp(a.leg[i].splay, b.leg[i].splay, k);
        o.leg[i].z     = mpx_lerp(a.leg[i].z,     b.leg[i].z,     k);
    }
    return o;
}

/** Command a stance. Does NOT send the frame — call mpx_frame_send(), or let
 *  mpx_ticker_wait() do it. */
static inline int mpx_stance_set(mpx_stance_t s)
{
    int worst = MPX_OK;
    for (int i = 0; i < 4; ++i) {
        int rc = mpx_foot_set((mpx_leg_t)i, s.leg[i].x, s.leg[i].splay, s.leg[i].z);
        if (rc != MPX_OK) worst = rc;
    }
    return worst;
}

/** Command a stance and send it immediately. */
static inline int mpx_stance_apply(mpx_stance_t s)
{
    int rc = mpx_stance_set(s);
    if (rc != MPX_OK) return rc;
    return mpx_frame_send();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Poses — what the joints are doing
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Twelve joint angles in degrees relative to centre. Index with a
 *  mpx_joint_t minus one, or use mpx_pose_set_joint(). */
typedef struct { float deg[12]; } mpx_pose_t;

/** Every joint centred — which is, by calibration, exactly standing. */
static inline mpx_pose_t mpx_pose_stand(void)
{
    mpx_pose_t p;
    for (int i = 0; i < 12; ++i) p.deg[i] = 0.0f;
    return p;
}

static inline float mpx_pose_get(mpx_pose_t p, mpx_joint_t j)
{
    return p.deg[(int)j - 1];
}

static inline mpx_pose_t mpx_pose_with(mpx_pose_t p, mpx_joint_t j, float deg)
{
    p.deg[(int)j - 1] = deg;
    return p;
}

/** The pose the robot is in right now, read back from the joints. Twelve bus
 *  reads — fine between moves, far too slow inside a frame loop. */
static inline mpx_pose_t mpx_pose_now(void)
{
    mpx_pose_t p;
    for (int i = 1; i <= 12; ++i) p.deg[i - 1] = mpx_joint_at((mpx_joint_t)i);
    return p;
}

static inline mpx_pose_t mpx_pose_lerp(mpx_pose_t a, mpx_pose_t b,
                                       float t, mpx_ease_t e)
{
    mpx_pose_t o;
    float k = mpx_ease(e, t);
    for (int i = 0; i < 12; ++i) o.deg[i] = mpx_lerp(a.deg[i], b.deg[i], k);
    return o;
}

/** Command a pose. Does NOT send the frame. */
static inline int mpx_pose_set(mpx_pose_t p)
{
    int worst = MPX_OK;
    for (int i = 1; i <= 12; ++i) {
        int rc = mpx_joint_set((mpx_joint_t)i, p.deg[i - 1]);
        if (rc != MPX_OK) worst = rc;
    }
    return worst;
}

/** Command a pose and send it immediately. */
static inline int mpx_pose_apply(mpx_pose_t p)
{
    int rc = mpx_pose_set(p);
    if (rc != MPX_OK) return rc;
    return mpx_frame_send();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Timelines
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    unsigned     t_ms;    /**< When this key happens, from the start.       */
    mpx_stance_t stance;
    mpx_ease_t   ease;    /**< How to ARRIVE here from the previous key.    */
} mpx_stance_key_t;

typedef struct {
    unsigned    t_ms;
    mpx_pose_t  pose;
    mpx_ease_t  ease;
} mpx_pose_key_t;

typedef struct {
    int rate_hz;   /**< Frames per second. 50 is smooth; above ~80 the servo
                    *   bus is the limit, not you.                          */
    int loops;     /**< How many times through. 0 means until cancelled.    */
} mpx_play_t;

static inline mpx_play_t mpx_play(int rate_hz, int loops)
{
    mpx_play_t o; o.rate_hz = rate_hz; o.loops = loops; return o;
}

/* Which segment of the timeline is `t_ms` in, and how far through it?
 * Shared by both play functions. */
static inline int mpx_tl_seg_(const unsigned *times, int n, unsigned t_ms, float *frac)
{
    int i;
    for (i = 0; i < n - 1; ++i) {
        if (t_ms < times[i + 1]) {
            unsigned span = times[i + 1] - times[i];
            *frac = span ? (float)(t_ms - times[i]) / (float)span : 1.0f;
            return i;
        }
    }
    *frac = 1.0f;
    return n - 2;
}

/**
 * Play a stance timeline.
 *
 * Blocks for the length of the timeline times `loops`. Returns MPX_OK, or
 * MPX_ERR_CANCELLED if the skill was stopped part-way — in which case the
 * robot is left wherever the last frame put it, so handle that return.
 *
 * Keys must be in ascending t_ms order and the first should be at 0. Fewer
 * than two keys does nothing and returns MPX_ERR_ARG.
 */
static inline int mpx_stance_play(const mpx_stance_key_t *keys, int n, mpx_play_t opt)
{
    unsigned times[32];
    unsigned total;
    int      loop, i;

    if (!keys || n < 2 || n > 32) return MPX_ERR_ARG;
    for (i = 0; i < n; ++i) times[i] = keys[i].t_ms;
    total = times[n - 1];
    if (total == 0) return MPX_ERR_ARG;

    for (loop = 0; opt.loops <= 0 || loop < opt.loops; ++loop) {
        mpx_ticker_t tk = mpx_ticker(opt.rate_hz > 0 ? opt.rate_hz : 50);
        for (;;) {
            unsigned t = mpx_ticker_elapsed(&tk);
            float    f;
            int      seg;
            int      rc;

            if (t > total) t = total;
            seg = mpx_tl_seg_(times, n, t, &f);
            rc = mpx_stance_set(mpx_stance_lerp(keys[seg].stance,
                                                keys[seg + 1].stance,
                                                f, keys[seg + 1].ease));
            if (rc != MPX_OK && rc != MPX_ERR_BUSY) return rc;

            rc = mpx_ticker_wait(&tk);
            if (rc != MPX_OK) return rc;

            if (mpx_ticker_elapsed(&tk) >= total) break;
        }
        /* Land exactly on the final key rather than wherever the last frame
         * boundary fell — a 50 Hz ticker can otherwise stop up to 20 ms short. */
        {
            int rc = mpx_stance_apply(keys[n - 1].stance);
            if (rc != MPX_OK && rc != MPX_ERR_BUSY) return rc;
        }
    }
    return MPX_OK;
}

/** Play a joint-pose timeline. Same contract as mpx_stance_play(). */
static inline int mpx_pose_play(const mpx_pose_key_t *keys, int n, mpx_play_t opt)
{
    unsigned times[32];
    unsigned total;
    int      loop, i;

    if (!keys || n < 2 || n > 32) return MPX_ERR_ARG;
    for (i = 0; i < n; ++i) times[i] = keys[i].t_ms;
    total = times[n - 1];
    if (total == 0) return MPX_ERR_ARG;

    for (loop = 0; opt.loops <= 0 || loop < opt.loops; ++loop) {
        mpx_ticker_t tk = mpx_ticker(opt.rate_hz > 0 ? opt.rate_hz : 50);
        for (;;) {
            unsigned t = mpx_ticker_elapsed(&tk);
            float    f;
            int      seg, rc;

            if (t > total) t = total;
            seg = mpx_tl_seg_(times, n, t, &f);
            rc = mpx_pose_set(mpx_pose_lerp(keys[seg].pose, keys[seg + 1].pose,
                                            f, keys[seg + 1].ease));
            if (rc != MPX_OK && rc != MPX_ERR_BUSY) return rc;

            rc = mpx_ticker_wait(&tk);
            if (rc != MPX_OK) return rc;

            if (mpx_ticker_elapsed(&tk) >= total) break;
        }
        {
            int rc = mpx_pose_apply(keys[n - 1].pose);
            if (rc != MPX_OK && rc != MPX_ERR_BUSY) return rc;
        }
    }
    return MPX_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  One-liners
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Travel smoothly from wherever the feet are to `to`, over `ms`. */
static inline int mpx_stance_glide(mpx_stance_t from, mpx_stance_t to,
                                   int ms, mpx_ease_t e)
{
    mpx_stance_key_t k[2];
    k[0].t_ms = 0;             k[0].stance = from; k[0].ease = MPX_EASE_LINEAR;
    k[1].t_ms = (unsigned)ms;  k[1].stance = to;   k[1].ease = e;
    return mpx_stance_play(k, 2, mpx_play(50, 1));
}

/** Travel smoothly from one JOINT pose to another, over `ms`.
 *
 *  The joint-side twin of mpx_stance_glide(), and the answer to "how do I set
 *  the speed of a joint": you do not set a speed, you set a DURATION.
 *
 *      mpx_pose_t here = mpx_pose_now();
 *      mpx_pose_t up   = mpx_pose_with(here, MPX_FR_SHOULDER, 20.0f);
 *      mpx_pose_glide(here, up, 1200, MPX_EASE_INOUT);     // 1.2 s
 *
 *  Duration rather than degrees-per-second on purpose. Twelve joints moving
 *  different distances at the same deg/s finish at twelve different moments,
 *  and a robot whose legs stop one after another looks broken. Given a
 *  duration they all arrive together, which is what you meant.
 *
 *  If you do think in deg/s, the conversion is one line:
 *      ms = (int)(fabsf(to - from) / dps * 1000.0f);
 */
static inline int mpx_pose_glide(mpx_pose_t from, mpx_pose_t to,
                                 int ms, mpx_ease_t e)
{
    mpx_pose_key_t k[2];
    k[0].t_ms = 0;             k[0].pose = from; k[0].ease = MPX_EASE_LINEAR;
    k[1].t_ms = (unsigned)ms;  k[1].pose = to;   k[1].ease = e;
    return mpx_pose_play(k, 2, mpx_play(50, 1));
}

/** Oscillate one value as a sine over time — the cheapest way to make
 *  something look alive. Phase is in turns (0..1), not radians. */
static inline float mpx_wave(unsigned t_ms, float period_ms,
                             float amplitude, float phase_turns)
{
    if (period_ms <= 0.0f) return 0.0f;
    return amplitude * mpx_sin(((float)t_ms / period_ms + phase_turns) * MPX_TWO_PI);
}

#ifdef __cplusplus
}
#endif
#endif /* MPX_MOTION_H */
