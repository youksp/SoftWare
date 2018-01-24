#include <stdio.h>
#include <stdlib.h>
#include <pigpiod_if2.h>

#define BAUD_RATE	1000000
#define LOOP		50
#define sampling_gpio 20
#define total_time 300

int pi;
int spi;
uint16_t value;
uint64_t sum = 0;
uint16_t avg;
int channel = 0;
char snd_buf[3];
char rcv_buf[LOOP][3];
uint32_t start_tick, diff_tick;
int count =0;
uint16_t data[total_time];

void r_time()
{

        if(count <= total_time){
        count++;
        avg = sum = 0;
        snd_buf[0] = 0x18 | channel;

        start_tick = get_current_tick(pi);

        for(int i = 0 ; i < LOOP ; i++)
            spi_xfer(pi, spi, snd_buf, rcv_buf[i], 3);

        diff_tick = get_current_tick(pi) - start_tick;

        for(int i = 0 ; i < LOOP ; i++){
            value = ((rcv_buf[i][1] & 0x3f) << 8) | (rcv_buf[i][2] & 0xff);
            value = value >> 2;
            sum += value;
        }
        avg = sum / LOOP;
        
        data[count-1] = avg;

//        printf("channel-%d : %4d %.1fv", channel, avg, 3.3*avg/4095);
//        printf("\t%lld sps", 1000000LL * LOOP / diff_tick);
//        printf("loop count : %d\n",icount);
//        printf("count : %d\n",count);
        }
}



int main(int argc, char* argv[])
{
    int i;

    if(argc == 2) channel = atoi(argv[1]);

    if((pi = pigpio_start(NULL, NULL)) < 0) {
        fprintf(stderr, "pigpio_start error\n"); 
        return 1;
    }

    set_PWM_frequency(pi, sampling_gpio, 10);
    set_PWM_range(pi, sampling_gpio, 2);
    set_PWM_dutycycle(pi, sampling_gpio, 1);

    if((spi = spi_open(pi, 0, BAUD_RATE, 0)) < 0) {
        fprintf(stderr, "spi_open error\n"); 
        return 2;
    }

    callback(pi, sampling_gpio, RISING_EDGE, r_time);

    while(count != total_time+1){
   }

    for(i=0; i<total_time;i++)
    {
        printf("%.1f %d\n",i/10.0,data[i]);
    }
	spi_close(pi, spi);
	pigpio_stop(pi);
	return 0;
}
