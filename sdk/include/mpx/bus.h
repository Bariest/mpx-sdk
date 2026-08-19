/* mpx/bus.h — owning the motor, not just the joint.
 *
 * The deepest layer. Here you set not only where a joint should be but how
 * hard it tries to get there: the proportional and derivative gains of the
 * servo's own control loop, and the current it is allowed to draw. This is
 * what people mean by "Unitree-style" joint control, and it is how you get a
 * leg that is compliant on landing, or stiff enough to hold a load.
 *
 * ── TAKING THE BUS PARKS THE GAIT ──────────────────────────────────────────
 * While you hold the bus, the gait generator does not run. You cannot walk and
 * tune at the same time. The usual shape is:
 *
 *     mpx_bus_take();
 *     ... set gains ...
 *     mpx_bus_release();
 *     mpx_gait(MPX_GAIT_FORWARD);        // now the gait uses your gains
 *
 * Gains persist on the driver board after you release, which is the point:
 * the built-in gaits pick them up too.
 *
 * ── THIS FILE SPEAKS A DIFFERENT ANGLE FRAME ───────────────────────────────
 * The driver boards use ABSOLUTE degrees, 0..270, with 135 at centre. The rest
 * of the SDK uses relative degrees, +/-135 with 0 at centre. Every function
 * here says _abs_ in its name so the two cannot be confused, and the macros
 * below convert.
 *
 *     absolute = 135 + relative
 *
 * ── THE ROBOT WILL LET YOU BREAK IT ────────────────────────────────────────
 * A high Kp with no current cap can stall a servo against its own limit and
 * cook it. Start from the stock values (Kp 65, Kd 800), change one joint at a
 * time, and keep a current cap on. The robot's Servo Studio page has a live
 * scope for exactly this and is a better place to find your numbers than a
 * skill is.
 */
#ifndef MPX_BUS_H
#define MPX_BUS_H

#include "mpx/abi.h"
#include "mpx/params.h"
#include "mpx/sys.h"
#include "mpx/leg.h"
#include "mpx/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Centre of the absolute frame, in absolute degrees. */
#define MPX_ABS_CENTRE_DEG 135.0f

/** Relative (+/-135, 0 = centre)  ->  absolute (0..270, 135 = centre). */
#define MPX_ABS_FROM_REL(d) ((float)(d) + MPX_ABS_CENTRE_DEG)
/** Absolute -> relative. */
#define MPX_REL_FROM_ABS(d) ((float)(d) - MPX_ABS_CENTRE_DEG)

/* ═══════════════════════════════════════════════════════════════════════════
 *  Ownership
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Take the bus. MPX_ERR_STATE if something else holds it — usually the
 *  robot's Servo Studio page being open in a browser somewhere. */
static inline int mpx_bus_take(void)    { return servo_lock(); }

/** Give the bus back. The gait generator resumes. The sandbox does this for
 *  you if your skill ends while still holding it, so a crash cannot leave the
 *  robot parked until someone reboots it. */
static inline int mpx_bus_release(void) { return servo_unlock(); }

/** Non-zero if the bus is currently held by anyone. */
static inline int mpx_bus_held(void)    { return servo_is_locked(); }

/* ═══════════════════════════════════════════════════════════════════════════
 *  Gains
 *
 *  Written through the driver board's config path, which is slow — tens of
 *  milliseconds each. Set them once, outside your motion loop.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* The parameter slots themselves are in mpx/params.h, generated from the
 * driver board's own table. They were hand-written here once and were wrong:
 * slot 4 is reverse_motor, not KP_POSITION, so setting "stiffness" reversed
 * a joint instead. Never retype a wire protocol. */

/* The four tuned gains this robot ships with. The two loops are in different
 * units and four orders of magnitude apart, which is exactly the trap: 65 is a
 * sane position gain and a catastrophic current gain. Restore from these
 * constants rather than from a number you remember. */
