#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <mqueue.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#define MAXBUF PIPE_BUF
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define SVMSG_MODE (S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP| S_IROTH)
#define PMODE 0666

#define MQ_KEY1 1234L
#define MQ_KEY2 1345L
#define MQ_KEY3	"/queue1"
#define MQ_KEY4 "/queue2"
typedef struct {
        long mesg_len;                   /* msg length in bytes */
        long mesg_type;                  /* msg type */
        char mesg_data[MAXBUF];         /* buffer address */
}Mesg;
