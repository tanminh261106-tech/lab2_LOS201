  #include <linux/module.h>
  #include <linux/kernel.h>
  #include <linux/fs.h>
  #include <linux/uaccess.h>
  #include <linux/mutex.h>
  #include <linux/device.h>
  #include <linux/cdev.h>
  #include <linux/proc_fs.h>
  #include <linux/seq_file.h>
  #include <linux/string.h>

  #define DRIVER_NAME  "lab2"
  #define CLASS_NAME   "lab2_class"
  #define DRIVER_MAJOR 240
  #define BUFFER_SIZE  1024

  static char device_buffer[BUFFER_SIZE];
  static size_t buffer_len;
  static int open_count;
  static int reverse_read = 0;
static unsigned long overwrite_count = 0;
  static DEFINE_MUTEX(lab2_mutex);
  static ssize_t reverse_read_show(struct device *dev,
                                   struct device_attribute *attr, char *buf)
  {
      int v;
      mutex_lock(&lab2_mutex);
      v = reverse_read;
      mutex_unlock(&lab2_mutex);
      return sprintf(buf, "%d\n", v);
  }

  static ssize_t reverse_read_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
  {
      int val;
      if (kstrtoint(buf, 10, &val) || (val != 0 && val != 1))
          return -EINVAL;
      mutex_lock(&lab2_mutex);
      reverse_read = val;
      mutex_unlock(&lab2_mutex);
      return count;
  }
  static DEVICE_ATTR_RW(reverse_read);

  static struct class *lab2_class;
  static struct device *lab2_device;
  static struct proc_dir_entry *lab2_proc;

  /* Open device */
  static int lab2_open(struct inode *inode, struct file *file)
  {
      mutex_lock(&lab2_mutex);
      open_count++;
      mutex_unlock(&lab2_mutex);

      pr_info("lab2: device opened\n");
      return 0;
  }

  /* Read from device */
  static ssize_t lab2_read(struct file *file,
                           char __user *user_buf,
                           size_t count,
                           loff_t *offset)
  {
      size_t available;
        size_t i;
      unsigned long not_copied;

      if (mutex_lock_interruptible(&lab2_mutex))
          return -ERESTARTSYS;

      if (*offset >= buffer_len) {
          mutex_unlock(&lab2_mutex);
          return 0;
      }

      available = buffer_len - *offset;

      if (count > available)
          count = available;

  if (reverse_read) {
      for (i = 0; i < count; i++) {
   if (put_user(device_buffer[buffer_len - 1 - (*offset + i)], user_buf + i)) {
          mutex_unlock(&lab2_mutex);
          return -EFAULT;
      }
  }
  } else {
      not_copied = copy_to_user(user_buf, device_buffer + *offset, count);
      if (not_copied) {
          mutex_unlock(&lab2_mutex);
          return -EFAULT;
      }
  }

      *offset += count;

      mutex_unlock(&lab2_mutex);
      return count;
  }

  /* Write to device */
  static ssize_t lab2_write(struct file *file,
                            const char __user *user_buf,
                            size_t count,
                            loff_t *offset)
  {
      size_t len;
      unsigned long not_copied;
      bool had_data;

      if (mutex_lock_interruptible(&lab2_mutex))
          return -ERESTARTSYS;

  had_data = (buffer_len > 0);

      len = min(count, (size_t)(BUFFER_SIZE - 1));

      not_copied = copy_from_user(
          device_buffer,
          user_buf,
          len
      );

      if (not_copied) {
          mutex_unlock(&lab2_mutex);
          return -EFAULT;
      }

      device_buffer[len] = '\0';
      buffer_len = len;

if (had_data)
        overwrite_count++;

      mutex_unlock(&lab2_mutex);

      pr_info("lab2: wrote %zu bytes\n", len);
      return len;
  }

  /* Close device */
  static int lab2_release(struct inode *inode, struct file *file)
  {
      mutex_lock(&lab2_mutex);

      if (open_count > 0)
          open_count--;

      mutex_unlock(&lab2_mutex);

      pr_info("lab2: device closed\n");
      return 0;
  }

  /* Seek in device */
  static loff_t lab2_llseek(struct file *file, loff_t off, int whence)
  {
      loff_t newpos;

      if (mutex_lock_interruptible(&lab2_mutex))
          return -ERESTARTSYS;

      switch (whence) {
      case SEEK_SET:
          newpos = off;
          break;
      case SEEK_CUR:
          newpos = file->f_pos + off;
          break;
      case SEEK_END:
          newpos = (loff_t)buffer_len + off;
          break;
      default:
          mutex_unlock(&lab2_mutex);
          return -EINVAL;
      }

      if (newpos < 0 || newpos > (loff_t)buffer_len) {
          mutex_unlock(&lab2_mutex);
          return -EINVAL;          /* f_pos giữ nguyên */
      }

      file->f_pos = newpos;
      mutex_unlock(&lab2_mutex);

      return newpos;
  }

  static const struct file_operations lab2_fops = {
      .owner   = THIS_MODULE,
      .open    = lab2_open,
      .read    = lab2_read,
      .write   = lab2_write,
      .release = lab2_release,
      .llseek  = lab2_llseek,      /* trước đây là default_llseek */
  };

  /* /proc/lab2_info */
  static int lab2_proc_show(struct seq_file *m, void *v)
  {
      mutex_lock(&lab2_mutex);

      seq_printf(m, "Driver: %s\n", DRIVER_NAME);
      seq_printf(m, "Major: %d\n", DRIVER_MAJOR);
      seq_printf(m, "Buffer size: %d\n", BUFFER_SIZE);
      seq_printf(m, "Buffer length: %zu\n", buffer_len);
 seq_printf(m, "Overwrites : %lu\n", overwrite_count); 
      seq_printf(m, "Open count: %d\n", open_count);
      seq_printf(m, "Last data: %s\n", device_buffer);

      mutex_unlock(&lab2_mutex);
      return 0;
  }

  static int lab2_proc_open(struct inode *inode, struct file *file)
  {
      return single_open(file, lab2_proc_show, NULL);
  }

  static const struct proc_ops lab2_proc_ops = {
      .proc_open    = lab2_proc_open,
      .proc_read    = seq_read,
      .proc_lseek   = seq_lseek,
      .proc_release = single_release,
  };

  /* sysfs: buffer_len */
  static ssize_t buffer_len_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
  {
      ssize_t ret;

      mutex_lock(&lab2_mutex);
      ret = sysfs_emit(buf, "%zu\n", buffer_len);
      mutex_unlock(&lab2_mutex);

      return ret;
  }
  static DEVICE_ATTR_RO(buffer_len);

  /* sysfs: open_count */
  static ssize_t open_count_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
  {
      ssize_t ret;

      mutex_lock(&lab2_mutex);
      ret = sysfs_emit(buf, "%d\n", open_count);
      mutex_unlock(&lab2_mutex);

      return ret;
  }
  static DEVICE_ATTR_RO(open_count);

  /* sysfs: last_data */
 
