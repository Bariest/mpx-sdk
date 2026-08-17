#include "mpx_host.h"
#include "moves.h"
extern const timetable_entry_t TIMETABLE[];
static void log_move(move_id_t cmd, short param) {
    int n = (int)cmd;
    char buf[16];
    int len = 0;
    /* "cmd:" prefix */
    buf[len++] = 'c'; buf[len++] = 'm'; buf[len++] = 'd'; buf[len++] = ':';
    /* print cmd as decimal */
    if(n == 0) { buf[len++] = '0'; }
    else {
        char tmp[8]; int tlen = 0;
        while(n > 0 && tlen < 8) { tmp[tlen++] = '0' + (n % 10); n /= 10; }
        for(int i = tlen-1; i >= 0; i--) buf[len++] = tmp[i];
    }
    buf[len++] = '(';
    /* print param */
    if(param < 0) { buf[len++] = '-'; param = -param; }
    if(param == 0) { buf[len++] = '0'; }
    else {
        char tmp[8]; int tlen = 0;
        while(param > 0 && tlen < 8) { tmp[tlen++] = '0' + (param % 10); param /= 10; }
        for(int i = tlen-1; i >= 0; i--) buf[len++] = tmp[i];
    }
    buf[len++] = ')'; buf[len++] = '\n';
    MPX_print(buf, len);
}
void on_start(void) {
    robot_gait_enum(GAIT_INIT); robot_delay_ms(2000);
    for(int i=0;i<TIMETABLE_ENTRIES;i++){
        if(TIMETABLE[i].delay_ms>0) robot_delay_ms(TIMETABLE[i].delay_ms);
        log_move((move_id_t)TIMETABLE[i].cmd,TIMETABLE[i].param);
        execute_move((move_id_t)TIMETABLE[i].cmd,TIMETABLE[i].param);
    }
    robot_gait_enum(GAIT_INIT); robot_delay_ms(2000); robot_gait_enum(GAIT_NONE);
}
