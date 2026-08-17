#ifndef MPX_DANCE_MOVES_H
#define MPX_DANCE_MOVES_H
float sinf(float);
float cosf(float);
#include "mpx_host.h"
typedef enum {
MOVE_NONE=0,MOVE_STOP=1,MOVE_HOLD=2,
MOVE_LOOK_UP=3,MOVE_LOOK_DOWN=4,MOVE_LOOK_LEFT=5,MOVE_LOOK_RIGHT=6,
MOVE_LOOK_UPPERLEFT=7,MOVE_LOOK_UPPERRIGHT=8,MOVE_LOOK_LOWERLEFT=9,MOVE_LOOK_LOWERRIGHT=10,
MOVE_NOD=11,MOVE_HEADBANG=12,MOVE_HEAD_ELLIPSE=13,MOVE_SEEK=14,
MOVE_SHOULDER_SHRUG=16,MOVE_RIGHT_SHOULDER_SHRUG=17,MOVE_LEFT_SHOULDER_SHRUG=18,
MOVE_BUTT_SHRUG=22,MOVE_RIGHT_BUTT_SHRUG=23,MOVE_LEFT_BUTT_SHRUG=24,
MOVE_BACKLEG_LIFT=25,MOVE_FRONT_KICK=26,
MOVE_LEAN_LEFT=27,MOVE_LEAN_RIGHT=28,MOVE_BODY_ROW=29,MOVE_SWAGGER=30,
MOVE_WIGGLE=31,MOVE_LEFT_WIGGLE=32,MOVE_RIGHT_WIGGLE=33,MOVE_BODY_ELLIPSE=34,
MOVE_RAISE_BODY=35,MOVE_LOWER_BODY=36,MOVE_SQUAT=37,MOVE_BOUNCE=38,
MOVE_GREET=39,MOVE_STRETCH=40,MOVE_ROLL_BODY=41,MOVE_PITCH_BODY=42,
MOVE_DIP=64,MOVE_SPIN=65,
MOVE_TWERK=128,MOVE_ADVANCE=129,MOVE_BACK=130,MOVE_LEFT=131,MOVE_RIGHT=132,
MOVE_TURN_LEFT=133,MOVE_TURN_RIGHT=134,MOVE_JUMP=135,MOVE_STEP=136,MOVE_INIT=137
} move_id_t;
typedef struct {unsigned char cmd;short param;unsigned short delay_ms;} __attribute__((packed)) timetable_entry_t;
static void _move_look_up(short p){int a=p>0?p:20;robot_set_servo_angle(SERVO_FR_SHOULDER,a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,-a*50);robot_set_servo_angle(SERVO_RL_SHOULDER,-a*50);robot_flush();}
static void _move_look_down(short p){int a=p>0?p:20;robot_set_servo_angle(SERVO_FR_SHOULDER,-a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,-a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,a*50);robot_set_servo_angle(SERVO_RL_SHOULDER,a*50);robot_flush();}
static void _move_look_left(short p){int a=p>0?p:30;robot_set_servo_angle(SERVO_FR_HIP,a*70);robot_set_servo_angle(SERVO_FL_HIP,-a*70);robot_set_servo_angle(SERVO_RR_HIP,a*70);robot_set_servo_angle(SERVO_RL_HIP,-a*70);robot_flush();}
static void _move_look_right(short p){int a=p>0?p:30;robot_set_servo_angle(SERVO_FR_HIP,-a*70);robot_set_servo_angle(SERVO_FL_HIP,a*70);robot_set_servo_angle(SERVO_RR_HIP,-a*70);robot_set_servo_angle(SERVO_RL_HIP,a*70);robot_flush();}
static void _move_look_upperleft(short p){int a=p>0?p:20;robot_set_servo_angle(SERVO_FR_HIP,a*50);robot_set_servo_angle(SERVO_FL_HIP,-a*50);robot_set_servo_angle(SERVO_RR_HIP,a*50);robot_set_servo_angle(SERVO_RL_HIP,-a*50);robot_set_servo_angle(SERVO_FR_SHOULDER,a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,-a*50);robot_set_servo_angle(SERVO_RL_SHOULDER,-a*50);robot_flush();}
static void _move_look_upperright(short p){int a=p>0?p:20;robot_set_servo_angle(SERVO_FR_HIP,-a*50);robot_set_servo_angle(SERVO_FL_HIP,a*50);robot_set_servo_angle(SERVO_RR_HIP,-a*50);robot_set_servo_angle(SERVO_RL_HIP,a*50);robot_set_servo_angle(SERVO_FR_SHOULDER,a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,-a*50);robot_set_servo_angle(SERVO_RL_SHOULDER,-a*50);robot_flush();}
static void _move_look_lowerleft(short p){int a=p>0?p:20;robot_set_servo_angle(SERVO_FR_HIP,a*50);robot_set_servo_angle(SERVO_FL_HIP,-a*50);robot_set_servo_angle(SERVO_RR_HIP,a*50);robot_set_servo_angle(SERVO_RL_HIP,-a*50);robot_set_servo_angle(SERVO_FR_SHOULDER,-a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,-a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,a*50);robot_set_servo_angle(SERVO_RL_SHOULDER,a*50);robot_flush();}
static void _move_look_lowerright(short p){int a=p>0?p:20;robot_set_servo_angle(SERVO_FR_HIP,-a*50);robot_set_servo_angle(SERVO_FL_HIP,a*50);robot_set_servo_angle(SERVO_RR_HIP,-a*50);robot_set_servo_angle(SERVO_RL_HIP,a*50);robot_set_servo_angle(SERVO_FR_SHOULDER,-a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,-a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,a*50);robot_set_servo_angle(SERVO_RL_SHOULDER,a*50);robot_flush();}
static void _move_nod(short p){int a=p>0?p:15;robot_set_servo_angle(SERVO_FR_SHOULDER,-a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,-a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,a*50);robot_set_servo_angle(SERVO_RL_SHOULDER,a*50);robot_flush();robot_delay_ms(100);robot_set_servo_angle(SERVO_FR_SHOULDER,a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,-a*50);robot_set_servo_angle(SERVO_RL_SHOULDER,-a*50);robot_flush();robot_delay_ms(100);}
static void _move_headbang(short p){int n=p>0?p:4;for(int i=0;i<n;i++){robot_set_servo_angle(SERVO_FR_SHOULDER,2000);robot_set_servo_angle(SERVO_FL_SHOULDER,2000);robot_set_servo_angle(SERVO_RR_SHOULDER,-1000);robot_set_servo_angle(SERVO_RL_SHOULDER,-1000);robot_flush();robot_delay_ms(80);robot_set_servo_angle(SERVO_FR_SHOULDER,-2000);robot_set_servo_angle(SERVO_FL_SHOULDER,-2000);robot_set_servo_angle(SERVO_RR_SHOULDER,1000);robot_set_servo_angle(SERVO_RL_SHOULDER,1000);robot_flush();robot_delay_ms(80);}}
static void _move_head_ellipse(short p){(void)p;for(int ri=0;ri<360;ri+=30){float rd=(float)ri*3.14159f/180.0f;int p2=(int)(1500.0f*sinf(rd));int y=(int)(1000.0f*cosf(rd));robot_set_servo_angle(SERVO_FR_SHOULDER,p2);robot_set_servo_angle(SERVO_FL_SHOULDER,p2);robot_set_servo_angle(SERVO_RR_SHOULDER,-p2/2);robot_set_servo_angle(SERVO_RL_SHOULDER,-p2/2);robot_set_servo_angle(SERVO_FR_HIP,-y);robot_set_servo_angle(SERVO_FL_HIP,y);robot_set_servo_angle(SERVO_RR_HIP,-y);robot_set_servo_angle(SERVO_RL_HIP,y);robot_flush();robot_delay_ms(60);}}
static void _move_seek(short p){int a=p>0?p:20;_move_look_left(a);robot_delay_ms(300);_move_look_right(a);robot_delay_ms(300);_move_look_up(a);robot_delay_ms(300);_move_look_down(a);robot_delay_ms(300);}
static void _move_shrug(short p){int a=p>0?p:10;robot_set_servo_angle(SERVO_FR_SHOULDER,a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,a*100);robot_flush();}
static void _move_rshrug(short p){int a=p>0?p:10;robot_set_servo_angle(SERVO_FR_SHOULDER,a*100);robot_flush();}
static void _move_lshrug(short p){int a=p>0?p:10;robot_set_servo_angle(SERVO_FL_SHOULDER,a*100);robot_flush();}
static void _move_butt(short p){int a=p>0?p:10;robot_set_servo_angle(SERVO_RR_SHOULDER,a*100);robot_set_servo_angle(SERVO_RL_SHOULDER,a*100);robot_flush();}
static void _move_rbutt(short p){int a=p>0?p:10;robot_set_servo_angle(SERVO_RR_SHOULDER,a*100);robot_flush();}
static void _move_lbutt(short p){int a=p>0?p:10;robot_set_servo_angle(SERVO_RL_SHOULDER,a*100);robot_flush();}
static void _move_backleg(short p){int a=p>0?p:15;robot_set_servo_angle(SERVO_RR_KNEE,a*100);robot_set_servo_angle(SERVO_RL_KNEE,a*100);robot_flush();}
static void _move_kick(short p){int a=p>0?p:20;robot_set_servo_angle(SERVO_FR_SHOULDER,-a*100);robot_set_servo_angle(SERVO_FR_KNEE,-a*100);robot_flush();}
static void _move_lean_left(short p){int a=p>0?p:10;robot_set_servo_angle(SERVO_FR_SHOULDER,a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,-a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,a*100);robot_set_servo_angle(SERVO_RL_SHOULDER,-a*100);robot_flush();}
static void _move_lean_right(short p){int a=p>0?p:10;robot_set_servo_angle(SERVO_FR_SHOULDER,-a*100);robot_set_servo_angle(SERVO_FL_SHOULDER,a*100);robot_set_servo_angle(SERVO_RR_SHOULDER,-a*100);robot_set_servo_angle(SERVO_RL_SHOULDER,a*100);robot_flush();}
static void _move_row(short p){int a=p>0?p:5;_move_lean_left(a);robot_delay_ms(150);_move_lean_right(a);robot_delay_ms(150);}
static void _move_swagger(short p){int n=p>0?p:3;for(int i=0;i<n;i++){int ai=5+(i%3)*3;robot_set_servo_angle(SERVO_FR_SHOULDER,ai*100);robot_set_servo_angle(SERVO_FL_SHOULDER,-ai*100);robot_set_servo_angle(SERVO_RR_SHOULDER,ai*100);robot_set_servo_angle(SERVO_RL_SHOULDER,-ai*100);robot_flush();robot_delay_ms(200);robot_set_servo_angle(SERVO_FR_SHOULDER,-ai*100);robot_set_servo_angle(SERVO_FL_SHOULDER,ai*100);robot_set_servo_angle(SERVO_RR_SHOULDER,-ai*100);robot_set_servo_angle(SERVO_RL_SHOULDER,ai*100);robot_flush();robot_delay_ms(200);}}
static void _move_wiggle(short p){int n=p>0?p:6;for(int i=0;i<n;i++){robot_set_servo_angle(SERVO_FR_SHOULDER,500);robot_set_servo_angle(SERVO_FL_SHOULDER,-500);robot_set_servo_angle(SERVO_RR_SHOULDER,500);robot_set_servo_angle(SERVO_RL_SHOULDER,-500);robot_flush();robot_delay_ms(60);robot_set_servo_angle(SERVO_FR_SHOULDER,-500);robot_set_servo_angle(SERVO_FL_SHOULDER,500);robot_set_servo_angle(SERVO_RR_SHOULDER,-500);robot_set_servo_angle(SERVO_RL_SHOULDER,500);robot_flush();robot_delay_ms(60);}}
static void _move_left_wiggle(short p){int n=p>0?p:4;for(int i=0;i<n;i++){robot_set_servo_angle(SERVO_FL_SHOULDER,800);robot_set_servo_angle(SERVO_RL_SHOULDER,800);robot_flush();robot_delay_ms(80);robot_set_servo_angle(SERVO_FL_SHOULDER,-800);robot_set_servo_angle(SERVO_RL_SHOULDER,-800);robot_flush();robot_delay_ms(80);}}
static void _move_right_wiggle(short p){int n=p>0?p:4;for(int i=0;i<n;i++){robot_set_servo_angle(SERVO_FR_SHOULDER,800);robot_set_servo_angle(SERVO_RR_SHOULDER,800);robot_flush();robot_delay_ms(80);robot_set_servo_angle(SERVO_FR_SHOULDER,-800);robot_set_servo_angle(SERVO_RR_SHOULDER,-800);robot_flush();robot_delay_ms(80);}}
static void _move_body_ellipse(short p){(void)p;for(int ri=0;ri<360;ri+=30){float rd=(float)ri*3.14159f/180.0f;int rl=(int)(1500.0f*sinf(rd));int pt=(int)(1000.0f*cosf(rd));robot_set_servo_angle(SERVO_FR_SHOULDER,pt+rl);robot_set_servo_angle(SERVO_FL_SHOULDER,pt-rl);robot_set_servo_angle(SERVO_RR_SHOULDER,-pt+rl);robot_set_servo_angle(SERVO_RL_SHOULDER,-pt-rl);robot_flush();robot_delay_ms(80);}}
static void _move_bounce(short p){int n=p>0?p:3;for(int i=0;i<n;i++){robot_set_config(80,80,15,10,10);robot_delay_ms(150);robot_set_config(80,50,15,10,10);robot_delay_ms(150);}robot_set_config(100,70,15,10,10);}
static void _move_greet(short p){(void)p;robot_set_servo_angle(SERVO_FR_SHOULDER,2000);robot_set_servo_angle(SERVO_FR_KNEE,-3000);robot_flush();robot_delay_ms(500);robot_set_servo_angle(SERVO_FR_SHOULDER,0);robot_set_servo_angle(SERVO_FR_KNEE,0);robot_flush();}
static void _move_dip(short p){(void)p;robot_set_servo_angle(SERVO_FR_SHOULDER,0);robot_set_servo_angle(SERVO_FL_SHOULDER,0);robot_set_servo_angle(SERVO_RR_SHOULDER,0);robot_set_servo_angle(SERVO_RL_SHOULDER,0);robot_set_config(100,50,15,10,10);robot_flush();robot_delay_ms(100);robot_set_servo_angle(SERVO_FR_SHOULDER,-1500);robot_set_servo_angle(SERVO_FL_SHOULDER,-1500);robot_set_servo_angle(SERVO_RR_SHOULDER,1500);robot_set_servo_angle(SERVO_RL_SHOULDER,1500);robot_flush();robot_delay_ms(100);robot_set_servo_angle(SERVO_FR_SHOULDER,0);robot_set_servo_angle(SERVO_FL_SHOULDER,0);robot_set_servo_angle(SERVO_RR_SHOULDER,0);robot_set_servo_angle(SERVO_RL_SHOULDER,0);robot_flush();robot_delay_ms(150);robot_set_config(100,70,15,10,10);robot_flush();robot_delay_ms(100);}
static void _move_spin(short p){(void)p;robot_gait_enum(GAIT_TURN_R);robot_delay_ms(700);robot_gait_enum(GAIT_NONE);robot_delay_ms(100);robot_gait_enum(GAIT_TURN_L);robot_delay_ms(350);robot_gait_enum(GAIT_NONE);}

