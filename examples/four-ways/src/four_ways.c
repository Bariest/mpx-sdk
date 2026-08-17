/* four_ways.c — every way to move this robot, in one file.
 *
 * There are four, and they are layered. Each gives you more control and takes
 * more responsibility. Pick the highest one that does what you need.
 *
 *   1. GAITS            the firmware walks; you say "advance"
 *   2. BUILT-IN IK      you give foot positions; firmware solves the angles
 *   3. YOUR OWN IK      you solve the angles; firmware just passes them on
 *   4. LOW-LEVEL SERVO  you set the angle AND the control gains per joint
 *
 *     cd examples/four-ways
 *     mpx-cli deploy
 *
 * Set WHICH below to run one section at a time. Running all four back to back
 * works, but you learn more watching one.
 */
#include "mpx_host.h"

#define WHICH  0        /* 0 = all, or 1..4 for a single section */

/* ── freestanding maths (a WASM skill has no libm) ─────────────────────── */
static float f_abs(float x){ return x<0?-x:x; }
static float f_sqrt(float x){ if(x<=0)return 0; float g=x*0.5f;
    for(int i=0;i<14;i++) g=0.5f*(g+x/g); return g; }
static float f_sin(float x){ while(x> 3.14159265f)x-=6.28318531f;
    while(x<-3.14159265f)x+=6.28318531f; float x2=x*x;
    return x*(1.0f - x2/6.0f + x2*x2/120.0f - x2*x2*x2/5040.0f); }
/* Minimax, NOT the Taylor series — that one is off by 3.5 deg near |x|=1,
 * which is inside a leg IK's working range. See examples/walk-creep. */
static float f_atan(float x){ if(f_abs(x)>1.0f)
        return (x>0?1.57079633f:-1.57079633f) - f_atan(1.0f/x);
    float x2=x*x;
    return x*(0.9998660f + x2*(-0.3302995f + x2*(0.1801410f
             + x2*(-0.0851330f + x2*0.0208351f)))); }
static float f_atan2(float y,float x){ if(x>0) return f_atan(y/x);
    if(x<0) return f_atan(y/x) + (y>=0?3.14159265f:-3.14159265f);
    return y>0?1.57079633f:-1.57079633f; }
static float f_acos(float c){ if(c>1)c=1; if(c<-1)c=-1;
    return 1.57079633f - f_atan2(c, f_sqrt(1.0f-c*c)); }


/* ═══ 1. GAITS — the firmware does the walking ════════════════════════════
 *
 * Highest level. The gait generator runs on the robot; you choose a name and
 * a duration. You cannot shape the motion, but you also cannot fall over from
 * a maths mistake.
 */
static void way1_gaits(void)
{
    MPX_LOG("1. built-in gaits");

    int rc = robot_gait_enum(GAIT_ADVANCE);   /* returns a code since ABI v2 */
    if (rc != MPX_OK) { MPX_LOG("gait refused"); return; }
    robot_delay_ms(1500);

    robot_gait_enum(GAIT_TURN_L);  robot_delay_ms(1000);
    robot_gait_enum(GAIT_NONE);    robot_delay_ms(300);
    robot_gait_enum(GAIT_INIT);    robot_delay_ms(800);
}


/* ═══ 2. BUILT-IN IK — you place the feet, firmware finds the angles ══════
 *
 * robot_ik_fr/fl/rr/rl take a FOOT POSITION and solve for that leg. This is
 * the Stanford kinematics the gait generator itself uses, so your poses sit
 * in the same frame as the built-in gaits and inherit the calibration offsets.
 *
 *   x    mm, forward (+) / back (-)
 *   th0  deg, hip abduction — the sideways splay
 *   z    mm, DOWN from the hip. About -78 is a normal stand.
 *
 * Use this when you want your own poses without owning the trigonometry.
 */
