#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define DEVICE_FILENAME	"/dev/leddev"	//mknod로 생성한 파일 이름
void print_led(char*);
void print_key(char*);
int main(int argc, char* argv[])
{
	int i;
	int dev;
	char buff = 0;
//	int ret;
	char ledbuff[4] = {0};
	char keybuff[8] = {0};
	char oldkeybuff[8] = {0};
	int ret;

	if(argc < 2)
	{
		printf("USAGE : %s [ledval] \n", argv[0]);
		return 1;
	}
	buff = atoi(argv[1]);
	for(i = 0; i < 4; i++)
	{
		ledbuff[i] = (buff>>i)&0x01;
		printf("ledbuff[%d] : %d\n", i, ledbuff[i]);
	}
	dev = open(DEVICE_FILENAME, O_RDWR | O_NDELAY);
	//커널의시스템 콜인  open함수를 호출하고 /dev/calldev의 데이터를 꺼내옴
	// char.. major..minor..가져오고 device driver 에 있는file operation open을 호출함
	//240번 인덱스에 있는 device driver의 open.
	
	if(dev<0)
	{	
		perror("open()");
		return 2;
	}
	ret = write(dev,ledbuff,sizeof(ledbuff));
	if(ret<0)
	{
		perror("write()");
		return 3;
	}
	print_led(ledbuff);
	buff = 0;

	do{
		read(dev,keybuff,sizeof(keybuff));
		for(i = 0 ; i <8; i ++)
		{
			if((keybuff[i] !=0)&&(keybuff[i] != oldkeybuff[i]))
			{
				print_key(keybuff);
				memcpy(ledbuff, keybuff, sizeof(ledbuff));
				write(dev, ledbuff, sizeof(ledbuff));
				print_led(ledbuff);
				memcpy(oldkeybuff, keybuff, sizeof(keybuff));
			}
		}
		if(keybuff[7])
			break;
	}while(1);	
	close(dev);
	return 0;
}
void print_led(char* ledbuff)
{
	int i;
	puts("1:2:3:4");
	for(i = 0; i<=3;i++)
	{
		if(ledbuff[i] == 1)
			putchar('o');
		else
			putchar('x');
		if(i<3)
			putchar(':');
		else
			putchar('\n');
	}
	return;
}
void print_key(char* keybuff)
{
	int i;
	puts("1:2:3:4:5:6:7:8");
	for(i = 0; i<=7;i++)
	{
		if(keybuff[i] == 1)
			putchar('o');
		else
			putchar('x');
		if(i<7)
			putchar(':');
		else
			putchar('\n');
	}
	return;
}
