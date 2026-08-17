/* walk_creep.c — a walk that actually stays up.
 *
 * The companion to examples/trot-4leg, which falls over. Same robot, same
 * motors, same tuning; the only difference is WHEN each foot leaves the ground.
 *
 * A TROT swings diagonal pairs, so only two feet are down at a time and the
 * body has to be caught by momentum. That is a dynamic gait, and it is why the
 * trot example lands on its side at 0.23 s.
 *
 * A CREEP swings ONE leg at a time. Three feet are always on the floor, so
 * there is always a support triangle under the centre of mass and the robot is
 * statically stable — it would stay up even if you paused it mid-stride.
 * Slower, but it works, and it is the right first walk to build on.
 *
 *     cd examples/walk-creep
 *     mpx-cli build src/walk_creep.c
 *     python ../../tools/mjsim.py build/walk_creep.wasm
 *
 * These numbers were not guessed. They came out of a 24-point parameter sweep
 * in MuJoCo, scored on "stayed upright" then "travelled furthest".
 */
#include "mpx_host.h"

#define L1        50.0f     /* thigh, mm — from the MJCF                      */
#define L2        56.0f     /* calf,  mm                                      */
#define STAND_Z  (-78.0f)   /* body height. -70 fell in 4 of 6 sweep runs;    */
                            /* -78 survived 11 of 12. Lower really is safer.  */
#define STRIDE    10.0f     /* half step length, mm. Longer strides swing the */
                            /* CoM past the support triangle.                 */
#define LIFT      16.0f     /* foot clearance, mm                             */
#define CYCLE_MS  1600      /* one full four-leg cycle                        */
#define DT_MS     20
#define CYCLES    3

/* ── freestanding maths: a WASM skill has no libm ─────────────────────── */
static float f_abs(float x){ return x<0?-x:x; }
static float f_sqrt(float x){ if(x<=0)return 0; float g=x*0.5f;
    for(int i=0;i<14;i++) g=0.5f*(g+x/g); return g; }
static float f_sin(float x){ while(x> 3.14159265f)x-=6.28318531f;
    while(x<-3.14159265f)x+=6.28318531f; float x2=x*x;
    return x*(1.0f - x2/6.0f + x2*x2/120.0f - x2*x2*x2/5040.0f); }
/* Minimax polynomial, not the Taylor series.
 *
 * The obvious x - x^3/3 + x^5/5 - x^7/7 is off by up to 3.5 DEGREES near
 * |x| = 1, which is squarely inside the working range of a leg IK. That error
 * was enough to turn a gait that stood up in a parameter sweep into one that
 * fell over at 3.6 s — the trajectory was right and the arithmetic was not.
 * This form is accurate to 0.0007 deg over the same range for the same cost. */
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

/* Foot target in the hip frame (mm) -> shoulder and knee angles (deg). */
static void leg_ik(float x, float z, float *sh, float *kn){
    float d2=x*x+z*z, d=f_sqrt(d2);
    if(d > L1+L2-1.0f){ float s=(L1+L2-1.0f)/d; x*=s; z*=s; d2=x*x+z*z; d=f_sqrt(d2); }
    float k = f_acos((d2-L1*L1-L2*L2)/(2.0f*L1*L2));
    float s = f_atan2(x,-z) + f_acos((d2+L1*L1-L2*L2)/(2.0f*L1*d));
    *sh = s*57.29578f - 45.0f;
    *kn = k*57.29578f - 90.0f;
}

/* Swing order. Opposite corners alternate, which keeps the support triangle
 * as large as possible at every moment — RF, then the diagonally opposite LB,
 * then LF, then RB. */
static const int SH[4] = { 2, 11,  5,  8 };   /* RF, LB, LF, RB shoulders */
static const int KN[4] = { 3, 12,  6,  9 };   /* ...and their knees       */

void on_start(void)
{
    if(mpx_abi_version() != MPX_ABI_VERSION){ MPX_LOG("rebuild against this SDK"); return; }
    MPX_LOG("creep walk — three feet down at all times");

    const int steps = CYCLE_MS / DT_MS;

    for(int c = 0; c < CYCLES; c++){
        for(int i = 0; i < steps; i++){
            float p = (float)i / (float)steps;          /* 0..1 through the cycle */

            for(int leg = 0; leg < 4; leg++){
                /* Each leg owns one quarter of the cycle for its swing. */
                float local = p - (float)leg * 0.25f;
                while(local < 0.0f) local += 1.0f;

                float x, z;
                if(local < 0.25f){                       /* SWING: lift and reach */
                    float u = local / 0.25f;
                    x = STRIDE * (2.0f*u - 1.0f);
                    z = STAND_Z + LIFT * f_sin(u * 3.14159265f);
                }else{                                   /* STANCE: push back     */
                    float u = (local - 0.25f) / 0.75f;
                    x = STRIDE * (1.0f - 2.0f*u);
                    z = STAND_Z;
                }

                float sh, kn;
                leg_ik(x, z, &sh, &kn);
                robot_set_servo_angle(SH[leg], (int)(sh * 100.0f));
                robot_set_servo_angle(KN[leg], (int)(kn * 100.0f));
            }

            robot_flush();
            robot_delay_ms(DT_MS);
        }
    }

    MPX_LOG("done");
}