static void way2_builtin_ik(void)
{
    MPX_LOG("2. built-in IK — body sway with the feet planted");

    for (int i = 0; i < 90; i++) {
        float t = (float)i / 90.0f;
        float sway = 18.0f * f_sin(t * 6.28318531f);   /* fore-aft, mm */

        /* All four feet stay on the floor; the body rides over them. */
        robot_ik_fr( sway, 0.0f, -78.0f);
        robot_ik_fl( sway, 0.0f, -78.0f);
        robot_ik_rr( sway, 0.0f, -78.0f);
        robot_ik_rl( sway, 0.0f, -78.0f);

        robot_delay_ms(20);
    }

    /* There is also whole-body attitude, if you only want to tilt: */
    robot_set_attitude_speed(60);           /* deg/s, so it glides not snaps */
    robot_set_body_pose(0.0f, 10.0f, 0.0f); /* roll, pitch, yaw */
    robot_delay_ms(700);
    robot_set_body_pose(0.0f, 0.0f, 0.0f);
    robot_delay_ms(700);
}


/* ═══ 3. YOUR OWN IK — you solve it, firmware passes it through ═══════════
 *
 * robot_set_servo_angle() takes a joint angle directly, so any kinematics you
 * write is welcome. Link lengths below come from the MJCF, not a guess.
 *
 * TWO THINGS THAT WILL BITE YOU:
 *
 *  - The angle is CENTIDEGREES and RELATIVE TO CENTRE (+/-135 deg = +/-13500).
 *  - Nothing moves until robot_flush(). Set every joint for a frame, then
 *    flush once. Flushing per joint gives you a robot that judders.
 */
#define L1 50.0f      /* thigh, mm — lf2 -> lf3 in the model */
#define L2 56.0f      /* calf,  mm — lf3 -> foot             */

static void my_leg_ik(float x, float z, float *shoulder_deg, float *knee_deg)
{
    float d2 = x*x + z*z, d = f_sqrt(d2);
    if (d > L1 + L2 - 1.0f) {                    /* clamp to reachable */
        float s = (L1 + L2 - 1.0f) / d;
        x *= s; z *= s; d2 = x*x + z*z; d = f_sqrt(d2);
    }
    float knee = f_acos((d2 - L1*L1 - L2*L2) / (2.0f*L1*L2));
    float sh   = f_atan2(x, -z) + f_acos((d2 + L1*L1 - L2*L2) / (2.0f*L1*d));
    *shoulder_deg = sh*57.29578f - 45.0f;        /* offset to the centred pose */
    *knee_deg     = knee*57.29578f - 90.0f;
}

static void way3_own_ik(void)
{
    MPX_LOG("3. my own IK — one leg through an arc");

    /* Plant the other three first so the robot has something to stand on. */
    float sh, kn;
    my_leg_ik(0.0f, -78.0f, &sh, &kn);
    robot_set_servo_angle(SERVO_FL_SHOULDER, (int)(sh*100));
    robot_set_servo_angle(SERVO_FL_KNEE,     (int)(kn*100));
    robot_set_servo_angle(SERVO_RR_SHOULDER, (int)(sh*100));
    robot_set_servo_angle(SERVO_RR_KNEE,     (int)(kn*100));
    robot_set_servo_angle(SERVO_RL_SHOULDER, (int)(sh*100));
    robot_set_servo_angle(SERVO_RL_KNEE,     (int)(kn*100));
    robot_flush();
    robot_delay_ms(400);

    for (int i = 0; i < 80; i++) {
        float t = (float)i / 80.0f;
        float x = 22.0f * f_sin(t * 6.28318531f);
        float z = -78.0f + 14.0f * f_sin(t * 3.14159265f);

        my_leg_ik(x, z, &sh, &kn);
        robot_set_servo_angle(SERVO_FR_SHOULDER, (int)(sh*100));
        robot_set_servo_angle(SERVO_FR_KNEE,     (int)(kn*100));

        robot_flush();                 /* ONE flush per frame */
        robot_delay_ms(16);

        /* Closed loop, if you want one. read_angle_cdeg is in the SAME frame
         * set_servo_angle takes; read_position is the opposite frame and a
         * loop built on it diverges. */
        /* int err = (int)(sh*100) - robot_read_angle_cdeg(SERVO_FR_SHOULDER); */
    }
}


