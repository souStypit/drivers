#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define DEVICE_1 "/dev/scull1"
#define DEVICE_2 "/dev/scull2"
#define BUFF_SIZE 1024

pid_t pid;
char client_name[16];

void scanf_sequence(char (*resultPtr)[BUFF_SIZE]) {
    char ch, *res = *resultPtr;
    int i = sprintf(res, "\nclient-%d: ", pid);

    getchar();
    while ((ch = getchar()) != '\n') {
        res[i++] = ch; 
    }
    res[i] = '\0';
}

void read_room(int fd) {
    char read_buf[BUFF_SIZE];
    while (read(fd, read_buf, sizeof(read_buf)) > 0) { 
        printf("%s", read_buf);
    }
    printf("\n");
}

void write_room(int fd) {
    char write_buf[BUFF_SIZE];
    scanf_sequence(&write_buf);
    lseek(fd, 0, SEEK_END);
    write(fd, write_buf, sizeof(write_buf));
}

int main() {
    int fd, room_n, exit = 0, conn = 0, choice;
    const char *room_set[] = { DEVICE_1, DEVICE_2 };

    pid = getpid();
    
    while (!exit) {
        if (!conn) {
            printf("choose room (1,2) or exit (3): \n");
            scanf("%d", &room_n);
            if (room_n == 3) exit = 1;
            if (room_n > 2 || room_n < 1) continue;
            conn = 1;
        }

        fd = open(room_set[room_n - 1], O_RDWR);
        if (fd < 0) {
            if (fd == -1) {
                printf("Error %d: max number of users - 4!\n", fd);
                conn = 0;
                continue;
            }

            return -1;
        }

        printf("1-choose_room, 2-read, 3-write, 4-exit\n");
        scanf("%d", &choice);
        switch (choice) {
        case 1:
            conn = 0;
            break;
        case 2:
            printf("history:");
            read_room(fd);
            break;
        case 3:
            printf("enter data: ");
            write_room(fd);
            break;
        case 4:
            exit = 1;
            break;
        }

        close(fd);
    }

    return 0;
}