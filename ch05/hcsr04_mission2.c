#include <stdio.h>
#include <unistd.h>
#include <pigpiod_if2.h>

#define TRIG_PINNO 16
#define ECHO_PINNO 6
#define LED 17

void trigger(void);
void cb_func_echo(int pi, unsigned gpio, unsigned level, uint32_t tick);

uint32_t start_tick_, dist_tick_;

int main()
{
    float distance;
    int pi;
    int d_high,d_low;

    if ((pi = pigpio_start(NULL, NULL)) < 0) return 1;
    printf("LOW : ");
    scanf("%d",&d_low);
    printf("HIGH : ");
    scanf("%d",&d_high);
    set_mode(pi, TRIG_PINNO, PI_OUTPUT);
    set_mode(pi, ECHO_PINNO, PI_INPUT);
    set_mode(pi, LED, PI_OUTPUT);

    callback(pi, ECHO_PINNO, EITHER_EDGE, cb_func_echo);
    gpio_write(pi, TRIG_PINNO, PI_OFF);
    time_sleep(1);     // delay 1 second

    printf("Raspberry Pi HC-SR04 UltraSonic sensor\n" );

    while(1){
        start_tick_ = dist_tick_ = 0;
        gpio_trigger(pi, TRIG_PINNO, 10, PI_HIGH);
        time_sleep(0.05);

        if(dist_tick_ && start_tick_){
            //distance = (float)(dist_tick_) / 58.8235;
            distance = dist_tick_ / 1000000. * 340 / 2 * 100;
            if(distance < 2 || distance > 400)
                printf("range error\n");
            else 
                printf("interval : %6dus, Distance : %6.1f cm\n", dist_tick_, distance);
        }
        else
            printf("sense error\n");
        
        if((distance >= d_low) &&(distance <= d_high))
        {
            gpio_write(pi, LED, PI_ON);
        }
        else
        {   
            gpio_write(pi, LED, PI_OFF);
        }


        time_sleep(0.1);
    }
    pigpio_stop(pi);

    return 0;
}

void cb_func_echo(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_ = tick;
    else if(level == PI_LOW)
        dist_tick_ = tick - start_tick_;
}
