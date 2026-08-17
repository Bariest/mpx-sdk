#ifndef MPX_HOST_H
#define MPX_HOST_H
#ifdef __cplusplus
extern "C" {
#endif
extern void print(int ptr, int len);
static inline void MPX_print(const char *str, int len) { print((int)str, len); }
#define MPX_LOG(msg) MPX_print((msg),(int)(sizeof(msg)-1))
extern void robot_gait(int name_ptr);
extern int robot_get_mode(void);
extern void robot_set_body_pose(float roll_deg,float pitch_deg,float yaw_deg);
extern void robot_set_attitude_speed(int dps);
extern void robot_set_attitude_speed_xyz(int roll_dps,int pitch_dps,int yaw_dps);
extern void robot_set_config(int period,int height,int up_height,int stride,int tilt);
extern int robot_get_period(void);
extern int robot_get_height(void);
extern int robot_get_up_height(void);
extern int robot_get_stride(void);
extern int robot_get_tilt(void);
extern void robot_set_servo_angle(int id,int centideg);
extern void robot_flush(void);
extern void robot_set_servo_speed(int id,int speed);
extern int robot_read_position(int id);
extern int robot_read_speed(int id);
extern int robot_read_load(int id);
extern int robot_read_voltage(int id);
extern int robot_read_temperature(int id);
extern int robot_read_moving(int id);
extern int robot_read_current(int id);
extern void robot_set_offset(int id,int centideg);
extern int robot_get_offset(int id);
extern int robot_ping_servo(int id);
extern void robot_delay_ms(int ms);
extern void robot_ik_fr(float x,float th0,float z);
extern void robot_ik_fl(float x,float th0,float z);
extern void robot_ik_rr(float x,float th0,float z);
extern void robot_ik_rl(float x,float th0,float z);
extern void robot_imu_read(int buffer_ptr);
extern void robot_imu_print(void);
typedef enum {GAIT_NONE=0,GAIT_INIT=1,GAIT_STEP=2,GAIT_ROLL=3,GAIT_PITCH=4,GAIT_STRETCH=5,GAIT_ADVANCE=6,GAIT_BACK=7,GAIT_LEFT=8,GAIT_RIGHT=9,GAIT_TURN_L=10,GAIT_TURN_R=11,GAIT_TWERK=12,GAIT_JUMP=13,GAIT_JUMP_FWD=14,GAIT_TEST_SPD=15,GAIT_STANFORD=38,GAIT_FRONT_KICK=39,GAIT_WIGGLE=40,GAIT_BUTT_SHRUG=41} robot_gait_t;
static inline void robot_gait_enum(robot_gait_t g){static const char*n[]={"none","init","step","roll","pitch","stretch","advance","back","left","right","turnL","turnR","twerk","jump","jumpfwd","testspeed"};if(g>=GAIT_NONE&&g<=GAIT_TEST_SPD)robot_gait((int)n[(int)g]);else if(g==GAIT_STANFORD)robot_gait((int)"stanford");else if(g==GAIT_FRONT_KICK)robot_gait((int)"frontkick");else if(g==GAIT_WIGGLE)robot_gait((int)"wiggle");else if(g==GAIT_BUTT_SHRUG)robot_gait((int)"buttshrug");}
typedef enum {SERVO_FR_HIP=1,SERVO_FR_SHOULDER=2,SERVO_FR_KNEE=3,SERVO_FL_HIP=4,SERVO_FL_SHOULDER=5,SERVO_FL_KNEE=6,SERVO_RR_HIP=7,SERVO_RR_SHOULDER=8,SERVO_RR_KNEE=9,SERVO_RL_HIP=10,SERVO_RL_SHOULDER=11,SERVO_RL_KNEE=12} robot_servo_t;
static inline void robot_set_servo_deg(robot_servo_t id,float deg){robot_set_servo_angle((int)id,(int)(deg*100.0f));}
static inline void robot_set_servo(robot_servo_t id,float deg,int speed){robot_set_servo_speed((int)id,speed);robot_set_servo_deg(id,deg);}
static inline void robot_walk_forward(int ms){robot_gait_enum(GAIT_ADVANCE);robot_delay_ms(ms);robot_gait_enum(GAIT_NONE);}
static inline void robot_twerk(int ms){robot_set_config(60,60,15,8,15);robot_gait_enum(GAIT_TWERK);robot_delay_ms(ms);robot_gait_enum(GAIT_NONE);}
static inline void robot_jump(void){robot_gait_enum(GAIT_JUMP);robot_delay_ms(2000);robot_gait_enum(GAIT_NONE);}
static inline void robot_stand(void){robot_gait_enum(GAIT_INIT);robot_delay_ms(2000);}
static inline void robot_stanford_walk(int ms){robot_gait_enum(GAIT_STANFORD);robot_delay_ms(ms);robot_gait_enum(GAIT_NONE);}
static inline void robot_front_kick(void){robot_gait_enum(GAIT_FRONT_KICK);robot_delay_ms(2000);}
static inline void robot_wiggle(int ms){robot_gait_enum(GAIT_WIGGLE);robot_delay_ms(ms);robot_gait_enum(GAIT_NONE);}
static inline void robot_butt_shrug(int ms){robot_gait_enum(GAIT_BUTT_SHRUG);robot_delay_ms(ms);robot_gait_enum(GAIT_NONE);}
static inline void robot_attitude(float r,float p,float y){robot_set_body_pose(r,p,y);}
static inline void robot_roll(float deg){robot_set_body_pose(deg,0.0f,0.0f);}
static inline void robot_pitch(float deg){robot_set_body_pose(0.0f,deg,0.0f);}
static inline void robot_yaw(float deg){robot_set_body_pose(0.0f,0.0f,deg);}
static inline void robot_reset_attitude(void){robot_gait((int)"none");}
static inline void robot_attitude_speed(float dps){robot_set_attitude_speed((int)dps);}
static inline void robot_attitude_speed_xyz(float r,float p,float y){robot_set_attitude_speed_xyz((int)r,(int)p,(int)y);}
#ifdef __cplusplus
}
#endif
#endif
