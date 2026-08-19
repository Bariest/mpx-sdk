/* mpx_compat.h — v2 names, for one more release.
 *
 * Include this INSTEAD of the old "mpx_host.h" and a skill written against SDK
 * v2 keeps compiling:
 *
 *     -#include "mpx_host.h"
 *     +#include "mpx_compat.h"
 *
 * Everything here forwards to the v3 API with no runtime cost. It exists so
 * that upgrading is two separate decisions — "does it still build" and "do I
 * like the new names" — rather than one big one.
 *
 * ── WHAT IS NOT HERE ───────────────────────────────────────────────────────
 * The forty-odd one-line gait wrappers (`robot_look_upper_left(ms)`,
 * `robot_backleg_lift_right(ms)`, `robot_move_lb(ms)` ...). Every one of them
 * was `gait(X); delay(ms); gait(NONE);`, they tripled the header's length, and
 * they hard-coded a policy — always return to NONE — that is wrong the moment
 * you want to chain two moves. They are one function now:
 *
 *     robot_look_upper_left(800)   ->   mpx_gait_for(MPX_GAIT_LOOK_UP_LEFT, 800)
 *     robot_move_lb(1500)          ->   mpx_gait_for(MPX_GAIT_MOVE_RL, 1500)
 *
 * `mpx-cli gaits` lists all 46 with their names, so the mapping is one lookup.
 *
 * ── THIS FILE WILL BE REMOVED ──────────────────────────────────────────────
 * Port off it. The v3 names are shorter, consistent, and the angle convention
 * is the same everywhere, which is the actual point.
 */
#ifndef MPX_COMPAT_H
#define MPX_COMPAT_H

#include "mpx.h"

/* ── Removed in the API reduction; here so old skills still build ────────── */
static inline int mpx_walk_speed_set(int mm_s) { return mpx_set_walk_speed(mm_s); }
static inline int mpx_walk_speed(void)         { return mpx_get_walk_speed(); }
static inline int mpx_body_speed(int dps)      { return robot_set_attitude_speed(dps); }
static inline int mpx_body_speed_xyz(int r, int p, int y)
                                { return robot_set_attitude_speed_xyz(r, p, y); }
static inline int mpx_body_to(float r, float p, float y, int dps, int settle_ms)
                                { return mpx_body_move(r, p, y, dps, settle_ms); }
static inline int mpx_bus_stage(mpx_joint_t j, float deg) { return mpx_bus_set(j, deg); }
static inline int mpx_bus_move (mpx_joint_t j, float deg) { return mpx_bus_apply(j, deg); }
static inline int mpx_bus_stage_ex(mpx_joint_t j, float deg, float tau_ma,
                                   float kp, float kd)
{   /* kp/kd were always discarded by the stock AT32 firmware. */
    (void)kp; (void)kd; return mpx_bus_set_ex(j, deg, tau_ma);
}
static inline mpx_stance_t mpx_stance_crouch(float drop_mm)
{   return mpx_stance_all(0.0f, 0.0f, MPX_STAND_Z_MM + drop_mm); }
static inline mpx_stance_t mpx_stance_front(float reach_mm, float front_z_mm)
{
    mpx_stance_t s = mpx_stance_stand();
    s = mpx_stance_with(s, MPX_FR, reach_mm, 0.0f, front_z_mm);
    s = mpx_stance_with(s, MPX_FL, reach_mm, 0.0f, front_z_mm);
    return s;
}


