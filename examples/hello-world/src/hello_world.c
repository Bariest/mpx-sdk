/* hello_world.c — the smallest skill that visibly does something.
 *
 * Every skill is one exported function, on_start(), which the robot calls
 * once. When it returns, the skill is over. There is no loop to set up and
 * nothing to initialise.
 *
 *     cd examples/hello-world
 *     mpx-cli deploy
 *
 * That builds it, uploads it over Wi-Fi and runs it. Read the four lines in
 * on_start() below and you know most of what there is to know.
 */
#include "mpx_host.h"

void on_start(void)
{
    /* Anything you print lands in the robot's log — `mpx-cli logs` shows it.
     * MPX_LOG() is for string literals; MPX_print(str, len) takes a variable. */
    MPX_LOG("hello from my first skill");

    /* Gaits are the high-level way to move. The robot walks until you tell it
     * to stop, so a skill is usually: start a gait, wait, stop it.
     *
     * robot_gait_enum() takes a name from the Gait enum rather than a string,
     * so a typo is a compile error instead of a robot that does nothing. */
    robot_gait_enum(GAIT_ADVANCE);
    robot_delay_ms(2000);              /* walk forward for two seconds       */

    robot_gait_enum(GAIT_NONE);        /* stop walking                       */
    robot_delay_ms(300);               /* let the legs settle before posing  */

    /* GAIT_INIT is the standing pose. Always leave the robot somewhere safe:
     * whatever position you finish in is where it stays. */
    robot_gait_enum(GAIT_INIT);
    robot_delay_ms(800);

    MPX_LOG("done");
}

/* WHERE TO GO NEXT
 *
 *   examples/walk-with-gains   tune the joint stiffness, then walk with it
 *   examples/per-servo-gains   give each joint different gains
 *   examples/lowlevel-servo    drive individual joints directly
 *
 * HOST_FUNCTIONS.md lists everything the robot exposes, in all three
 * languages. Two things worth knowing before you go further:
 *
 *   - Your skill is stopped after 60 seconds, whatever it is doing.
 *   - robot_delay_ms() is the only way to wait. A busy-loop will not yield,
 *     so the robot cannot service anything else while you spin.
 */
