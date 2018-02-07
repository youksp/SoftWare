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

int ref_speed   =  0;
int spin_speed  =  35;
int left_end    =  0;
int right_end   =  0;

int c_flag      =  0;

uint32_t start_tick_[3], dist_tick_[3];
float sensor_L, sensor_F, sensor_R;

float error = 0;
float kp = 10;
float kd = 1;
float pre_error = 0;
float ref_sensor = 4;


//-----------------------------------------------------
//                  function set
//-----------------------------------------------------
void global_Init();
void Motor_Init();

void cb_func_echo0(int pi, unsigned gpio, unsigned level, uint32_t tick);
void cb_func_echo1(int pi, unsigned gpio, unsigned level, uint32_t tick);
void cb_func_echo2(int pi, unsigned gpio, unsigned level, uint32_t tick);
void cb_control(int pi, unsigned gpio, unsigned level, uint32_t tick); //제어주기 50ms

int control_flag(float s_l, float s_f, float s_r); //왼쪽, 정면, 오른쪽(좌수법 알고리즘)

void Motor_front(int left, int right);
void Motor_left_turn();
void Motor_right_turn();
void Motor_smusse_stop(int left, int right);

void senser();
//-----------------------------------------------------
//                       main
//-----------------------------------------------------
int main(int argc, char* argv[])
{
    if(argc == 2) ref_speed = atoi(argv[1]);
    else{
        printf("./a.out [reference speed]\n");
        return -1;
    }
    global_Init();
    Motor_Init();

    callback(pi, ECHO_PINN0, EITHER_EDGE, cb_func_echo0);
    gpio_write(pi, TRIG_PINN0, PI_OFF);

    callback(pi, ECHO_PINN1, EITHER_EDGE, cb_func_echo1);
    gpio_write(pi, TRIG_PINN1, PI_OFF);
                 
    callback(pi, ECHO_PINN2, EITHER_EDGE, cb_func_echo2);
    gpio_write(pi, TRIG_PINN2, PI_OFF);

    time_sleep(1);

    callback(pi,control_cycle_PIN, RISING_EDGE, cb_control);


    while(1);

    pigpio_stop(pi);
    

    return 0;
}
//----------------------------------------------------
//                  control function
//----------------------------------------------------
void cb_control(int pi, unsigned gpio, unsigned level, uint32_t tick) //제어주기 50ms
{
    senser();

//    c_flag = control_flag(sensor_L, sensor_F,sensor_R);
    printf("sensor_L : %.2f, sensor_F : %.2f, sensor_R : %.2f\n",sensor_L, sensor_F, sensor_R);
    error = ref_sensor - sensor_L;
    
    //controller
    if(error >= 0){
        left_end = ref_speed - kp*error - kd*(error - pre_error); 
        right_end = ref_speed;
    }
    else{
        left_end = ref_speed;
        right_end = ref_speed + kp*error + kd*(error - pre_error);
    }
    printf("M_L : %d, M_R : %d\n",left_end,right_end);
    pre_error = error;
}


int control_flag(float s_l, float s_f, float s_r) //왼쪽, 정면, 오른쪽(좌수법 알고리즘)
{
    // 1 = 벽, 0 = 빈공간
    if(s_l > 12)
        s_l = 0;
    else
        s_l = 1;
    
    if(s_f > 12)
        s_f = 0;
    else
        s_f = 1;
    
    if(s_r > 12)
        s_r = 0;
    else
        s_r = 1;

    if((s_l == 0)&&(s_l == 0)&&(s_l == 0)){

    }
    else if((s_l == 0)&&(s_l == 0)&&(s_l == 0)){

    }






    return 0;
}

//-----------------------------------------------------
//                      function
//-----------------------------------------------------
void global_Init()
{
    // pigpio demon start
    if((pi = pigpio_start(NULL, NULL)) < 0){
        fprintf(stderr, "%s\n",pigpio_error(pi)); exit(-1);
    }

    set_PWM_frequency(pi, control_cycle_PIN, 20); // 50ms control period
    set_PWM_range(pi, control_cycle_PIN, 2);
    set_PWM_dutycycle(pi, control_cycle_PIN, 1);

    //pwm range 1000
    set_PWM_range(pi, PWM_PIN0, 100);
    set_PWM_range(pi, PWM_PIN1, 100);
    set_PWM_dutycycle(pi, PWM_PIN0, 0);
    set_PWM_dutycycle(pi, PWM_PIN1, 0);

    set_mode(pi, TRIG_PINN0, PI_OUTPUT);
    set_mode(pi, ECHO_PINN0, PI_INPUT);

    set_mode(pi, TRIG_PINN1, PI_OUTPUT);
    set_mode(pi, ECHO_PINN1, PI_INPUT);

    set_mode(pi, TRIG_PINN2, PI_OUTPUT);
    set_mode(pi, ECHO_PINN2, PI_INPUT);

}

void Motor_Init()
{
    set_mode(pi, INPUT1, PI_OUTPUT);    set_mode(pi, INPUT2, PI_OUTPUT);
    set_mode(pi, INPUT3, PI_OUTPUT);    set_mode(pi, INPUT4, PI_OUTPUT);

    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);
}

void cb_func_echo0(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_[0] = tick;
    else if(level == PI_LOW)
        dist_tick_[0] = tick - start_tick_[0];
}
void cb_func_echo1(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_[1] = tick;
    else if(level == PI_LOW)
        dist_tick_[1] = tick - start_tick_[1];
}
void cb_func_echo2(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_[2] = tick;
    else if(level == PI_LOW)
        dist_tick_[2] = tick - start_tick_[2];
}


void Motor_front(int left, int right)
{
    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, left);
    set_PWM_dutycycle(pi, PWM_PIN1, right);

    gpio_write(pi, INPUT1, PI_HIGH);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_HIGH);     gpio_write(pi, INPUT4, PI_LOW);
 
}
void Motor_left_turn()
{
    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, spin_speed);
    set_PWM_dutycycle(pi, PWM_PIN1, spin_speed);

    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_HIGH);
    gpio_write(pi, INPUT3, PI_HIGH);     gpio_write(pi, INPUT4, PI_LOW);
               
}
void Motor_right_turn()
{
    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, spin_speed);
    set_PWM_dutycycle(pi, PWM_PIN1, spin_speed);

    gpio_write(pi, INPUT1, PI_HIGH);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_HIGH);
               
}
void Motor_smusse_stop(int left, int right)
{
    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, left);
    set_PWM_dutycycle(pi, PWM_PIN1, right);

    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
}

void senser()
{
    int i;

    for(i =0; i < 3; i++)
    {
        start_tick_[i] = 0;
        dist_tick_[i] = 0;
    }
    gpio_trigger(pi, TRIG_PINN0, 10, PI_HIGH); 
    gpio_trigger(pi, TRIG_PINN1, 10, PI_HIGH);
    gpio_trigger(pi, TRIG_PINN2, 10, PI_HIGH);

    time_sleep(0.02);

    if(dist_tick_[0] && start_tick_[0]){
        sensor_L = dist_tick_[0] / 1000000. * 340 / 2 * 100;
    }
    else
        sensor_L = 4;
    
    if(dist_tick_[1] && start_tick_[1]){
        sensor_F = dist_tick_[1] / 1000000. * 340 / 2 * 100;
    }
    else
        sensor_F = -1;
    
    if(dist_tick_[2] && start_tick_[2]){
        sensor_R = dist_tick_[2] / 1000000. * 340 / 2 * 100;
    }
    else
        sensor_R = -1;
}






