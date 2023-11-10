#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kobject.h> 
#include <linux/sysfs.h>
#include <asm/processor.h>
#include <asm/segment.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junior Ravhuravhu");
MODULE_DESCRIPTION("Data Storage Character Device Driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME "mychardevice"
#define BUF_SIZE 1024

static int major_number;
static char buffer[BUF_SIZE];
static int buffer_size = 0;

static int device_open(struct inode *inode, struct file *file) {
    // device already exists
    if (buffer_size > 0) {
        printk(KERN_ALERT "mychardevice is already open.\n");
        return -EBUSY;
    }

    // usage count -increase
    try_module_get(THIS_MODULE);
    
    printk(KERN_INFO "\n___________________\nmychardevice opened.\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    // usage count - decrease
    module_put(THIS_MODULE);
    
    printk(KERN_INFO "mychardevice closed.\n___________________\n");
    return 0;
}

// Device Read
static ssize_t device_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset) {
    int bytes_to_read = min(count, buffer_size);

    // if no data to read
    if (bytes_to_read <= 0) {
        return 0;
    }

    if (copy_to_user(user_buffer, buffer, bytes_to_read)) {
        return -EFAULT;
    }

    // Clear the buffer after reading
    memset(buffer, 0, sizeof(buffer));
    buffer_size = 0;

    return bytes_to_read;
}

// Device file operations
static ssize_t device_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset) {
    char request[256];
    int bytes_to_copy = min(count, sizeof(request) - 1);

    if (copy_from_user(request, user_buffer, bytes_to_copy)) {
        printk(KERN_ALERT "Failed to copy request from user space.\n");
        return -EFAULT;
    }

    request[bytes_to_copy] = '\0';

    if (strncmp(request, "CREATE:", 7) == 0) {
        // Request to create a file
        const char *actual_filename = request + 7;
        struct file *new_file;
        int error;

        new_file = filp_open(actual_filename, O_WRONLY | O_CREAT, 0644);
        if (IS_ERR(new_file)) {
            error = PTR_ERR(new_file);
            printk(KERN_ALERT "Failed to create file '%s' (error code: %d).\n", actual_filename, error);
        } else {
            filp_close(new_file, NULL);
            printk(KERN_INFO "File '%s' is created Successfully.\n", actual_filename);
        }
    } else if (strncmp(request, "DELETE:", 7) == 0) {
        // Request to delete a file
        const char *filename = request + 7;
        struct path path;
        int error;

        error = kern_path(filename, LOOKUP_FOLLOW, &path);
        if (!error) {
            char path_buf[256];
            char *path_str = d_path(&path, path_buf, sizeof(path_buf));
            if (IS_ERR(path_str)) {
                printk(KERN_ALERT "Failed to locate file '%s'.\n", filename);
            } else {
                printk(KERN_INFO "File '%s' located at: %s\n", filename, path_str);

                // File deletion
                error = vfs_unlink(path.dentry->d_parent->d_inode->i_sb->s_user_ns,
                    path.dentry->d_parent->d_inode, path.dentry, NULL);

                if (error) {
                    printk(KERN_ALERT "Failed to delete file '%s' (error code: %d).\n", filename, error);
                } else {
                    printk(KERN_INFO "File '%s' is deleted. Successfully.\n", filename);
                }
            }
            path_put(&path);
        } else {
            printk(KERN_ALERT "Failed to find file '%s' (error code: %d).\n", filename, error);
        }
    } else if (strncmp(request, "WRITE:", 6) == 0) {
        // Request to write data to a file
        const char *write_request = request + 6;
        char *filename = NULL;
        char *data = strchr(write_request, ':');
        if (data) {
            *data = '\0';
            data++;
            filename = write_request;
            struct file *file;
            loff_t pos = 0;
            int error;

            file = filp_open(filename, O_WRONLY, 0);
            if (IS_ERR(file)) {
                error = PTR_ERR(file);
                printk(KERN_ALERT "Failed to open file '%s' (error code: %d).\n", filename, error);
            } else {
                // data to the file
                error = kernel_write(file, data, strlen(data), &pos);

                filp_close(file, NULL);

                if (error >= 0) {
                    printk(KERN_INFO "Data written to file '%s'.\n", filename);
                } else {
                    printk(KERN_ALERT "Failed to write data to file '%s' (error code: %d).\n", filename, error);
                }
            }
        } else {
            printk(KERN_ALERT "Invalid WRITE request format. Use 'WRITE:<filename>:<data>'.\n");
        }
    } else if (strncmp(request, "READ:", 5) == 0) {
        // Request to read data from a file
        const char *filename = request + 5;
        struct file *file;
        loff_t pos = 0;
        int error;

        file = filp_open(filename, O_RDWR, 0);

        if (IS_ERR(file)) {
            error = PTR_ERR(file);
            printk(KERN_ALERT "Failed to open file '%s' (error code: %d).\n", filename, error);
        } else {
            // Read data
            error = kernel_read(file, buffer, sizeof(buffer), &pos);

            filp_close(file, NULL);

            if (error >= 0) {
                buffer_size = error; // Set the buffer size to the actual data read
                printk(KERN_INFO "Data read from file '%s': %s\n", filename, buffer);
            } else {
                printk(KERN_ALERT "Failed to read data from file '%s' (error code: %d).\n", filename, error);
            }
        }
    }

    return bytes_to_copy;
}

static struct file_operations file_ops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

// Register the character device
static int __init mychardevice_init(void) {
    major_number = register_chrdev(0, DEVICE_NAME, &file_ops);
    if (major_number < 0) {
        printk(KERN_ALERT "Failed to register a major number for mychardevice.\n");
        return major_number;
    }

    printk(KERN_INFO "mychardevice registered with major number %d.\n", major_number);

    return 0;
}

// Unregister the character device
static void __exit mychardevice_exit(void) {
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "mychardevice unregistered.\n");
}

module_init(mychardevice_init);
module_exit(mychardevice_exit);