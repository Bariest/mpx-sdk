/* A four-leg trot, written the way a maker would, to exercise the MuJoCo bridge. */
#include "mpx_host.h"
#define L1 50.0f
#define L2 56.0f
#define STAND_Z (-78.0f)
#define STRIDE   22.0f
#define LIFT     14.0f
#define FRAMES   240
#define DT_MS    12

static float f_abs(float x){ return x<0?-x:x; }
static float f_sqrt(float x){ if(x<=0)return 0; float g=x*0.5f; for(int i=0;i<14;i++) g=0.5f*(g+x/g); return g; }
static float f_sin(float x){ while(x> 3.14159265f)x-=6.28318531f; while(x<-3.14159265f)x+=6.28318531f;
    float x2=x*x; return x*(1.0f - x2/6.0f + x2*x2/120.0f - x2*x2*x2/5040.0f); }
static float f_cos(float x){ return f_sin(x+1.57079633f); }
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

static void leg_ik(float x,float z,float *thigh,float *calf){
    float d2=x*x+z*z, d=f_sqrt(d2);
    if(d>L1+L2-1.0f){ float s=(L1+L2-1.0f)/d; x*=s; z*=s; d2=x*x+z*z; d=f_sqrt(d2); }
    float kn=f_acos((d2-L1*L1-L2*L2)/(2.0f*L1*L2));
    float sh=f_atan2(x,-z)+f_acos((d2+L1*L1-L2*L2)/(2.0f*L1*d));
    *thigh=sh*57.29578f-45.0f; *calf=kn*57.29578f-90.0f;
}

/* leg -> {shoulder servo, knee servo} */
static const int SH[4]={2,5,8,11}, KN[4]={3,6,9,12};
/* trot: FR+RL together, FL+RR together */
static const float PH[4]={0.0f,0.5f,0.5f,0.0f};

void on_start(void){
    if(mpx_abi_version()!=MPX_ABI_VERSION){ MPX_LOG("rebuild against this robot's SDK"); return; }
    MPX_LOG("four-leg trot, default motor tuning");

    for(int i=0;i<FRAMES;i++){
        float t=(float)i/40.0f;                       /* 40 frames per cycle */
        for(int leg=0; leg<4; leg++){
            float p=t+PH[leg]; p-=(float)(int)p;      /* fractional phase 0..1 */
            float x,z;
            if(p<0.5f){                                /* stance: push back     */
                x = STRIDE*(1.0f-4.0f*p);  z = STAND_Z;
            }else{                                     /* swing: lift and reach */
                float s=(p-0.5f)*2.0f;
                x = STRIDE*(2.0f*s-1.0f);  z = STAND_Z + LIFT*f_sin(s*3.14159265f);
            }
            float sh,kn; leg_ik(x,z,&sh,&kn);
            robot_set_servo_angle(SH[leg],(int)(sh*100.0f));
            robot_set_servo_angle(KN[leg],(int)(kn*100.0f));
        }
        robot_flush();
        robot_delay_ms(DT_MS);
    }
    MPX_LOG("done");
}
