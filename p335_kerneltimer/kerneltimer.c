#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
#define TIME_STEP	timeval  //KERNEL HZ=100
#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))
static int timeval = 100;	//f=100HZ, T=1/100 = 10ms, 100*10ms = 1Sec
module_param(timeval,int ,0);
static int ledval = 0;
module_param(ledval,int ,0);
typedef struct
{
	struct timer_list timer;	//24
	unsigned long 	  led;
} __attribute__ ((packed)) KERNEL_TIMER_MANAGER;

static KERNEL_TIMER_MANAGER* ptrmng = NULL;
void kerneltimer_timeover(unsigned long arg);

int key[] = {
    IMX_GPIO_NR(1, 20),
    IMX_GPIO_NR(1, 21),
    IMX_GPIO_NR(4, 8),
    IMX_GPIO_NR(4, 9),
    IMX_GPIO_NR(4, 5),
    IMX_GPIO_NR(7, 13),
    IMX_GPIO_NR(1, 7),
    IMX_GPIO_NR(1, 8),
};

int led[] = {
    IMX_GPIO_NR(1, 16),
    IMX_GPIO_NR(1, 17),
    IMX_GPIO_NR(1, 18),
    IMX_GPIO_NR(1, 19),
};

static int led_init(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
		} 
		else {
			gpio_direction_output(led[i], 0);
		}
	}
	return ret;
}
static void led_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
}

void led_write(char data)
{
    int i;
    for(i = 0; i < ARRAY_SIZE(led); i++){
        gpio_set_value(led[i], ((data >> i) & 0x01));
#if DEBUG
        printk("#### %s, data = %d(%d)\n", __FUNCTION__, data,(data>>i));
#endif
    }
}
void kerneltimer_registertimer(KERNEL_TIMER_MANAGER *pdata, unsigned long timeover)
{
	init_timer( &(pdata->timer) );	//커널 타이머리스트 구조체  초기화
	pdata->timer.expires = get_jiffies_64() + timeover;  //10ms *100 = 1sec 만료 시간..1초 뒤에 타임 오버 함수 호출
	pdata->timer.data	 = (unsigned long)pdata ; //handler의 매개변수를 맞추기 위해
	pdata->timer.function = kerneltimer_timeover;//
	add_timer( &(pdata->timer) );
}
void kerneltimer_timeover(unsigned long arg) // pData가 arg로 왔음.. interrupt handler
{
	KERNEL_TIMER_MANAGER* pdata = NULL;
	if( arg )
	{
		pdata = ( KERNEL_TIMER_MANAGER *)arg; //arg는 주소기 때문에 형변환
		led_write(pdata->led & 0x0f);
#if DEBUG
		printk("led : %#04x\n",(unsigned int)(pdata->led & 0x0000000f));
#endif
		pdata->led = ~(pdata->led);//0x05를 보수.. 
		kerneltimer_registertimer( pdata, TIME_STEP); //다시 등록
	}
}

int kerneltimer_init(void)
{
	led_init();

	printk("timeval : %d , sec : %d , size : %d\n",timeval,timeval/HZ, sizeof(KERNEL_TIMER_MANAGER ));

	ptrmng = (KERNEL_TIMER_MANAGER *)kmalloc( sizeof(KERNEL_TIMER_MANAGER ), GFP_KERNEL);
	if(ptrmng == NULL) return -ENOMEM;
	memset( ptrmng, 0, sizeof( KERNEL_TIMER_MANAGER));
	ptrmng->led = ledval;
	kerneltimer_registertimer( ptrmng, TIME_STEP);
	return 0;
}
void kerneltimer_exit(void)
{
	if(timer_pending(&(ptrmng->timer)))
		del_timer(&(ptrmng->timer));
	if(ptrmng != NULL)
	{
		kfree(ptrmng);
	}
	led_write(0);
	led_exit();
}
module_init(kerneltimer_init);
module_exit(kerneltimer_exit);
MODULE_LICENSE("Dual BSD/GPL");
