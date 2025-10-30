#include <linux/module.h>
#include <linux/printk.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joan Ripoll");
MODULE_DESCRIPTION("Device that echoes writes back");

struct data_slice {
	size_t size;
	size_t offset;
	char buffer[];
};

static ssize_t kecho_read(struct file *filp, char __user *buffer, size_t count,
			  loff_t *offset);
static ssize_t kecho_write(struct file *filp, const char __user *buffer,
			   size_t count, loff_t *offset);
static int kecho_open(struct inode *inode, struct file *filp);
static int kecho_release(struct inode *inode, struct file *filp);

static dev_t dev;
static struct cdev cdev;
static struct class *class;
static struct device *device;

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = kecho_read,
	.write = kecho_write,
	.open = kecho_open,
	.release = kecho_release,
};

static int __init kecho_init(void)
{
	int ret;
	if ((ret = alloc_chrdev_region(&dev, 0, 1, "kecho")) < 0) {
		pr_alert("Failed to allocate device number\n");
		goto err_alloc;
	}
	cdev_init(&cdev, &fops);
	if ((ret = cdev_add(&cdev, dev, 1)) < 0) {
		pr_alert("Failed to add device\n");
		goto err_add;
	}
	if (IS_ERR(class = class_create("kecho"))) {
		ret = PTR_ERR(class);
		pr_alert("Failed to create class\n");
		goto err_class;
	}
	if (IS_ERR(device = device_create(class, NULL, dev, NULL, "echo"))) {
		ret = PTR_ERR(device);
		pr_alert("Failed to create device\n");
		goto err_device;
	}

	pr_info("Loeaded kecho\n");
	return 0;

err_device:
	class_destroy(class);
err_class:
	cdev_del(&cdev);
err_add:
	unregister_chrdev_region(dev, 1);
err_alloc:
	return ret;
}

static void __exit kecho_exit(void)
{
	device_destroy(class, dev);
	class_destroy(class);
	cdev_del(&cdev);
	unregister_chrdev_region(dev, 1);
	pr_info("Unloaded kecho\n");
}

static int kecho_open(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static int kecho_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static ssize_t kecho_read(struct file *filp, char __user *buffer, size_t count,
			  loff_t *offset)
{
	struct data_slice *data = filp->private_data;
	if (!data)
		return 0;

	size_t remaining = data->size - data->offset;
	if (remaining == 0) {
		return 0;
	} else if (count < remaining) {
		if (copy_to_user(buffer, data->buffer, count))
			return -EFAULT;
		data->offset += count;
		return count;
	} else {
		if (copy_to_user(buffer, data->buffer, remaining))
			return -EFAULT;
		data->offset += remaining;
		return remaining;
	}
}

static ssize_t kecho_write(struct file *filp, const char __user *buffer,
			   size_t count, loff_t *offset)
{
	struct data_slice *data;
	kfree(filp->private_data);
	data = kmalloc(struct_size(data, buffer, count), GFP_KERNEL);
	filp->private_data = data;
	if (!filp->private_data)
		return -ENOSPC;

	if (copy_from_user(data->buffer, buffer, count))
		return -EFAULT;
	data->size = count;
	data->offset = 0;

	return count;
}

module_init(kecho_init);
module_exit(kecho_exit);
