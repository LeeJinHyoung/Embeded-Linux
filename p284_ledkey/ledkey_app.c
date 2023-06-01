#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DEVICE_FILENAME	"/dev/leddev"	//mknod로 생성한 파일 이름
void print_led(unsigned char);
void print_key(unsigned char);
int main(int argc, char* argv[])
{
	int dev;
	char buff = 0;
	int ret;
	int buf;
	
	if(argc < 2)
	{
		printf("USAGE : %s [ledval] \n", argv[0]);
		return 1;
	}

	buf = atoi(argv[1]);
	dev = open(DEVICE_FILENAME, O_RDWR | O_NDELAY);
	//커널의시스템 콜인  open함수를 호출하고 /dev/calldev의 데이터를 꺼내옴
	// char.. major..minor..가져오고 device driver 에 있는file operation open을 호출함
	//240번 인덱스에 있는 device driver의 open.
	
	if(dev<0)
	{	
		perror("open()");
	}
	ret = write(dev,&buf,sizeof(buf));
	print_led(buff);
	if(ret<0)
		perror("write()");
	
	buf = 0;
	ret = read(dev,&buf,sizeof(buf));
	print_key(buf);
	close(dev);
	return 0;
}
void print_led(unsigned char led)
{
	int i;
	puts("1:2:3:4");
	for(i = 0; i<=3;i++)
	{
		if(led&(0x01<<i))
			putchar('o');
		else
			putchar('x');
		if(i<=3)
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
	for(i = 0; i<=7;i++)
	{
		if(key&(0x01<<i))
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
