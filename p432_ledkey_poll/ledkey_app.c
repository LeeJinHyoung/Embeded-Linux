#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>

#define DEVICE_FILENAME "/dev/ledkey_dev"

int main(int argc, char *argv[])
{
	int dev;
	char buff;
	int ret;
	int num = 1;
	struct pollfd Events[2];//두 개의 장치의 입력을 감시하겠다.
	char keyStr[80];

    if(argc != 2)
    {
        printf("Usage : %s [led_data(0x0~0xf)]\n",argv[0]);
        return 1;
    }
    buff = (char)strtoul(argv[1],NULL,16);//string 을 unsigned long으로
//    if(!((0 <= buff) && (buff <= 15)))
    if((buff < 0) || (15 < buff)) //0~15만 쓰기 때문에 유효성 검사
    {
        printf("Usage : %s [led_data(0x0~0xf)]\n",argv[0]);
        return 2;
    }

//  dev = open(DEVICE_FILENAME, O_RDWR | O_NONBLOCK);
  	dev = open(DEVICE_FILENAME, O_RDWR );
	if(dev < 0)
	{
		perror("open");
		return 2;
	}
	write(dev,&buff,sizeof(buff));

	fflush(stdin);//입력장치의 buffer를 초기화 해준다.
	memset( Events, 0, sizeof(Events));
  	Events[0].fd = fileno(stdin);//파일포인터를 디스크럽터 넘버로 바꿔줌
  	Events[0].events = POLLIN; //어떤 이벤트? 입력 이벤트 감시
	Events[1].fd = dev;
	Events[1].events = POLLIN;
	while(1)
	{
		ret = poll(Events, 2, 2000); //이벤트, 장치 개수, timeout
		if(ret<0)
		{
			perror("poll");
			exit(1);
		}
		else if(ret==0) //timeout으로 일어났을 때 
		{
  			printf("poll time out : %d Sec\n",2*num++);
			continue;
		}
		//어떤 장치에서 입력이 발생했는지 
		if(Events[0].revents & POLLIN)  //stdin / 키보드에서 이벤트가 발생
		{
			fgets(keyStr,sizeof(keyStr),stdin);//gets는 크기정보가 없어서 오류
			if(keyStr[0] == 'q')//종료생
				break;
			//keystr길이는 2 .. -1을하면 1.. 1번째의 \n을 \0으로 치환
			keyStr[strlen(keyStr)-1] = '\0';
			printf("STDIN : %s\n",keyStr);
			buff = (char)atoi(keyStr);
			write(dev,&buff,sizeof(buff));
		}
		else if(Events[1].revents & POLLIN) //ledkey / 스위치에서 이벤트 발
		{
			//swno에 값이 있으니 read 실행
			ret = read(dev,&buff,sizeof(buff));
			printf("key_no : %d\n",buff);
			write(dev,&buff,sizeof(buff));
			if(buff == 8)
				break;
		}
	}
	close(dev);
	return 0;
}
