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

#define KERTIM_DEV_NAME			"ktimerdev"
#define KERTIM_DEV_MAJOR		240

static int timeval = 100;	//f=100HZ, T=1/100 = 10ms, 100*10ms = 1Sec
//module_param(timeval,int ,0);
//static int ledval = 0;
//module_param(ledval,int ,0);
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

static int key_init(void)
{
	int ret = 0;
	int i;

	for(i = 0; i < ARRAY_SIZE(key); i++){
		ret = gpio_request(key[i], "gpio key");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n",     key[i], ret);
		}
		else
			gpio_direction_input(key[i]);
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

static void key_exit(void)
{
	int i;
	for (i = 0; i<ARRAY_SIZE(key); i++){
		gpio_free(key[i]);
	}
}


void key_read(char* key_data)
{
	int i;
	char data=0;
	for(i=0; i<ARRAY_SIZE(key);i++)
	{
		if(gpio_get_value(key[i]))
		{
			data = i+1;
			break;
		}
	}
	*key_data = data;
	return;
}

void led_write(char data)
{
    int i;
    for(i = 0; i < ARRAY_SIZE(led); i++){
		gpio_set_value(led[i], (data >> i ) & 0x01);	
	}
	
/*        gpio_set_value(led[i], ((data >> i) & 0x01));
#if DEBUG
        printk("#### %s, data = %d(%d)\n", __FUNCTION__, data,(data>>i));
#endif
    }*/
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
		pdata = ( KERNEL_TIMER_MANAGER *)arg;//arg는 주소기 때문에 형변환
		led_write(pdata->led & 0x0f);
#if DEBUG
		printk("led : %#04x\n",(unsigned int)(pdata->led & 0x0000000f));
#endif
		pdata->led = ~(pdata->led);//0x05를 보수.. 
		kerneltimer_registertimer( pdata, TIME_STEP); //다시 등록
	}
}

ssize_t ktimerdev_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
//	get_user(kbuf,&buf);
	ret = copy_from_user(&kbuf,buf,count);
	if(ret<0)
		return -ENOMEM;

	ptrmng->led = kbuf;
	return count;
}

ssize_t ktimerdev_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
	key_read(&kbuf);
	ret = copy_to_user(buf,&kbuf,count);
	return count;
}

int ktimerdev_open(struct inode *inode, struct file *filp)
{
	int num0 = MAJOR(inode->i_rdev);
	int num1 = MINOR(inode->i_rdev);
	printk( "ledkeydev open -> major : %d\n", num0 );
	printk( "ledkeydev open -> minor : %d\n", num1 );

	return 0;
}

struct file_operations kerneltimer_fops =
{
	.owner	= THIS_MODULE,
	.open	= ktimerdev_open,
	.read	= ktimerdev_read,
	.write	= ktimerdev_write,
};

int kerneltimer_init(void)
{
	int result;
	led_init();
	key_init();
	printk("timeval : %d , sec : %d , size : %d\n",timeval,timeval/HZ, sizeof(KERNEL_TIMER_MANAGER ));

	result = register_chrdev(KERTIM_DEV_MAJOR, KERTIM_DEV_NAME, &kerneltimer_fops);
	if(result<0) return result;

	ptrmng = (KERNEL_TIMER_MANAGER *)kmalloc( sizeof(KERNEL_TIMER_MANAGER ), GFP_KERNEL);
	if(ptrmng == NULL) return -ENOMEM;
	memset( ptrmng, 0, sizeof( KERNEL_TIMER_MANAGER));
//	ptrmng->led = ledval;
	kerneltimer_registertimer( ptrmng, TIME_STEP);
	return 0;
}
void kerneltimer_exit(void)
{
	if(timer_pending(&(ptrmng->timer)))
		del_timer(&(ptrmng->timer));
	unregister_chrdev(KERTIM_DEV_MAJOR, KERTIM_DEV_NAME);
	if(ptrmng != NULL)
	{
		kfree(ptrmng);
	}
	led_write(0);
	led_exit();
	key_exit();
}
module_init(kerneltimer_init);
module_exit(kerneltimer_exit);
MODULE_LICENSE("Dual BSD/GPL");
