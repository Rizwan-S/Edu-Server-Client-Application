// OS project MsgQ Client process
// Compilation of this file
// gcc -o msgqclt Client.c -lrt

//Name: Mohd. Rizwan Shaikh
//Roll no. IMT2019513

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <string.h>

#define MSG_VAL_LEN  16
#define CLIENT_Q_NAME_LEN 16
#define MSG_TYPE_LEN 16

typedef struct
{
    char client_q[CLIENT_Q_NAME_LEN];
    char msg_val[MSG_VAL_LEN];
} client_msg_t;

typedef struct
{
    char msg_val[MSG_VAL_LEN];
} server_msg_t;

#define SERVER_QUEUE_NAME   "/server_msgq"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE sizeof(client_msg_t)
#define MSG_BUFFER_SIZE (MAX_MSG_SIZE * MAX_MESSAGES)

void InterpretServerCoder(int code);
int instructNum = 1;

int main (int argc, char **argv)
{
    mqd_t qd_srv, qd_client; 
    char instruction[MSG_VAL_LEN];

    if((qd_srv = mq_open(SERVER_QUEUE_NAME, O_WRONLY)) == -1) 
    {
        perror("Client MsgQ: mq_open(qd_srv)");
        exit(1);
    }

    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    client_msg_t out_msg;
 
    sprintf(out_msg.client_q, "/clientQ-%d", getpid());

    if((qd_client = mq_open(out_msg.client_q, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) 
    {
        perror("Client msgq: mq_open (qd_client)");
        exit(1);
    }
    printf("Client MsgQ: Welcome!!!\n");
    
    while(1) 
    {
        printf("\n%d: \e[1;37mEnter your instruction:\033[0m ", instructNum);
        scanf("%[^\n]%*c", instruction);
		sprintf (out_msg.msg_val, "%s", instruction);         
        if(mq_send(qd_srv, (char *) &out_msg, sizeof(out_msg), 0) == -1) 
        {
            perror("\e[1;37mClient MsgQ:\033[0m \e[0;31mNot able to send message to the queue /server_msgq\033[0m");
            continue;
        }

        printf ("%d: \e[1;37mClient MsgQ:\033[0m \e[0;32mMessage sent successfully\033[0m\n", instructNum);

        sleep(1);

		server_msg_t in_msg;
        if(mq_receive(qd_client,(char *) &in_msg, MAX_MSG_SIZE, NULL) == -1) 
        {
            perror ("Client MsgQ: mq_receive from server");
            exit (1);
        }
        int svrCode = atoi(in_msg.msg_val);
        InterpretServerCoder(svrCode);
        // printf("%d: Client MsgQ: Msg received from the server with val = %s\n", instructNum, in_msg.msg_val);
        instructNum++;
    }
    printf ("Client MsgQ: bye\n");
    exit (0);
}

void InterpretServerCoder(int code)
{
    switch(code)
    {
        case 1:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;32mSuccessfully added all the courses\033[0m\n", instructNum);
            break;
        case 2:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;32mSuccessfully added all the teachers\033[0m\n", instructNum);
            break;
        case 3:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;32mSuccessfully deleted all the entered courses\033[0m\n", instructNum);
            break;
        case 4:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;32mSuccessfully deleted all the entered teachers\033[0m\n", instructNum);
            break;
            
        case -1:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;31mSome or all courses not added as they already exist\033[0m\n", instructNum);
            break;
        case -2:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;31mSome or all teachers not added as they already exist\033[0m\n", instructNum);
            break;
        case -3:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;31mMax count of courses reached. Cannot add more courses\033[0m\n", instructNum);
            break;
        case -4:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;31mMax count of teachers reached. Cannot add more teachers\033[0m\n", instructNum);
            break;
        case -5:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;31mMin count of courses reached. Cannot delete more courses\033[0m\n", instructNum);
            break;
        case -6:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;31mMin count of teachers reached. Cannot delete more teachers\033[0m\n", instructNum);
            break;
        case -7:
            printf("%d: \e[1;37mClient MsgQ:\033[0m \e[0;31mNo course found. Cannot add teacher\033[0m\n", instructNum);
            break;   
        default:
            printf("%d: Client MsgQ: \e[0;33mNo operation performed in server\033[0m\n", instructNum);
    }
}
