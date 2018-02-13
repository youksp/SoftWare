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
#define speed_limit 350
#define spin_speed 350
//-----------------------------------------------------
//                      global var
//-----------------------------------------------------
int pi;

int ref_speed   =  0;   //설정속도

int left_end    =  0;   //왼쪽 모터 pwm값
int right_end   =  0;   //오른쪽 모터 pwm값

int c_flag      =  0;   //좌수법 알고리즘에 의한 현재상황 플래그

uint32_t start_tick_[3], dist_tick_[3];
float sensor_L, sensor_F, sensor_R; //현재 거리값
float s_save_L, s_save_F, s_save_R; //사용되는 거리값

float error_L = 0;
float error_R = 0;
float error_F = 0;

float kp = 10.5;    //왼쪽 모터 게인값
float kd = 2;

float kp_r = 9;     //오른쪽 모터 게인값
float kd_r = 10;

float kp_f = 8;     //정면 게인값

float pre_error_L = 0;  //
float pre_error_R = 0;


float ref_sensor = 5.2;     //좌, 우 이격 전진 거리
float ref_sensor_F = 15;    //정면 벽 감지 감속 시작 지점

int ns_l, ns_f, ns_r;       //벽 감지 상태 저장 변수
int flag = 2; 
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

void* sensor(void* arg );
//-----------------------------------------------------
//                       main
//-----------------------------------------------------
int main(int argc, char* argv[])
{
    if(argc == 2) ref_speed = 10 * atoi(argv[1]); //out 파일 실행시 기준 속도 입력
    else{
        printf("./a.out [reference speed]\n");
        return -1;
    }

    global_Init(); //pigipio demon, sona, pwm init
    Motor_Init(); //motor pin init



    //초음파 센서를 위한 callback함수 실행
    callback(pi, ECHO_PINN0, EITHER_EDGE, cb_func_echo0);
    gpio_write(pi, TRIG_PINN0, PI_OFF);

    callback(pi, ECHO_PINN1, EITHER_EDGE, cb_func_echo1);
    gpio_write(pi, TRIG_PINN1, PI_OFF);
                 
    callback(pi, ECHO_PINN2, EITHER_EDGE, cb_func_echo2);
    gpio_write(pi, TRIG_PINN2, PI_OFF);

    //time_sleep(1);

    //초음파 센서를 받는 함수 스레드 실행
    start_thread(sensor,(void *)0);

    //50ms제어주기의 callback함수 실행
    callback(pi,control_cycle_PIN, RISING_EDGE, cb_control);
    time_sleep(1);

    //start_thread(sensor,(void *)0);

    while(1);

    pigpio_stop(pi);
    

    return 0;
}
//----------------------------------------------------
//                  control function
//----------------------------------------------------
void cb_control(int pi, unsigned gpio, unsigned level, uint32_t tick) //제어주기 50ms
{
    //센서 오류가 아닌 값만 사용
    if(sensor_L > 0)    s_save_L = sensor_L;
    if(sensor_F > 0)    s_save_F = sensor_F;
    if(sensor_R > 0)    s_save_R = sensor_R;

    c_flag = control_flag(s_save_L, s_save_F, s_save_R);

    // 전진확인 제어변수 1:회전, 2:왼전진, 3:오른전진
    //c_flag = 3;
    printf("sensor_L : %.2f, sensor_F : %.2f, sensor_R : %.2f\n",sensor_L, sensor_F, sensor_R);


    //원하는 거리값을 맞추기위해 (설정거리 - 현재거리) 에러값 생성 
    error_L = ref_sensor - s_save_L;
    error_R = ref_sensor - s_save_R - 0.2;
    error_F = ref_sensor_F - s_save_F;
    if(error_F < 0) error_F = 0; // 정면은 설정거리부터 감속하기위해 음수 삭제
       


    if(c_flag == 1) //회전운동
    {

    }
    else if(c_flag == 2) //전진운동 왼쪽보기
    {

        //확꺽어지지 안도록 제어값 제한 둬보기
/*  
        //controller left look 왼쪽 거리를 맞추며 전진하는 제어
        if(error_L >= 0){
            right_end = ref_speed - ref_speed*(kp*error_L + kd*(error_L - pre_error_L))/100.0 - ref_speed*kp_f*error_F/100.0; 
            left_end = ref_speed - ref_speed*kp_f*error_F/100.0;
            if(left_end < 0) 
                left_end = 0;
            if(right_end < speed_limit - ref_speed*kp_f*error_F/100.0)   //회전 속도 감속 제한
                right_end = speed_limit - ref_speed*kp_f*error_F/100.0;

        }
        else{
            right_end = ref_speed - ref_speed*kp_f*error_F/100.0;
            left_end = ref_speed + ref_speed*(kp*error_L + kd*(error_L - pre_error_L))/100.0 - ref_speed*kp_f*error_F/100.0;
            if(right_end < 0)
                right_end = 0;
            if(left_end < speed_limit - ref_speed*kp_f*error_F/100.0)    //회전 속도 감속 제한 
                left_end = speed_limit - ref_speed*kp_f*error_F/100.0;

        }
*/
    }
    else if(c_flag == 3) //전진운동 오른쪽보기
    {
        //controller right look 오른쪽 거리를 맞추며 전진하는 제어
        if(error_R >= 0){
            left_end = ref_speed - ref_speed*(kp_r*error_R + kd_r*(error_R - pre_error_R))/100.0 - ref_speed*kp_f*error_F/100.0; 
            right_end = ref_speed - ref_speed*kp_f*error_F/100.0;
            if(right_end < 0) 
                right_end = 0;
            if(left_end < speed_limit - ref_speed*kp_f*error_F/100.0)    //회전 속도 감속 제한 
                left_end = speed_limit - ref_speed*kp_f*error_F/100.0;

        }
        else{
            left_end = ref_speed - ref_speed*kp_f*error_F/100.0;
            right_end = ref_speed + ref_speed*(kp_r*error_R + kd_r*(error_R - pre_error_R))/100.0 - ref_speed*kp_f*error_F/100.0;
            if(left_end < 0)
                left_end = 0;
            if(right_end < speed_limit - ref_speed*kp_f*error_F/100.0)   //회전 속도 감속 제한
                right_end = speed_limit - ref_speed*kp_f*error_F/100.0;

        }
    }

    printf("M_L : %d, M_R : %d\n",left_end,right_end);


    Motor_front(left_end,right_end);

    pre_error_L = error_L;
    pre_error_R = error_R;

    

}