/* sysfs: overwrite_count */
static ssize_t overwrite_count_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);
    ret = sysfs_emit(buf, "%lu\n", overwrite_count);
    mutex_unlock(&lab2_mutex);

    return ret;
}
static DEVICE_ATTR_RO(overwrite_count);

 static ssize_t last_data_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
  {
      ssize_t ret;

      mutex_lock(&lab2_mutex);
      ret = sysfs_emit(buf, "%s\n", device_buffer);
      mutex_unlock(&lab2_mutex);

      return ret;
  }
  static DEVICE_ATTR_RO(last_data);

  static struct attribute *lab2_attrs[] = {
      &dev_attr_buffer_len.attr,
      &dev_attr_open_count.attr,
      &dev_attr_last_data.attr,
&dev_attr_overwrite_count.attr,      
NULL,
  };

  static const struct attribute_group lab2_attr_group = {
      .attrs = lab2_attrs,
  };

  /* Initialize driver */
  static int __init lab2_init(void)
  {
      int ret;

      ret = register_chrdev(
          DRIVER_MAJOR,
          DRIVER_NAME,
          &lab2_fops
      );

      if (ret < 0) {
          pr_err("lab2: failed to register character device\n");
          return ret;
      }

      lab2_class = class_create(THIS_MODULE, CLASS_NAME);
      if (IS_ERR(lab2_class)) {
          ret = PTR_ERR(lab2_class);
          unregister_chrdev(DRIVER_MAJOR, DRIVER_NAME);
          return ret;
      }

      lab2_device = device_create(
          lab2_class,
          NULL,
          MKDEV(DRIVER_MAJOR, 0),
          NULL,
          DRIVER_NAME
      );

  device_create_file(lab2_device, &dev_attr_reverse_read);

      if (IS_ERR(lab2_device)) {
          ret = PTR_ERR(lab2_device);
          class_destroy(lab2_class);
          unregister_chrdev(DRIVER_MAJOR, DRIVER_NAME);
          return ret;
      }

      ret = sysfs_create_group(
          &lab2_device->kobj,
          &lab2_attr_group
      );

      if (ret) {
  device_remove_file(lab2_device, &dev_attr_reverse_read);        
  device_destroy(lab2_class, MKDEV(DRIVER_MAJOR, 0));
          class_destroy(lab2_class);
          unregister_chrdev(DRIVER_MAJOR, DRIVER_NAME);
          return ret;
      }

      lab2_proc = proc_create(
          "lab2_info",
          0444,
          NULL,
          &lab2_proc_ops
      );

      if (!lab2_proc) {
          sysfs_remove_group(
              &lab2_device->kobj,
              &lab2_attr_group
          );
          device_destroy(lab2_class, MKDEV(DRIVER_MAJOR, 0));
          class_destroy(lab2_class);
          unregister_chrdev(DRIVER_MAJOR, DRIVER_NAME);
          return -ENOMEM;
      }

      pr_info("lab2: driver initialized, major=%d\n",
              DRIVER_MAJOR);
      pr_info("lab2: /proc/lab2_info created\n");
      pr_info("lab2: sysfs attributes created\n");

      return 0;
  }

  /* Remove driver */
  static void __exit lab2_exit(void)
  {
      proc_remove(lab2_proc);

      sysfs_remove_group(
          &lab2_device->kobj,
          &lab2_attr_group
      );

      device_destroy(lab2_class, MKDEV(DRIVER_MAJOR, 0));
      class_destroy(lab2_class);
      unregister_chrdev(DRIVER_MAJOR, DRIVER_NAME);

      pr_info("lab2: driver removed\n");
  }

  module_init(lab2_init);
  module_exit(lab2_exit);

  MODULE_LICENSE("GPL");
  MODULE_AUTHOR("Embedded Linux Lab");
  MODULE_DESCRIPTION("LAB-02 Character Device, procfs and sysfs");
