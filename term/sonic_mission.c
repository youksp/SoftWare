#include <stdio.h>
#include <unistd.h>
#include <pigpiod_if2.h>

#define TRIG_PINNO 13
#define ECHO_PINNO 16

#define TRIG_PINN1 19
#define ECHO_PINN1 20

#define TRIG_PINN2 26
#define ECHO_PINN2 21

//void trigger(void);
void cb_func_echo0(int pi, unsigned gpio, unsigned level, uint32_t tick);
void cb_func_echo1(int pi, unsigned gpio, unsigned level, uint32_t tick);
void cb_func_echo2(int pi, unsigned gpio, unsigned level, uint32_t tick);

uint32_t start_tick_0, dist_tick_0, start_tick_1, dist_tick_1, start_tick_2, dist_tick_2;

int main()
{
    float distance;
    int pi;

    if ((pi = pigpio_start(NULL, NULL)) < 0) return 1;

    set_mode(pi, TRIG_PINNO, PI_OUTPUT);
    set_mode(pi, ECHO_PINNO, PI_INPUT);

    set_mode(pi, TRIG_PINN1, PI_OUTPUT);
    set_mode(pi, ECHO_PINN1, PI_INPUT);
 
    set_mode(pi, TRIG_PINN2, PI_OUTPUT);
    set_mode(pi, ECHO_PINN2, PI_INPUT);
 
    callback(pi, ECHO_PINNO, EITHER_EDGE, cb_func_echo0);
    gpio_write(pi, TRIG_PINNO, PI_OFF);

    callback(pi, ECHO_PINN1, EITHER_EDGE, cb_func_echo1);
    gpio_write(pi, TRIG_PINN1, PI_OFF);
 
    callback(pi, ECHO_PINN2, EITHER_EDGE, cb_func_echo2);
    gpio_write(pi, TRIG_PINN2, PI_OFF);
 
    time_sleep(1);     // delay 1 second

    printf("Raspberry Pi HC-SR04 UltraSonic sensor\n" );

    while(1){
        start_tick_0 = dist_tick_0 = 0;
        start_tick_0 = dist_tick_0 = 0;
        start_tick_0 = dist_tick_0 = 0;
        gpio_trigger(pi, TRIG_PINNO, 10, PI_HIGH); 
        gpio_trigger(pi, TRIG_PINN1, 10, PI_HIGH);
        gpio_trigger(pi, TRIG_PINN2, 10, PI_HIGH);
        
        time_sleep(0.05);

        if(dist_tick_0 && start_tick_0){
            //distance = (float)(dist_tick_) / 58.8235;
            distance = dist_tick_0 / 1000000. * 340 / 2 * 100;
            if(distance < 2 || distance > 400)
                printf("range error");
            else 
                printf("Distance0 : %6.1f cm", dist_tick_0, distance);
        }
        else
            printf("sense error");


         if(dist_tick_1 && start_tick_1){
            //distance = (float)(dist_tick_) / 58.8235;
            distance = dist_tick_1 / 1000000. * 340 / 2 * 100;
            if(distance < 2 || distance > 400)
                printf("   range error");
            else 
                printf("   Distance : %6.1f cm", dist_tick_1, distance);
        }
        else
            printf("   sense error");
        
        if(dist_tick_2 && start_tick_2){
            //distance = (float)(dist_tick_) / 58.8235;
            distance = dist_tick_2 / 1000000. * 340 / 2 * 100;
            if(distance < 2 || distance > 400)
                printf("   range error\n");
            else 
                printf("   Distance : %6.1f cm\n", dist_tick_2, distance);
        }
        else
            printf("   sense error\n");
        time_sleep(0.1);
    }
    pigpio_stop(pi);

    return 0;
}

void cb_func_echo0(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_0 = tick;
    else if(level == PI_LOW)
        dist_tick_0 = tick - start_tick_0;
}

void cb_func_echo1(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_1 = tick;
    else if(level == PI_LOW)
        dist_tick_1 = tick - start_tick_1;
}
void cb_func_echo2(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_2 = tick;
    else if(level == PI_LOW)
        dist_tick_2 = tick - start_tick_2;
}

