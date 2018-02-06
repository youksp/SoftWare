#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <pigpiod_if2.h>


//-----------------------------------------------------
//                      define
//-----------------------------------------------------
#define INPUT1 5
#define INPUT2 6
#define INPUT3 17
#define INPUT4 27

#define PWM_PIN0 23
#define PWM_PIN1 24

#define TRIG_PINN0 13
#define ECHO_PINN0 16

#define TRIG_PINN1 19
#define ECHO_PINN1 20

#define TRIG_PINN2 26
#define ECHO_PINN2 21

#define control_cycle_PIN 12


//-----------------------------------------------------
//                      global var
//-----------------------------------------------------
int pi;

int ref_speed =0;


//-----------------------------------------------------
//                  function set
//-----------------------------------------------------
void global_Init();
void Motor_Init();

void cb_func_echo(int pi, unsigned gpio, unsigned level, uint32_t tick);
void cb_control(int pi, unsigned gpio, unsigned level, uint32_t tick); //제어주기 40ms

//-----------------------------------------------------
//                       main
//-----------------------------------------------------
int main(int argc, char* argv[])
{
    if(argc == 2) ref_speed = atoi(argv[1]);
    else{
        prinf("./a.out [reference speed]\n");
        return -1;
    }

    global_Init();
    Motor_Init();

    return 0;
}

//-----------------------------------------------------
//                      function
//-----------------------------------------------------
void global_Init()
{
    // pigpio demon start
    if((pi = pigpio_start(NULL, NULL)) < 0){
        fprintf(stderr, "%s\n",pigpio_serror(pi)); exit(-1);
    }

    set_PWM_frequency(pi, control_cycle_PIN, 20); // 50ms control period
    set_PWM_range(pi, control_cycle_PIN, 2);
    set_PWM_dutycycle(pi, control_cycle_PIN, 1);

}

void Motoer_Init()
{
    
}
