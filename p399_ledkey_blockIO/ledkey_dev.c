#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/wait.h> //*
#include <linux/sched.h>

#define   LED_DEV_NAME            "ledkeydev"
#define   LED_DEV_MAJOR            240      
#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Read);


static unsigned long ledvalue = 15;
static char * twostring = NULL;
static int sw_irq[8] = {0};
static char sw_no = 0;

module_param(ledvalue, ulong ,0);
module_param(twostring,charp,0);

int led[4] = {
	IMX_GPIO_NR(1, 16),   //16
	IMX_GPIO_NR(1, 17),	  //17
	IMX_GPIO_NR(1, 18),   //18
	IMX_GPIO_NR(1, 19),   //19
};
int key[8] = {
	IMX_GPIO_NR(1, 20),   //20
	IMX_GPIO_NR(1, 21),	  //21
	IMX_GPIO_NR(4, 8),    //104
	IMX_GPIO_NR(4, 9),    //105
	IMX_GPIO_NR(4, 5),    //101
	IMX_GPIO_NR(7, 13),	  //205
	IMX_GPIO_NR(1, 7),    //7
	IMX_GPIO_NR(1, 8),    //8
};
static int ledkey_request(void)
{
	int ret = 0;
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
			break;
		} 
  		gpio_direction_output(led[i], 0);
	}
	for (i = 0; i < ARRAY_SIZE(key); i++) {
//		ret = gpio_request(key[i], "gpio key"); //success or fail return
		sw_irq[i] = gpio_to_irq(key[i]); //irq numer return
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", key[i], ret);
			break;
		} 
//  		gpio_direction_input(key[i]); //외부 인터럽트라 알아서 input으로 설정
	}
	return ret;
}
static void ledkey_free(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
	for (i = 0; i < ARRAY_SIZE(key); i++){
//		gpio_free(key[i]);
		free_irq(sw_irq[i],NULL);
	}
}

void led_write(unsigned char data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
    	gpio_set_value(led[i], (data >> i ) & 0x01);
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
}
void key_read(unsigned char * key_data)
{
	int i;
	unsigned long data=0;
	unsigned long temp;
	for(i=0;i<ARRAY_SIZE(key);i++)
	{
		temp = gpio_get_value(key[i]);
		if(temp)
		{
			data = i + 1;
			break;
		}
	}
#if DEBUG
	printk("#### %s, data = %ld\n", __FUNCTION__, data);
#endif
	*key_data = data;
	return;
}


int ledkeydev_open (struct inode *inode, struct file *filp)
{
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "ledkeydev open -> major : %d\n", num0 );
    printk( "ledkeydev open -> minor : %d\n", num1 );

    return 0;
}

ssize_t ledkeydev_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	int ret;
#if DEBUG
    printk( "ledkeydev read -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
#endif
	/*sleep on을 할지 말지 생각한다.. 0을 리턴할지 0이 아닌값이 올 때까지 잠들지*/
	if(!(filp->f_flags & O_NONBLOCK))//Blocking mode로 열었다.. nonblock으로 연 것의 반대
	{
		if(sw_no==0)
		{
			interruptible_sleep_on(&WaitQueue_Read);//process sleep
			//스위치 눌리면 isr 호출됨.. 커널의 함수니까 <-- 여기에 wake up 넣어야
		}
	//	wait_event_interruptible(WaitQueue_Read,sw_no);
		
		//100 : 1/100*100 = 1sec
	//	wait_event_interruptible_timeout(WaitQueue_Read,sw_no,100);
		
	}
	ret=copy_to_user(buf,&sw_no,count);
	sw_no = 0;
	if(ret < 0)
		return -ENOMEM;
    return count;
}

ssize_t ledkeydev_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
#if DEBUG
    printk( "ledkeydev write -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
#endif
	ret=copy_from_user(&kbuf,buf,count);
	if(ret < 0)
		return -ENOMEM;
	led_write(kbuf);
    return count;
}

static long ledkeydev_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{

#if DEBUG
    printk( "ledkeydev ioctl -> cmd : %08X, arg : %08X \n", cmd, (unsigned int)arg );
#endif
    return 0x53;
}

int ledkeydev_release (struct inode *inode, struct file *filp)
{
    printk( "ledkeydev release \n" );
    return 0;
}

struct file_operations ledkeydev_fops =
{
    .owner    = THIS_MODULE,
    .open     = ledkeydev_open,     
    .read     = ledkeydev_read,     
    .write    = ledkeydev_write,    
	.unlocked_ioctl = ledkeydev_ioctl,
    .release  = ledkeydev_release,  
};

irqreturn_t sw_isr(int irq, void *unuse) //커널에 호출해줌..그래서 매개변수 갖고있음
{
	int i; 
	for(i = 0; i<ARRAY_SIZE(key);i++)
	{
		if(irq==sw_irq[i])
		{
			sw_no = i+1;
			break;
		}
	}
	printk("IRQ : %d , switch_number : %d\n", irq,sw_no);
	wake_up_interruptible(&WaitQueue_Read);//재웠던 대기 큐 탐색
	return IRQ_HANDLED;
}
int ledkeydev_init(void) // device driver
{
	int result=0;
	int i;
	char* sw_name[8] = {"key1", "key2", "key3", "key4", "key5", "key6", "key7","key8"};
    printk( "ledkeydev ledkeydev_init \n" );    

    result = register_chrdev( LED_DEV_MAJOR, LED_DEV_NAME, &ledkeydev_fops);
    if (result < 0) return result;

	result = ledkey_request();
	if(result < 0)
	{
  		return result;     /* Device or resource busy */
	}
	
	for(i = 0; i < ARRAY_SIZE(key); i++)
	{ //여기서 등록하면 매개변수가 없기 때문에 파일포인터 사용 x, 따라서 open에서 해줌
		result = request_irq(sw_irq[i],sw_isr,IRQF_TRIGGER_RISING,sw_name[i],NULL);
		if(result)
		{
			printk("#### FAILED Request irq %d. error : %d \n", sw_irq[i], result);
			break;
		}
	}
    return result;
}

void ledkeydev_exit(void)
{
    printk( "ledkeydev ledkeydev_exit \n" );    
    unregister_chrdev( LED_DEV_MAJOR, LED_DEV_NAME );
	ledkey_free();
}

module_init(ledkeydev_init);
module_exit(ledkeydev_exit);

MODULE_AUTHOR("KSH");
MODULE_DESCRIPTION("test module");
MODULE_LICENSE("Dual BSD/GPL");