int control_flag(float s_l, float s_f, float s_r) //왼쪽, 정면, 오른쪽(좌수법 알고리즘)
{
    // 좌수법 알고리즘
    
    // 1 = 벽, 0 = 빈공간  12cm 이상이 측정되면 벽이아닌 비어있는 것으로 가정한다.
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


    if(((ns_l != s_l) || (ns_f != s_f) || (ns_r != s_r)) || (s_f == 1)) //이전상태와 현재 상태가 다른가 혹은 전방에 벽이 갑지 되었나
    {
        //플래그 스위칭 0: 전진주행, 1: 회전주행 
        if(flag == 0)
            flag =1;
        else
            flag = 0;
    }

    if(flag == 1) //이전상태와 현재 상태가 다른가 혹은 전방에 벽이 갑지 되었나
    {
        //현재상태와 다른상태가 나올때 까지 회전하기 위해 현재상태 저장
        ns_l = s_l;
        ns_f = s_f;
        ns_r = s_r; 
        
        //현재상태에 따라 좌회전 우회전 결정 기본 왼쪽 뚫려있으면 좌회전 왼쪽 박혀있고 오른쪽 뚫려있으면 우회전
        //현재상태와 다른 상태가 나올때 까지 회전한다.

        if((s_l == 1) && (s_f == 1) && (s_r == 1)) //전부 벽인가
        {
            Motor_right_turn();
        }
        else if(s_l == 1)
        {
            return 2;
        }
        else if(s_l == 0) //왼쪽이 뚫렸나 왼쪽회전
        {  
            Motor_left_turn();
        }
        else if(s_r == 0) //오른쪽이 뚫렸나 오른쪽 회전
        {
            Motor_right_turn();
        }
        return 1; //회전운동
    }
    else if(flag == 0)
    {
        //다음 제어에 현재상태를 비교하기 위해 현재 상태를 저장한다.
        ns_l = s_l;
        ns_f = s_f;
        ns_r = s_r;

        if(s_l == 1)
            return 2; //전진운동 왼쪽 보기
        else if(s_r == 1)
            return 3; //전진운동 오른쪽 보기
    }
    else
        return 0;
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
//    set_PWM_frequency(pi, PWM_PIN0, 2000); //소리가 심함
//    set_PWM_frequency(pi, PWM_PIN1, 2000);
    set_PWM_range(pi, PWM_PIN0, 1000);
    set_PWM_range(pi, PWM_PIN1, 1000);
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

    //printf("echo0");
}
void cb_func_echo1(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_[1] = tick;
    else if(level == PI_LOW)
        dist_tick_[1] = tick - start_tick_[1];
    //printf("echo1");
}
void cb_func_echo2(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if(level == PI_HIGH)
        start_tick_[2] = tick;
    else if(level == PI_LOW)
        dist_tick_[2] = tick - start_tick_[2];
    //printf("echo2");
}


