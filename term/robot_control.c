#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<pigpiod_if2.h>

#define INPUT1 5
#define INPUT2 6
#define INPUT3 17
#define INPUT4 27

#define PWM_PIN0 23
#define PWM_PIN0 24


int pi;

int ref_duty = 50;

void Motor_Init()
{
    set_mode(pi,INPUT1, PI_OUTPUT);     set_mode(pi,INPUT2, PI_OUTPUT);
    set_mode(pi,INPUT3, PI_OUTPUT);     set_mode(pi,INPUT4, PI_OUTPUT);

    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);
}

void Motor_Forword()
{
     gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

   gpio_write(pi, INPUT1, PI_HIGH);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_HIGH);     gpio_write(pi, INPUT4, PI_LOW);

}

void Motor_Backword()
{
     gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

   gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_HIGH);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_HIGH);

}

void Motor_Left()
{
     gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

   gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_HIGH);
    gpio_write(pi, INPUT3, PI_HIGH);     gpio_write(pi, INPUT4, PI_LOW);

}

void Motor_Right()
{
     gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

   gpio_write(pi, INPUT1, PI_HIGH);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_HIGH);

}

void Motor_stop()
{
    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);
}

void Motor_Driving(int left, int right)
{

    gpio_write(pi, INPUT1, PI_LOW);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_LOW);     gpio_write(pi, INPUT4, PI_LOW);
    usleep(10);

    set_PWM_dutycycle(pi, PWM_PIN0, left);
    set_PWM_dutycycle(pi, PWM_PIN1, right);
 
    gpio_write(pi, INPUT1, PI_HIGH);     gpio_write(pi, INPUT2, PI_LOW);
    gpio_write(pi, INPUT3, PI_HIGH);     gpio_write(pi, INPUT4, PI_LOW);

}

void PWM_Init()
{
    set_PWM_range(pi, PWM_PIN0, 100);
    set_PWM_range(pi, PWM_PIN1, 100);
    set_PWM_dutycycle(pi, PWM_PIN0, ref_duty);
    set_PWM_dutycycle(pi, PWM_PIN1, ref_duty);
    usleep(10);
}

int main(void)
{
    int i;


    static struct termios oldtio, newtio;

    tcgetattr(0, &oldtio);
    newtio = oldtio;
    newtio.c_lflag &= ~ICANON;
    newtio.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &newtio);

    if((pi = pigpio_start(NULL,NULL)) < 0){
        fprintf(stderr, "%s\n",pigpio_error(pi)); exit(-1);
    }
    Motor_Init();
    PWM_Init();

    /*******************************/
    int c;
    while((c = getc(stdin)) != EOF){
        switch(c){
            case 27:    
                c = getc(stdin);
                if(c == 91){
                    c = getc(stdin);
                    if(c == 68){ 
                        printf("<LEFT>\n");     Motor_Left();
                    }
                    else if(c == 67){
                        printf("<RIGHT>\n");    Motor_Right();
                    }
                    else if(c == 65){
                        printf("<UP>\n");       Motor_Forword();
                    }
                    else if(c == 66){
                        printf("<DOWN>\n");     Motor_Backword();
                    }
                }
                break;
            case 32:
                printf("<SPACE BAR>\n");
                Motor_stop();
                break;
            case 10:
                printf("<ENTER>\n");
                break;
        }
    }
    if(ferror(stdin)){
        printf("input error\n");
        return 1;
    }
    /*******************************/

    set_PWM_dutycycle(pi, PWM_PIN, 0);
    pigpio_stop(pi);
    tcsetattr(0, TCSANOW, &oldtio);
    return 0;
}