/* ═══ 4. LOW-LEVEL SERVO — angle AND control gains, per joint ═════════════
 *
 * This is Unitree-style joint control. You take the servo bus, which PARKS
 * THE GAIT, then talk to the driver boards directly.
 *
 *   servo_lock()      stop the gait, take the bus. Nothing else may write.
 *   servo_set_gain()  Kp/Kd and the current loop, per servo, persistent
 *   servo_stage()     queue one joint: angle, current cap, per-frame kp/kd
 *   servo_commit()    send every staged joint in ONE bus transaction
 *   servo_unlock()    give the bus back, gait resumes
 *
 * THREE THINGS TO KNOW:
 *
 *  - servo_stage() takes the ABSOLUTE AT32 angle, 0..270 with 135 = centre —
 *    NOT the +/-135 relative frame used above. abs = 135 + rel.
 *  - You cannot walk while you hold the lock. Set gains, unlock, then gait.
 *  - Params 1,2,3 (MIN/MAX_POSITION_ADC, RANGE_POSITION_DEG) are calibration,
 *    not gains. servo_set_gain refuses them with MPX_ERR_READONLY.
 */
static void way4_low_level(void)
{
    MPX_LOG("4. low-level servo — gains then direct angles");

    if (servo_lock() != MPX_OK) {          /* Studio may hold the bus */
        MPX_LOG("could not take the servo bus");
        return;
    }

    /* Per-joint gains. These persist on the driver board after unlock, so the
     * built-in gait uses them too. Stock values are Kp=65, Kd=800. */
    for (int id = 1; id <= 12; id++) {
        servo_set_gain(id, MPX_PARAM_KP_POSITION, 65.0f);
        servo_set_gain(id, MPX_PARAM_KD_POSITION, 800.0f);
    }
    /* One joint stiffer than the rest, because you can: */
    servo_set_gain(SERVO_FR_KNEE, MPX_PARAM_KP_POSITION, 95.0f);

    /* Read one back to confirm it landed. */
    float kp = 0.0f;
    if (servo_get_gain(SERVO_FR_KNEE, MPX_PARAM_KP_POSITION, &kp) == MPX_OK) {
        /* kp is now 95.0 */
    }

    /* Direct angle control. 135 = centre; stage every joint, then commit once. */
    for (int i = 0; i < 60; i++) {
        float t = (float)i / 60.0f;
        float abs_deg = 135.0f + 12.0f * f_sin(t * 6.28318531f);

        /* id, angle(abs deg), current cap(mA), kp, kd  — 0,0 = use the
         * gains written above rather than a per-frame override. */
        servo_stage(SERVO_FR_SHOULDER, abs_deg, 400.0f, 0.0f, 0.0f);
        servo_stage(SERVO_FR_KNEE,     135.0f,  400.0f, 0.0f, 0.0f);

        servo_commit();                 /* one SPI transaction for the frame */
        robot_delay_ms(16);
    }

    servo_unlock();                     /* gait may run again */
    robot_delay_ms(200);
    robot_gait_enum(GAIT_INIT);
    robot_delay_ms(600);
}


void on_start(void)
{
    if (mpx_abi_version() != MPX_ABI_VERSION) {
        MPX_LOG("built against a different SDK — run mpx-cli deploy again");
        return;
    }

#if WHICH == 0 || WHICH == 1
    way1_gaits();
#endif
#if WHICH == 0 || WHICH == 2
    way2_builtin_ik();
#endif
#if WHICH == 0 || WHICH == 3
    way3_own_ik();
#endif
#if WHICH == 0 || WHICH == 4
    way4_low_level();
#endif

    MPX_LOG("done");
}