#define MPX_KP_STOCK           65.0f      /* position P                       */
#define MPX_KD_STOCK          800.0f      /* position D                       */
#define MPX_KP_CURRENT_STOCK    0.0006f   /* current P — note the scale       */
#define MPX_KFF_CURRENT_STOCK   0.00022f  /* current feed-forward             */

/** Set one parameter on one joint — or on all twelve with MPX_ALL_JOINTS.
 *
 *  THIS IS THE WHOLE GAIN API. There were five more functions wrapping it
 *  (mpx_current_kp, mpx_current_kff, mpx_current_all, mpx_max_effort,
 *  mpx_max_effort_all) and they earned their keep by naming things — except
 *  mpx/params.h already names them, generated from the driver board's own
 *  table, and MPX_PARAM_KP_CURRENT says which loop as well as which gain.
 *  Five wrappers to learn, or one function and an enum you can autocomplete.
 *
 *  Max PWM duty is clamped to 0..1 here rather than in a wrapper, so the limit
 *  holds no matter which path writes it.
 *
 *  Writes are slow — a driver-board config exchange, milliseconds each. Set
 *  them once, outside your motion loop. Returns the first error, or MPX_OK. */
static inline int mpx_gain_set(mpx_joint_t j, mpx_param_t p, float v)
{
    if (p == MPX_PARAM_MAX_PWM_DUTY_CYCLE) v = mpx_clamp(v, 0.0f, 1.0f);

    if (j == MPX_ALL_JOINTS) {
        /* EVERY joint is attempted, and the worst result is reported.
         *
         * This returned on the first error once, which sounds careful and is
         * the opposite. A driver board that has come unplugged fails on its
         * first servo, so a robot with one bad board could not have ANY of its
         * other nine joints tuned — the write died on joint 1 and the loop
         * never reached the boards that were answering. "All twelve" has to
         * mean it tried all twelve.
         *
         * The return value still tells you something went wrong; it just no
         * longer decides that nothing else is worth doing. Read one back if
         * you need to know which joints took it. */
        int worst = MPX_OK;
        for (int id = 1; id <= 12; ++id) {
            int rc = servo_set_gain(id, (int)p, v);
            if (rc != MPX_OK) worst = rc;
        }
        return worst;
    }
    return servo_set_gain((int)j, (int)p, v);
}

/** Read one parameter back from one joint. Not valid with MPX_ALL_JOINTS —
 *  twelve joints have twelve answers and one float to put them in. */
static inline int mpx_gain_get(mpx_joint_t j, mpx_param_t p, float *out)
{
    if (!out || j == MPX_ALL_JOINTS) return MPX_ERR_ARG;
    return servo_get_gain((int)j, (int)p, out);
}

/** Put every joint back to the factory control gains — all four, both loops.
 *
 *  Keep this one call: it is the safety net, and a skill that restores only
 *  half is worse than one that restores nothing, because it looks tidy. Call
 *  it at the end of on_start AND in on_stop — gains live on the driver board
 *  and outlive your skill. */
static inline int mpx_gains_stock(void)
{
    int worst = MPX_OK, rc;
    rc = mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KP_POSITION,  MPX_KP_STOCK);
    if (rc != MPX_OK) worst = rc;
    rc = mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KD_POSITION,  MPX_KD_STOCK);
    if (rc != MPX_OK) worst = rc;
    rc = mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KP_CURRENT,   MPX_KP_CURRENT_STOCK);
    if (rc != MPX_OK) worst = rc;
    rc = mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KFF_CURRENT,  MPX_KFF_CURRENT_STOCK);
    if (rc != MPX_OK) worst = rc;
    return worst;
}

/** Persist this joint's gains to the driver board's flash. Survives a reboot —
 *  which is exactly why you should be sure first. */
static inline int mpx_gain_save(mpx_joint_t j)    { return servo_save_config((int)j); }
/** Reload the joint's gains from its flash, discarding run-time changes. */
static inline int mpx_gain_restore(mpx_joint_t j) { return servo_restore_config((int)j); }

