#include <stdio.h>
#include <pigpio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "74hc595_functions.h"

#define ROW 8
#define COL_size 300

void dot(int row, int col)
{
    uint8_t row8, col8;
    uint16_t tmp;
    row8 = ~(1 << (8-row));
    col8 = 1 << (8-col);
    tmp = (row8<<8) | col8;
    set16(tmp);
}

int cnt;
void callback(void)
{
//	printf("500 milliseconds have elapsed\n");
	cnt++;	
}
 
int main(void)
{
    FILE *fp;
    int i,j;
    int length;
    int total_col;
    char check[COL_size];
    
    fp = fopen("data","r+");
    
    fgets(check,COL_size,fp);
    length = strlen(check);

    uint8_t mat[ROW][length-1];

    for(i =0; i< ROW; i++)
    {
        fseek(fp,length*i,SEEK_SET);
        fgets(check,COL_size,fp);
        for(j =0;j<length-1;j++)
        {
            if(check[j] == ' ')
                mat[i][j] = 0;
            else if(check[j] == 'o')
                mat[i][j] = 1;
            else
                NULL;   
        }
    }

    fclose(fp);
    
    total_col = sizeof(mat) / 8;

    int ret;
    ret = init();
    if(ret == 0)
        return 0;

    gpioSetTimerFunc(0, 100, callback);

    while(1)
        for(i = 0 ; i < 8 ; i++)
            for(j = 0 ; j < 8 ; j++)
                if(mat[i][(j+cnt)%total_col] == 1)
                    dot(i+1,j+1);


    release();
    return 0;
}

