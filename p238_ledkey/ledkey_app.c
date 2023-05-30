#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILENAME  "/dev/ledkey_dev"

void print_led(unsigned char);
void print_key(unsigned char);
int main(int argc,char * argv[])
{
    int dev;
    char buff;
	char ledbuff[4] = {0};
	char keybuff[8] = {0};
    char oldkeybuff = 0;
	int ret;
	int i;
	if(argc < 2)
	{
		printf("USAGE : %s [ledval] \n",argv[0]);
		return 1;
	}
	buff = atoi(argv[1]);
	for(i = 0; i<4; i++)
	{
		ledbuff[i] = (buff>>i)&0x01;
		printf("ledbiff[%d] : %d\n", i, ledbuff[i]);
	}
    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
	if(dev<0)
	{
		perror("open()");
		return 2;
	}
    ret = write(dev,&buff,sizeof(buff));
	if(ret < 0)
	{	
		perror("write()");
		return 3;
	}
	print_led(buff);
	
	buff = 0;

	do{
		read(dev, &buff, sizeof(buff));
		if((buff != 0)&&(oldkeybuff != buff))
		{
			printf("key : %d\n", buff);
			print_led(buff);
			write(dev,&buff,sizeof(buff));
			print_key(buff);
			oldkeybuff = buff;
			if(buff == 128) //key == 8
			{
				printf("STOP DEVICE application \n");
				break;
			}
		}
	}while(1);
	
	close(dev);
    return 0;
}
void print_led(unsigned char led)
{
	int i;
	puts("1:2:3:4");
	for(i=0;i<=3;i++)
	{
		if(led & (0x01 << i))
			putchar('O');
		else
			putchar('X');
		if(i < 3 )
			putchar(':');
		else
			putchar('\n');
	}
	return;
}

void print_key(unsigned char key)
{
	int i;
	puts("1:2:3:4:5:6:7:8");
	for(i=0;i<=7;i++)
	{
		if(key & (0x01 << i))
			putchar('O');
		else
			putchar('X');
		if(i < 7 )
			putchar(':');
		else
			putchar('\n');
	}
	return;
}