void Motor_front(int left, int right)
{
//    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
//    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
//    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, left);
    set_PWM_dutycycle(pi, PWM_PIN1, right);

    gpio_write(pi, INPUT1, PI_HIGH);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_HIGH);
 
}
void Motor_left_turn()
{
//    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
//    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
//    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, spin_speed);
    set_PWM_dutycycle(pi, PWM_PIN1, spin_speed);

    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_HIGH);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_HIGH);
               
}
void Motor_right_turn()
{
//    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
//    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
//    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, spin_speed);
    set_PWM_dutycycle(pi, PWM_PIN1, spin_speed);

    gpio_write(pi, INPUT1, PI_HIGH);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_HIGH);     gpio_write(pi, INPUT4, PI_LOW);
               
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

void* sensor(void* arg)
{
    int i;


    while(1){
    for(i =0; i < 3; i++)
    {
        start_tick_[i] = dist_tick_[i]= 0;
       // dist_tick_[i] = 0;
    }
    gpio_trigger(pi, TRIG_PINN0, 10, PI_HIGH); 
    gpio_trigger(pi, TRIG_PINN1, 10, PI_HIGH);
    gpio_trigger(pi, TRIG_PINN2, 10, PI_HIGH);

    time_sleep(0.03);

    if(dist_tick_[0] && start_tick_[0]){
        sensor_L = dist_tick_[0] / 1000000. * 340 / 2 * 100;
        if(sensor_L < 2 || sensor_L > 400)
            sensor_L = - 2;
    }
    else
        sensor_L = -1;
    
    if(dist_tick_[1] && start_tick_[1]){
        sensor_F = dist_tick_[1] / 1000000. * 340 / 2 * 100;
        if(sensor_F < 2 || sensor_F > 400)
            sensor_F = - 2;
    }
    else
        sensor_F = -1;
    
    if(dist_tick_[2] && start_tick_[2]){
        sensor_R = dist_tick_[2] / 1000000. * 340 / 2 * 100;
        if(sensor_R < 2 || sensor_R > 400)
            sensor_R = - 2;
    }
    else
        sensor_R = -1;
    // -1 sensor error, -2 range_error
//    printf("sensor");
    }
}