#ifdef __cplusplus
extern "C" {
#endif

/* ── Gait enum ──────────────────────────────────────────────────────────────
 * Same numeric values; the names gained a prefix and a few gained clarity
 * (GAIT_ADVANCE reads as a verb but is not one — it is MPX_GAIT_FORWARD now). */
typedef mpx_gait_t robot_gait_t;
#define GAIT_NONE         MPX_GAIT_NONE
#define GAIT_INIT         MPX_GAIT_STAND
#define GAIT_STEP         MPX_GAIT_STEP
#define GAIT_ROLL         MPX_GAIT_ROLL
#define GAIT_PITCH        MPX_GAIT_PITCH
#define GAIT_STRETCH      MPX_GAIT_STRETCH
#define GAIT_ADVANCE      MPX_GAIT_FORWARD
#define GAIT_BACK         MPX_GAIT_BACK
#define GAIT_LEFT         MPX_GAIT_STRAFE_LEFT
#define GAIT_RIGHT        MPX_GAIT_STRAFE_RIGHT
#define GAIT_TURN_L       MPX_GAIT_TURN_LEFT
#define GAIT_TURN_R       MPX_GAIT_TURN_RIGHT
#define GAIT_TWERK        MPX_GAIT_TWERK
#define GAIT_JUMP         MPX_GAIT_JUMP
#define GAIT_JUMP_FWD     MPX_GAIT_JUMP_FORWARD
#define GAIT_TEST_SPD     MPX_GAIT_TEST_SPEED
#define GAIT_LOOK_UP      MPX_GAIT_LOOK_UP
#define GAIT_LOOK_DOWN    MPX_GAIT_LOOK_DOWN
#define GAIT_LOOK_LEFT    MPX_GAIT_LOOK_LEFT
#define GAIT_LOOK_RIGHT   MPX_GAIT_LOOK_RIGHT
#define GAIT_LOOK_UL      MPX_GAIT_LOOK_UP_LEFT
#define GAIT_LOOK_UR      MPX_GAIT_LOOK_UP_RIGHT
#define GAIT_LOOK_LL      MPX_GAIT_LOOK_DOWN_LEFT
#define GAIT_LOOK_LR      MPX_GAIT_LOOK_DOWN_RIGHT
#define GAIT_FORELEG_LIFT_L MPX_GAIT_LIFT_FRONT_LEFT
#define GAIT_FORELEG_LIFT_R MPX_GAIT_LIFT_FRONT_RIGHT
#define GAIT_BACKLEG_LIFT_L MPX_GAIT_LIFT_REAR_LEFT
#define GAIT_BACKLEG_LIFT_R MPX_GAIT_LIFT_REAR_RIGHT
#define GAIT_HEIGHT_UP    MPX_GAIT_HEIGHT_UP
#define GAIT_HEIGHT_DOWN  MPX_GAIT_HEIGHT_DOWN
#define GAIT_BALANCE      MPX_GAIT_BALANCE
#define GAIT_BOW_BACK     MPX_GAIT_BOW
#define GAIT_BODY_CYCLE   MPX_GAIT_BODY_CYCLE
#define GAIT_HEAD_ELLIPSE MPX_GAIT_HEAD_ELLIPSE
#define GAIT_MOVE_LF      MPX_GAIT_MOVE_FL
#define GAIT_MOVE_RF      MPX_GAIT_MOVE_FR
#define GAIT_MOVE_LB      MPX_GAIT_MOVE_RL
#define GAIT_MOVE_RB      MPX_GAIT_MOVE_RR
#define GAIT_STANFORD     MPX_GAIT_TROT
#define GAIT_FRONT_KICK   MPX_GAIT_FRONT_KICK
#define GAIT_WIGGLE       MPX_GAIT_WIGGLE
#define GAIT_BUTT_SHRUG   MPX_GAIT_BUTT_SHRUG
#define GAIT_WIGGLE_L     MPX_GAIT_WIGGLE_LEFT
#define GAIT_WIGGLE_R     MPX_GAIT_WIGGLE_RIGHT
#define GAIT_BUTT_SHRUG_L MPX_GAIT_BUTT_SHRUG_LEFT
#define GAIT_BUTT_SHRUG_R MPX_GAIT_BUTT_SHRUG_RIGHT

/* ── Servo ids ───────────────────────────────────────────────────────────── */
typedef mpx_joint_t robot_servo_t;
#define SERVO_FR_HIP MPX_FR_HIP
#define SERVO_FR_SHOULDER MPX_FR_SHOULDER
#define SERVO_FR_KNEE MPX_FR_KNEE
#define SERVO_FL_HIP MPX_FL_HIP
#define SERVO_FL_SHOULDER MPX_FL_SHOULDER
#define SERVO_FL_KNEE MPX_FL_KNEE
#define SERVO_RR_HIP MPX_RR_HIP
#define SERVO_RR_SHOULDER MPX_RR_SHOULDER
#define SERVO_RR_KNEE MPX_RR_KNEE
#define SERVO_RL_HIP MPX_RL_HIP
#define SERVO_RL_SHOULDER MPX_RL_SHOULDER
#define SERVO_RL_KNEE MPX_RL_KNEE

/* ── Logging ─────────────────────────────────────────────────────────────── */
static inline void MPX_print(const char *s, int len) { mpx_log_n(s, len); }
static inline void MPX_print_int(int v) { mpx_log_i("", v); }

/* ── Gaits and movement ──────────────────────────────────────────────────── */
static inline int  robot_gait_enum(mpx_gait_t g)   { return mpx_gait(g); }
static inline void robot_stand(void)               { mpx_stand(); }
static inline int  robot_walk_forward(int ms)      { return mpx_gait_for(MPX_GAIT_FORWARD, ms); }
static inline int  robot_walk_backward(int ms)     { return mpx_gait_for(MPX_GAIT_BACK, ms); }
static inline int  robot_turn_left(int ms)         { return mpx_gait_for(MPX_GAIT_TURN_LEFT, ms); }
static inline int  robot_turn_right(int ms)        { return mpx_gait_for(MPX_GAIT_TURN_RIGHT, ms); }
static inline int  robot_strafe_left(int ms)       { return mpx_gait_for(MPX_GAIT_STRAFE_LEFT, ms); }
static inline int  robot_strafe_right(int ms)      { return mpx_gait_for(MPX_GAIT_STRAFE_RIGHT, ms); }
static inline int  robot_step_in_place(int ms)     { return mpx_gait_for(MPX_GAIT_STEP, ms); }
static inline void robot_jump(void)                { mpx_gait_for(MPX_GAIT_JUMP, 0); }
static inline void robot_front_kick(void)          { mpx_gait_for(MPX_GAIT_FRONT_KICK, 0); }
static inline int  robot_stanford_walk(int ms)     { return mpx_gait_for(MPX_GAIT_TROT, ms); }
static inline int  robot_wiggle(int ms)            { return mpx_gait_for(MPX_GAIT_WIGGLE, ms); }
static inline int  robot_butt_shrug(int ms)        { return mpx_gait_for(MPX_GAIT_BUTT_SHRUG, ms); }
static inline int  robot_balance(int ms)           { return mpx_gait_for(MPX_GAIT_BALANCE, ms); }
static inline int  robot_bow_back(int ms)          { return mpx_gait_for(MPX_GAIT_BOW, ms); }

/* ── Body attitude ───────────────────────────────────────────────────────── */
static inline void robot_attitude(float r, float p, float y) { mpx_body(r, p, y); }
static inline void robot_roll (float d)            { mpx_body(d, 0.0f, 0.0f); }
static inline void robot_pitch(float d)            { mpx_body(0.0f, d, 0.0f); }
static inline void robot_yaw  (float d)            { mpx_body(0.0f, 0.0f, d); }
static inline void robot_reset_attitude(void)      { mpx_gait(MPX_GAIT_NONE); }
static inline void robot_attitude_speed(float dps) { mpx_body_speed((int)dps); }
static inline void robot_attitude_speed_xyz(float r, float p, float y)
                                                   { mpx_body_speed_xyz((int)r, (int)p, (int)y); }
static inline void robot_attitude_at(float r, float p, float y, float dps)
                                                   { mpx_body_speed((int)dps); mpx_body(r, p, y); }

/* ── Renamed in the _set/_apply/_move cleanup ─────────────────────────────
   mpx_joint_to/mpx_foot_to/mpx_feet_to read as if they moved something. They
   stage a frame; mpx_frame_send() moves it. Renamed to _set to say so. */
static inline int mpx_joint_to(mpx_joint_t j, float deg) { return mpx_joint_set(j, deg); }
static inline int mpx_foot_to (mpx_leg_t leg, float x, float splay, float z)
                                                { return mpx_foot_set(leg, x, splay, z); }
static inline int mpx_feet_to (float x, float splay, float z)
                                                { return mpx_feet_set(x, splay, z); }
static inline int mpx_feet_stand(void) { return mpx_feet_set(0.0f, 0.0f, MPX_STAND_Z_MM); }

/* ── Joints ──────────────────────────────────────────────────────────────── */
static inline int  robot_set_servo_deg(mpx_joint_t j, float deg) { return mpx_joint_set(j, deg); }
/* `speed` is accepted and ignored: the driver boards have no speed field and
   the firmware never sent this value anywhere. See the note in mpx/leg.h. */
static inline void robot_set_servo(mpx_joint_t j, float deg, int speed)
                                                   { (void)speed; mpx_joint_set(j, deg); }

/* ── Config ──────────────────────────────────────────────────────────────── */
typedef struct { int period, height, up_height, stride, tilt; } robot_config_t;

static inline void robot_set_config_ex(robot_config_t c)
{
    mpx_gait_config_t g;
    g.period_ms = c.period; g.height_mm = c.height; g.lift_mm = c.up_height;
    g.stride_mm = c.stride; g.tilt_deg = c.tilt;
    mpx_gait_config_set(g);
}

static inline robot_config_t robot_get_config_ex(void)
{
    mpx_gait_config_t g = mpx_gait_config();
    robot_config_t c;
    c.period = g.period_ms; c.height = g.height_mm; c.up_height = g.lift_mm;
    c.stride = g.stride_mm; c.tilt = g.tilt_deg;
    return c;
}

/* ── Full pose ───────────────────────────────────────────────────────────── */
typedef struct {
    float fr_hip, fr_shoulder, fr_knee;
    float fl_hip, fl_shoulder, fl_knee;
    float rr_hip, rr_shoulder, rr_knee;
    float rl_hip, rl_shoulder, rl_knee;
} robot_pose_t;

static inline void robot_apply_pose(robot_pose_t p)
{
    mpx_pose_t q;
    q.deg[0]=p.fr_hip; q.deg[1]=p.fr_shoulder; q.deg[2]=p.fr_knee;
    q.deg[3]=p.fl_hip; q.deg[4]=p.fl_shoulder; q.deg[5]=p.fl_knee;
    q.deg[6]=p.rr_hip; q.deg[7]=p.rr_shoulder; q.deg[8]=p.rr_knee;
    q.deg[9]=p.rl_hip; q.deg[10]=p.rl_shoulder; q.deg[11]=p.rl_knee;
    mpx_pose_apply(q);
}

/* ── Servo bus ───────────────────────────────────────────────────────────── */
static inline int mpx_servo_set_all_gains(float kp, float kd)
{
    int rc = mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KP_POSITION, kp);
    if (rc != MPX_OK) return rc;
    return mpx_gain_set(MPX_ALL_JOINTS, MPX_PARAM_KD_POSITION, kd);
}

#ifdef __cplusplus
}
#endif
#endif /* MPX_COMPAT_H */