/* ═══════════════════════════════════════════════════════════════════════════
 *  Moving
 *
 *  One joint: mpx_bus_move(). Several at once: stage them, then send.
 *
 *  The two-step exists because one bus transaction per FRAME is what keeps
 *  motion smooth — sending per joint gives you a robot that judders. But that
 *  is only true when you are moving several joints, and it used to be the
 *  only option, so "move one joint" was a five-argument stage plus a separate
 *  send that was easy to forget. Forgetting it fails silently: the joint is
 *  queued and nothing is ever transmitted.
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Move one joint and send it, in one call. Relative degrees, +/-135, 0 =
 *  centre. Uses the gains already set and the board's own current limit.
 *
 *  The call to reach for. Anything it cannot express is below. */
static inline int mpx_bus_move(mpx_joint_t j, float deg)
{
    int rc = servo_stage((int)j, MPX_ABS_FROM_REL(deg), 0.0f, 0.0f, 0.0f);
    if (rc != MPX_OK) return rc;
    return servo_commit();
}

/** Queue one joint for the next mpx_bus_send(). Relative degrees.
 *
 *  Nothing moves until you send. Stage every joint of the frame, then send
 *  once. */
static inline int mpx_bus_stage(mpx_joint_t j, float deg)
{
    return servo_stage((int)j, MPX_ABS_FROM_REL(deg), 0.0f, 0.0f, 0.0f);
}

/** mpx_bus_stage() with the per-frame overrides.
 *
 *  @param tau_ma  Current cap for this move, mA. 0 uses the board's own.
 *  @param kp,kd   Per-frame gains. 0,0 uses the gains already set — which is
 *                 what you want unless you are modulating stiffness WITHIN a
 *                 motion, such as going compliant as a foot lands. */
static inline int mpx_bus_stage_ex(mpx_joint_t j, float deg, float tau_ma,
                                   float kp, float kd)
{
    return servo_stage((int)j, MPX_ABS_FROM_REL(deg), tau_ma, kp, kd);
}

/** Send every staged joint in one bus transaction. */
static inline int mpx_bus_send(void) { return servo_commit(); }

/** Let a joint go limp — or all of them with MPX_ALL_JOINTS.
 *
 *  How you check a leg by hand without fighting it. Relaxing all twelve sits
 *  the robot down; hold it or expect it to drop. */
static inline int mpx_bus_relax(mpx_joint_t j)
{
    if (j == MPX_ALL_JOINTS) {
        for (int id = 1; id <= 12; ++id) {
            int rc = servo_direct(id, 0, 0.0f, 0.0f);
            if (rc != MPX_OK) return rc;
        }
        return MPX_OK;
    }
    return servo_direct((int)j, 0, 0.0f, 0.0f);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  State
 * ═══════════════════════════════════════════════════════════════════════════ */

/** One joint's measured state. Angles are ABSOLUTE degrees. */
typedef struct {
    float q_deg;     /**< Measured position, absolute degrees. */
    float dq_dps;    /**< Measured speed, degrees per second.  */
    float tau_ma;    /**< Measured current, mA.                */
    float temp_c;    /**< Winding temperature, Celsius.        */
} mpx_bus_state_t;

/** Refresh the firmware's cached state for all joints. One bus sweep. */
static inline int mpx_bus_poll(void) { return servo_poll(); }

static inline int mpx_bus_read(mpx_joint_t j, mpx_bus_state_t *out)
{
    if (!out) return MPX_ERR_ARG;
    return servo_read((int)j, out);
}

/** Read all twelve into an array of twelve, index 0 = joint 1. */
static inline int mpx_bus_read_all(mpx_bus_state_t out[12])
{
    if (!out) return MPX_ERR_ARG;
    return servo_read_all(out);
}

/** Bitmask of joints that answered: bit 0 is joint 1. 0xFFF means all twelve.
 *  The quickest way to find a servo that is not talking. */
static inline int mpx_bus_scan(void) { return servo_scan(); }

#ifdef __cplusplus
}
#endif
#endif /* MPX_BUS_H */