static void execute_move(move_id_t cmd, short p) {
    if(cmd==MOVE_NONE||cmd==MOVE_STOP)return;
    if(cmd==MOVE_HOLD){robot_delay_ms(p>0?p:500);return;}
    if(cmd==MOVE_LOOK_UP){_move_look_up(p);return;}
    if(cmd==MOVE_LOOK_DOWN){_move_look_down(p);return;}
    if(cmd==MOVE_LOOK_LEFT){_move_look_left(p);return;}
    if(cmd==MOVE_LOOK_RIGHT){_move_look_right(p);return;}
    if(cmd==MOVE_LOOK_UPPERLEFT){_move_look_upperleft(p);return;}
    if(cmd==MOVE_LOOK_UPPERRIGHT){_move_look_upperright(p);return;}
    if(cmd==MOVE_LOOK_LOWERLEFT){_move_look_lowerleft(p);return;}
    if(cmd==MOVE_LOOK_LOWERRIGHT){_move_look_lowerright(p);return;}
    if(cmd==MOVE_NOD){_move_nod(p);return;}
    if(cmd==MOVE_HEADBANG){_move_headbang(p);return;}
    if(cmd==MOVE_HEAD_ELLIPSE){_move_head_ellipse(p);return;}
    if(cmd==MOVE_SEEK){_move_seek(p);return;}
    if(cmd==MOVE_SHOULDER_SHRUG){_move_shrug(p);return;}
    if(cmd==MOVE_RIGHT_SHOULDER_SHRUG){_move_rshrug(p);return;}
    if(cmd==MOVE_LEFT_SHOULDER_SHRUG){_move_lshrug(p);return;}
    if(cmd==MOVE_BUTT_SHRUG){_move_butt(p);return;}
    if(cmd==MOVE_RIGHT_BUTT_SHRUG){_move_rbutt(p);return;}
    if(cmd==MOVE_LEFT_BUTT_SHRUG){_move_lbutt(p);return;}
    if(cmd==MOVE_BACKLEG_LIFT){_move_backleg(p);return;}
    if(cmd==MOVE_FRONT_KICK){_move_kick(p);return;}
    if(cmd==MOVE_LEAN_LEFT){_move_lean_left(p);return;}
    if(cmd==MOVE_LEAN_RIGHT){_move_lean_right(p);return;}
    if(cmd==MOVE_BODY_ROW){_move_row(p);return;}
    if(cmd==MOVE_SWAGGER){_move_swagger(p);return;}
    if(cmd==MOVE_WIGGLE){_move_wiggle(p);return;}
    if(cmd==MOVE_LEFT_WIGGLE){_move_left_wiggle(p);return;}
    if(cmd==MOVE_RIGHT_WIGGLE){_move_right_wiggle(p);return;}
    if(cmd==MOVE_BODY_ELLIPSE){_move_body_ellipse(p);return;}
    if(cmd==MOVE_RAISE_BODY){robot_set_config(100,70+(p>0?p:10),15,10,10);return;}
    if(cmd==MOVE_LOWER_BODY){robot_set_config(100,70-(p>0?p:10),15,10,10);return;}
    if(cmd==MOVE_SQUAT){robot_set_config(100,70-(p>0?p:20),15,10,10);return;}
    if(cmd==MOVE_BOUNCE){_move_bounce(p);return;}
    if(cmd==MOVE_GREET){_move_greet(p);return;}
    if(cmd==MOVE_STRETCH){robot_gait_enum(GAIT_STRETCH);robot_delay_ms(1500);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_ROLL_BODY){robot_gait_enum(GAIT_ROLL);robot_delay_ms(p>0?p:2000);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_PITCH_BODY){robot_gait_enum(GAIT_PITCH);robot_delay_ms(p>0?p:2000);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_DIP){_move_dip(p);return;}
    if(cmd==MOVE_SPIN){_move_spin(p);return;}
    if(cmd==MOVE_TWERK){robot_gait_enum(GAIT_TWERK);robot_delay_ms(p>0?p:1000);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_ADVANCE){robot_gait_enum(GAIT_ADVANCE);robot_delay_ms(p>0?p:500);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_BACK){robot_gait_enum(GAIT_BACK);robot_delay_ms(p>0?p:500);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_LEFT){robot_gait_enum(GAIT_LEFT);robot_delay_ms(p>0?p:500);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_RIGHT){robot_gait_enum(GAIT_RIGHT);robot_delay_ms(p>0?p:500);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_TURN_LEFT){robot_gait_enum(GAIT_TURN_L);robot_delay_ms(p>0?p:500);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_TURN_RIGHT){robot_gait_enum(GAIT_TURN_R);robot_delay_ms(p>0?p:500);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_JUMP){robot_gait_enum(GAIT_JUMP);robot_delay_ms(2000);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_STEP){robot_gait_enum(GAIT_STEP);robot_delay_ms(p>0?p:1000);robot_gait_enum(GAIT_NONE);return;}
    if(cmd==MOVE_INIT){robot_gait_enum(GAIT_INIT);robot_delay_ms(2000);return;}
}
#endif
