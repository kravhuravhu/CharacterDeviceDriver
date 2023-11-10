#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/fs.h>

#define DEVICE_FILE "/dev/mychardevice"
#define BUF_SIZE 1024


// Mmenu
void display_menu() {
    printf("\nMenu Options\n");
    printf("1. Create a file\n");
    printf("2. Delete a file\n");
    printf("3. List files\n");
    printf("4. Read data from a file\n");
    printf("5. Write data to a file\n");
    printf("6. Exit\n");
    printf("\nEnter your choice: ");
}

// Create file
void create_file() {
    int device_fd = open(DEVICE_FILE, O_WRONLY);
    if (device_fd == -1) {
        perror("Failed to open the device file");
        return;
    }

    char filename[256];
    printf("Enter a file name: ");
    scanf("%s", filename);

    // request to create
    char request[256];
    snprintf(request, sizeof(request), "CREATE:%s", filename);

    if (write(device_fd, request, strlen(request)) < 0) {
        perror("Failed to create a file");
    } else {
        printf("File created successfully.\n");
    }

    close(device_fd);
}

// Delete file
void delete_file() {
    char filename[256];
    printf("Enter a file name: ");
    scanf("%s", filename);

    // if the file exists
    if (access(filename, F_OK) != -1) {
        int device_fd = open(DEVICE_FILE, O_WRONLY);

        if (device_fd == -1) {
            perror("Failed to open the device file");
            return;
        }

        // request to delete
        char request[256];
        sprintf(request, "DELETE:%s", filename);

        if (write(device_fd, request, strlen(request)) < 0) {
            perror("Failed to delete file");
        } else {
            printf("File deleted successfully.\n");
        }

        close(device_fd);
    } else {
        printf("The file '%s' does not exist.\n", filename);
    }
}

// Function to list files
void list_files() {
    DIR *dir;
    struct dirent *ent;

    printf("\nList of files:\n\n");

    if ((dir = opendir(".")) != NULL) {
        int i = 0;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                printf("File #%d: %s\n", i, ent->d_name);
                i++;
            }
        }
        closedir(dir);
        printf("____________\n");
    } else {
        perror("\nError reading directory\n\n");
    }
}

// Read data a file contetn
void read_data() {
    char filename[256];
    printf("Enter a file name: ");
    scanf("%s", filename);

    // Check if the file exists
    if (access(filename, F_OK) == -1) {
        printf("The file '%s' does not exist.\n", filename);
        return;
    }

    // Open the device
    int device_fd = open(DEVICE_FILE, O_RDWR);

    if (device_fd == -1) {
        perror("Failed to open the device file");
        return;
    }

    // request to read
    char request[256];
    snprintf(request, sizeof(request), "READ:%s", filename);

    if (write(device_fd, request, strlen(request)) < 0) {
        perror("Failed to read data from the file");
    } else {
        // Read data sent by the module
        char response[1024];
        int bytesRead = read(device_fd, response, sizeof(response));
        if (bytesRead < 0) {
            perror("Failed to read data from the kernel");
        } else {
            response[bytesRead] = '\0';
            printf("Data read from the file '%s':\n%s\n", filename, response);
        }
    }

    close(device_fd);
}

// Write to file
void write_data() {
    char filename[256];
    char data[1024];

    printf("Enter a file name: ");
    scanf("%s", filename);

    // if the file exists
    if (access(filename, F_OK) == -1) {
        printf("The file '%s' does not exist.\n", filename);
        return;
    } else {
        getchar();
        printf("Enter data to write: ");
        fgets(data, sizeof(data), stdin);

        int device_fd = open(DEVICE_FILE, O_WRONLY);
        if (device_fd == -1) {
            perror("Failed to open the device file");
            return;
        }

        // Create a request 
        char request[256];
        snprintf(request, sizeof(request), "WRITE:%s:%s", filename, data);

        if (write(device_fd, request, strlen(request)) < 0) {
            perror("Failed to write data to file");
        } else {
            printf("Data written to file successfully.\n");
        }

        close(device_fd);
    }
}

int main() {
    while (1) {
        display_menu();

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                create_file();
                break;
            case 2:
                delete_file();
                break;
            case 3:
                list_files();
                break;
            case 4:
                read_data();
                break;
            case 5:
                write_data();
                break;
            case 6:
                printf("Exiting...\n");
                return 0;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}