#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>

#define DEBUG	1
#define IMX_GPIO_NR(bank, nr)	(((bank)-1)*32+(nr))
#define LED_DEV_NAME	"leddev"
#define LED_DEV_MAJOR	240		

int led[] = {
	IMX_GPIO_NR(1, 16),
	IMX_GPIO_NR(1, 17),
	IMX_GPIO_NR(1, 18),
	IMX_GPIO_NR(1, 19),
};
static int led_request(void)
{
	int ret = 0;
	int i;

	for(i = 0; i < ARRAY_SIZE(led); i++)
	{
		ret = gpio_request(led[i], "gpio led");
		if(ret < 0)
		{
			printk("FAILED REQUEST GPIO %d , error : %d \n", led[i], ret);
			break;
		}
	}
	return ret;
}
static void led_free(void)
{
	int i;
	for(i = 0; i<ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
}
void led_write(unsigned long data)
{
	int i;
	for(i = 0 ; i< ARRAY_SIZE(led); i++){
		gpio_direction_output(led[i], (data>>i)&0x01);
	}
}
void led_read(char* led_data)
{
	int i;
	unsigned long data=0;
	unsigned long temp;
	for(i = 0; i<4; i++)
	{
		gpio_direction_input(led[i]);//16..17..18..19..를 입력으로 하겠다
		temp = gpio_get_value(led[i])<<i;//한 비트의 값.. 16번 비트의 값을 읽어오겠다
	//	data |= temp;
		data = data | temp;
	}
	*led_data=data;
	led_write(data); //because not implement switch yet
	return;
}

static int leddev_open(struct inode *inode, struct file *filp)
{
	int num = MINOR(inode->i_rdev);
	printk("leddev open -> minor : %d\n", num);
	num = MAJOR(inode->i_rdev);
	printk("leddev open -> major : %d\n", num);
	return 0;
}
static loff_t leddev_llseek(struct file *filp, loff_t off, int whence)
{
	printk("leddev llseek -> off : %08X, whence : %08X\n",(unsigned int)off, whence);
	return 0x23;
}
static ssize_t leddev_read(struct file *filp, char* buf, size_t count, loff_t* f_pos)
{
	printk("leddev read -> buf : %08X, count : %08X\n",(unsigned int)buf, count);
	led_read(buf);
	return count;
}
static ssize_t leddev_write(struct file *filp, const char* buf, size_t count, loff_t* f_pos)
{
	printk("leddev write -> buf : %d\n, count : %08X\n",(unsigned int)buf, count);
	led_write((unsigned long)(*buf));
	return count;
}
static long leddev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) 
{
	printk("leddev ioctl -> cmd : %08X, arg : %08X\n",cmd, (unsigned int)arg);
	return 0x53;
}
static int leddev_release(struct inode *inode, struct file *filp)
{
	printk("leddev release\n");
	return 0;
}
struct file_operations leddev_fops = 
{
	.owner = THIS_MODULE,
	.open = leddev_open,
	.read = leddev_read,
	.write = leddev_write,
	.unlocked_ioctl = leddev_ioctl,
	.llseek = leddev_llseek,
	.release = leddev_release,
};
static int led_init(void)
{
	int result;
	int ret;
	printk("leddev leddev_init \n");
	result = register_chrdev(LED_DEV_MAJOR, LED_DEV_NAME, &leddev_fops);
	ret = led_request();
	if(result < 0) return result;
	return 0;
}
static void led_exit(void)
{
	printk("leddev leddev_exit \n");
	unregister_chrdev(LED_DEV_MAJOR, LED_DEV_NAME); //구조체쪽에 nullptr 넣어서 어플리케이션과 디바이스드라이버의 연결 해제
	led_free();
}
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("Dual BSD/GPL"); // 
