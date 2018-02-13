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
#define spin_speed 400
//-----------------------------------------------------
//                      global var
//-----------------------------------------------------
int pi;

int ref_speed   =  0;   //설정속도

int left_end    =  0;   //왼쪽 모터 pwm값
int right_end   =  0;   //오른쪽 모터 pwm값


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

int ss_l, ss_f, ss_r;          //벽 감지 상태 현재 변수
int ns_l, ns_f, ns_r;       //벽 감지 상태 이전 변수
int flag = 0; 
int c_flag      =  2;   //좌수법 알고리즘에 의한 현재상황 플래그, 0:left turn, 1:right turn 2:forword(L), 3:forword(R), others: stop

int i = 0; //단순변수

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
void Motor_stop();

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

    // 전진확인 제어변수 0:좌회전, 1:우회전, 2:왼전진, 3:오른전진, 나머지 정지
    //c_flag 초기값 2
    printf("sensor_L : %.2f, sensor_F : %.2f, sensor_R : %.2f\n",sensor_L, sensor_F, sensor_R);


    //원하는 거리값을 맞추기위해 (설정거리 - 현재거리) 에러값 생성 
    error_L = ref_sensor - s_save_L;
    error_R = ref_sensor - s_save_R - 0.2;
    error_F = ref_sensor_F - s_save_F;
    if(error_F < 0) error_F = 0; // 정면은 설정거리부터 감속하기위해 음수 삭제
       

    switch(c_flag){
    case 0: //left turn
        pre_error_L = 0;
        pre_error_R = 0;
        Motor_left_turn();
        break;

    case 1: //right turn
        pre_error_L = 0;
        pre_error_R = 0;
        Motor_right_turn();
        break;

    case 2: //forword left
        pre_error_R = 0;
        //확꺽어지지 안도록 제어값 제한 둬보기

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
        Motor_stop(left_end,right_end);
        break;

    case 3: //forword right
        pre_error_L = 0;
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
        Motor_front(left_end,right_end);
        break;

    default: //stop
        pre_error_L = 0;
        pre_error_R = 0;
        Motor_stop();
        break;
    }

    // 현재상태 저장
    ns_l = ss_l;
    ns_f = ss_f;
    ns_r = ss_r;   

    printf("M_L : %d, M_R : %d, c_flag: %d \n",left_end,right_end,c_flag);

    pre_error_L = error_L;
    pre_error_R = error_R;

}


int control_flag(float s_l, float s_f, float s_r) //왼쪽, 정면, 오른쪽(좌수법 알고리즘)
{
    // 좌수법 알고리즘
    
    // 1 = 벽, 0 = 빈공간  12cm 이상이 측정되면 벽이아닌 비어있는 것으로 가정한다.
    if(s_l > 12)
        ss_l = 0;
    else
        ss_l = 1;
    
    if(s_f > 8)
        ss_f = 0;
    else
        ss_f = 1;
    
    if(s_r > 12)
        ss_r = 0;
    else
        ss_r = 1;

    //초기값 세팅
    if(i == 0){
        ns_l = ss_l;
        ns_f = ss_f;
        ns_r = ss_r;
        i++;
    }


    // 알고리즘 시작

    //회전하기 전
    if(ss_f == 1) //정면 벽 감지
    {
        if(ss_l == 0) //왼쪽 뚫림 좌회전
            return 0;
        else if(ss_r == 0) // 오른쪽 뚫림 우회전
            return 1;
        else if((ss_l == 1) && (ss_r == 1)) // 막다른 골목 우회전
            return 1;
        else
            return 4;
    }
    else if(ss_f == 0) //정면 벽 없음
    {
        if(ns_l == 0)//전 스탭에서 왼쪽 이 뚫려있었을경우
        {
            if(ss_r == 1)
                return 3;
        }
        if(ss_l == 0) // 왼쪽 뚫림 좌회전
            return 0;
        else if(ss_l == 1)// 왼쪽 벽 왼전진
            return 2;
        else if(ss_r == 1)// 오른쪽 벽 오른 전진
            return 3;
        else
            return 4;
    }
    //회전한 후

    return 4;
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
void Motor_stop()
{
    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, 0);
    set_PWM_dutycycle(pi, PWM_PIN1, 0);

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
    }
}
