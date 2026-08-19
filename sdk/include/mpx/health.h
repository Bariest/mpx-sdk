/* mpx/health.h — what the robot can tell you about itself.
 *
 * Two groups, both about CONDITION rather than movement, which is why they are
 * not in mpx/leg.h:
 *
 *   TELEMETRY     load, current, temperature, battery. Read them to decide
 *                 something -- back off when a joint is straining, park when
 *                 the battery is low, stop before a motor cooks.
 *
 *   CALIBRATION   the per-joint offsets that define where "centre" is, and a
 *                 ping to ask whether a joint is answering at all.
 *
 * EVERY READ IS A ROUND TRIP to a driver board -- roughly a millisecond. Do not
 * put twelve of them inside a 60 Hz loop; sample one joint per frame, or read
 * between moves. This is the single most common way to turn a smooth skill
 * into a juddering one.
 */
#ifndef MPX_HEALTH_H
#define MPX_HEALTH_H

#include "mpx/abi.h"
#include "mpx/leg.h"
#include "mpx/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* mpx_ik2() was here, and it lied. It solved a two-link leg with MPX_CALF_MM
 * (60 mm, the Stanford model) and a hardcoded -45/-90 degree centring, while
 * mpx_foot_set() below goes through the firmware's planar IK: 56 mm calf, a
 * neutral computed from NEUTRAL_Z, and per-leg sign flips. Compared side by
 * side they disagree by 12 degrees at the standing pose and by up to 74
 * degrees at reach. A skill that solved a leg with mpx_ik2() and commanded the
 * angles landed somewhere else entirely -- and the leg still moved, so nothing
 * announced the error.
 *
 * If you want the angles for a foot position, ask the robot for them: call
 * mpx_foot_set() and read mpx_joint_at(). If you are writing your own model,
 * mpx/geometry.h has the dimensions -- but know that the FEET path uses the
 * firmware's planar IK, whose calf is 56 mm, not MPX_CALF_MM's 60 mm.
 */

#ifdef __cplusplus
}
#endif
#endif /* MPX_HEALTH_H */
