// OS project MsgQ Server process
// Compilation of this file
// gcc -D_REENTRANT -o msgqsrv Server.c -lrt -lpthread

//Name: Mohd. Rizwan Shaikh
//Roll no. IMT2019513

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <semaphore.h>
#include <unistd.h>  
#include <pthread.h>

#define MSG_VAL_LEN  16
#define CLIENT_Q_NAME_LEN 16
#define MSG_TYPE_LEN 16

#define MIN_COURSES 10
#define MAX_COURSES 15
#define MIN_TEACHERS 5
#define MAX_TEACHERS 10

int minCourses = -1;
int maxCourses = -1;
int minTeachers = -1;
int maxTeachers = -1;

typedef struct
{
    char client_q[CLIENT_Q_NAME_LEN];
    char msg_val[MSG_VAL_LEN];
} client_msg_t;

typedef struct
{
    char msg_val[MSG_VAL_LEN];
} server_msg_t;

typedef struct
{
    char *name;
    char *teachers[MAX_TEACHERS];
    int valid;
} course;

course allCourses[MAX_COURSES];
char *allTeachers[MAX_TEACHERS];

static client_msg_t client_msg;

#define SERVER_QUEUE_NAME   "/server_msgq"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE sizeof(client_msg_t)
#define MSG_BUFFER_SIZE (MAX_MSG_SIZE * MAX_MESSAGES) 

int executeClientInstruction(char instruction[]);
void *generateReport(void *pThreadName);
void INThandler(int signal);

int num = 1;
int courseCount = 0;
int teacherCount = 0;
sem_t bin_sem;

int main (int argc, char **argv)
{
    signal(SIGINT, INThandler);
    printf ("Server MsgQ: Welcome!!!\n\n");

    printf("Enter MIN_COURSES Count: ");
    scanf("%d", &minCourses);

    printf("Enter MAX_COURSES Count: ");
    scanf("%d", &maxCourses);

    printf("Enter MIN_TEACHERS Count: ");
    scanf("%d", &minTeachers);

    printf("Enter MAX_TEACHERS Count: ");
    scanf("%d", &maxTeachers);

    if(minCourses < MIN_COURSES || minCourses > MAX_COURSES)
        minCourses = MIN_COURSES;
    if(maxCourses > MAX_COURSES || maxCourses < MIN_COURSES)
        maxCourses = MAX_COURSES;
    if(minTeachers < MIN_TEACHERS || minTeachers > MAX_TEACHERS)
        minTeachers = MIN_TEACHERS;
    if(maxTeachers > MAX_TEACHERS || maxTeachers < MIN_TEACHERS)
        maxTeachers = MAX_TEACHERS;

    printf("\nMIN_COURSES: %d\nMAX_COURSES: %d\nMIN_TEACHERS: %d\nMAX_TEACHERS: %d\n\n", minCourses, maxCourses, minTeachers, maxTeachers);

    mqd_t qd_srv, qd_client; 

    int res_sem, res1;
    pthread_t thread;

    res_sem = sem_init(&bin_sem, 0, 1);  
    if(res_sem != 0) 
    {
        printf("Semaphore creation failure: %d\n", res_sem);
        exit(1);
    } 

    if((res1 = pthread_create(&thread, NULL, &generateReport, "Report Generator Thread")))  
    {
        printf("Thread creation failed: %d\n", res1);
        exit(1);
    }

    char userCommand[200];

    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    if((qd_srv = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) 
    {
        perror("Server MsgQ: mq_open (qd_srv)");
        exit(1);
    }

    client_msg_t in_msg;
    int returnCode = 1000;
    while(1) 
    {
        if(mq_receive(qd_srv,(char *) &in_msg, MAX_MSG_SIZE, NULL) == -1) 
        {
            perror("Server msgq: mq_receive");
            exit(1);
        }

        printf("\n%d: Client msg q name = %s\n", num, in_msg.client_q);
        printf("%d: Client msg val = %s\n", num, in_msg.msg_val);

        returnCode = executeClientInstruction(in_msg.msg_val);

		server_msg_t out_msg; 
		sprintf(out_msg.msg_val, "%d", returnCode);    
		             		       	
        if((qd_client = mq_open(in_msg.client_q, O_WRONLY)) == 1) 
        {
            perror("Server MsgQ: Not able to open the client queue");
            continue;
        }     
        
        if(mq_send(qd_client, (char *) &out_msg, sizeof(out_msg), 0) == -1) 
        {
            perror("Server MsgQ: Not able to send message to the client queue");
            continue;
        }          
        num++;
    } 
} 

void  INThandler(int sig_num)
{
    signal(sig_num, SIG_IGN);
    //Write to text file
    FILE *fptr;

    fptr = fopen("Report.txt", "w");

    if(fptr == NULL)
    {
        printf("\e[0;31mError!\033[0m\n");   
        exit(1);             
    }

    fprintf(fptr, "STATISTICS\n");
    fprintf(fptr, "MIN_COURSES SET  : %d\n", minCourses);
    fprintf(fptr, "MAX_COURSES SET  : %d\n", maxCourses);
    fprintf(fptr, "MIN_TEACHERS SET : %d\n", minTeachers);
    fprintf(fptr, "MAX_TEACHERS SET : %d\n\n", maxTeachers);

    fprintf(fptr, "COURSES COUNT    : %d\n", courseCount);
    fprintf(fptr, "TEACHERS COUNT   : %d\n\n", teacherCount);
    
    fprintf(fptr, "ALL COURSES:\n");
    for(int i = 0; i < maxCourses; i++)
    {
        if(allCourses[i].valid == 1)
        {
            fprintf(fptr, "COURSE NAME: %s\n", allCourses[i].name);
            fprintf(fptr, "COURSE TEACHER: ");
            for(int j = 0; j < maxTeachers; j++)
            {
                if(allCourses[i].teachers[j] != NULL)
                    fprintf(fptr, "%s, ", allCourses[i].teachers[j]);
            }
            fprintf(fptr, "\n");
        }
    }
    fprintf(fptr, "\nALL TEACHERS:\n");
    for(int i = 0; i < maxTeachers; i++)
    {
        if(allTeachers[i] != NULL)
            fprintf(fptr, "%s\n", allTeachers[i]);
    }

   fclose(fptr);

    printf("\e[0;32mData stored in Report.txt text file successfully\033[0m\n");
    exit(0);
}

void *generateReport(void *pThreadName)
{
    printf("\e[1;32m%s is entered\e[0m\n", (char *) pThreadName);
    printf("\e[3mReport will be generated every 10 seconds ...\e[0m\n");
    while(1)
    {
        sem_wait(&bin_sem);
        printf("\n\e[1;37m\e[3m:::::::::::::::REPORT:::::::::::::::\e[0m\n");

        printf("\e[4;37m\e[1;37mSTATISTICS:\e[0m\n");
        printf("MIN_COURSES SET  : %d\n", minCourses);
        printf("MAX_COURSES SET  : %d\n", maxCourses);
        printf("MIN_TEACHERS SET : %d\n", minTeachers);
        printf("MAX_TEACHERS SET : %d\n\n", maxTeachers);

        printf("COURSES COUNT    : %d\n", courseCount);
        printf("TEACHERS COUNT   : %d\n\n", teacherCount);

        int k = 1;
        printf("\e[4;37m\e[1;37mALL COURSES:\e[0m\n");
        for(int i = 0; i < MAX_COURSES; i++)
        {
            if(allCourses[i].valid == 1)
            {
                printf("%d: COURSE NAME:     %s\n", k, allCourses[i].name);
                printf("%d: COURSE TEACHERS:", k);
                for(int j = 0; j < MAX_TEACHERS; j++)
                {
                    if(allCourses[i].teachers[j] != NULL)
                    printf(" %s,", allCourses[i].teachers[j]);
                }
                printf("\n");
                k++;
            }
        }

        printf("\n\e[4;37m\e[1;37mALL TEACHERS:\e[0m\n");
        k = 1;
        for(int i = 0; i < MAX_TEACHERS; i++)
        {
            if(allTeachers[i] != NULL && allTeachers[i] != "\0")
            {
                printf("%d: TEACHER NAME:  %s \n", k, allTeachers[i]);
                k++;
            }
        }
        printf("\n\e[1;37m\e[3m====================================\e[0m\n");

        sem_post(&bin_sem);
        sleep(10);
    }
}

int executeClientInstruction(char instruction[])
{
    sem_wait(&bin_sem);
    char *token;
    token = strtok(instruction, " ");
    if(!strcmp(token, "ADD_COURSE") || !strcmp(token, "ADDC") || !strcmp(token, "ADD_C"))
    { 
        int returnCode = 1;
        token = strtok(NULL, " ");
        while(token != NULL)
        {
            if(token[strlen(token)-1] == ',')
                token[strlen(token)-1] = '\0';

            int freeblock = 0;
            for(int i = 0; i < maxCourses; i++)
            {
                if(allCourses[i].valid == 0)
                    freeblock = 1;
            }
            if(freeblock == 0)
            {
                printf("%d: Max count of courses reached. Unable to add more courses\n", num);
                sem_post(&bin_sem);
                return -3;
            }
                
            int freeCourse = -1;
            for(int i = 0; i < maxCourses; i++)
            {
                if(allCourses[i].name != NULL)
                {
                    if(!strcmp(allCourses[i].name, token) && allCourses[i].valid == 1)
                    {
                        freeCourse = -1; 
                        returnCode = -1;
                        break;
                    }
                    if(allCourses[i].valid == 0)
                        freeCourse = i;
                }
                else
                    freeCourse = i;
            }
            if(freeCourse >= 0)
            {
                allCourses[freeCourse].name = malloc(strlen(token)+1);
                strcpy(allCourses[freeCourse].name, token);

                for(int i = 0; i < maxTeachers; i++)
                    allCourses[freeCourse].teachers[i] = NULL;

                allCourses[freeCourse].valid = 1;
                printf("%d: Course added: %s\n", num, allCourses[freeCourse].name);
                courseCount++;
            }
            token = strtok(NULL, " ");
        }

        if(returnCode == -1)
            printf("%d: Few or all the courses not added due to repetition\n", num);

        sem_post(&bin_sem);
        return returnCode;
    }
    else if(!strcmp(token, "ADD_TEACHER") || !strcmp(token, "ADDT") || !strcmp(token, "ADD_T"))
    {
        int returnCode = 2;
        token = strtok(NULL, " ");
        int addedCount = 0, totalCount = 0, teacherExists, courseThere;
        while(token != NULL)
        {
            teacherExists = 0;
            courseThere = 0;
            if(token[strlen(token)-1] == ',')
                token[strlen(token)-1] = '\0';

            for(int i = 0; i < maxTeachers; i++)
            {   
                if(allTeachers[i] != NULL && !strcmp(allTeachers[i], token))
                    teacherExists = 1;         
            }   

            for(int i = 0; i < maxCourses; i++)
            {
                if(allCourses[i].valid == 1)
                    courseThere = 1;  
            }
            if(courseThere == 0)
            {
                printf("%d: No course found. Cannot add teacher\n", num);
                sem_post(&bin_sem);
                return -7;
            }

            if(teacherCount == maxTeachers)
            {
                printf("%d: Max count of teachers reached. Cannot add more teachers\n", num);
                sem_post(&bin_sem);
                return -4;
            }
            if(teacherExists == 0 && courseThere == 1)
            {
                for(int i = 0; i < maxTeachers; i++)
                {
                    if(allTeachers[i] == NULL)
                    {
                        allTeachers[i] = malloc(strlen(token) + 1);
                        strcpy(allTeachers[i], token);
                        
                        int randNum = rand() % maxCourses;
                        while(allCourses[randNum].valid == 0)
                            randNum = rand() % maxCourses;
                        
                        for(int j = 0; j < maxTeachers; j++)
                        {
                            if(!allCourses[randNum].teachers[j])
                            {
                                allCourses[randNum].teachers[j] = malloc(strlen(token)+1);
                                strcpy(allCourses[randNum].teachers[j], token);
                                printf("%d: Teacher added: %s\n", num, allCourses[randNum].teachers[j]);
                                addedCount++;
                                teacherCount++;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            totalCount++;
            token = strtok(NULL, " ");
        }
        if(addedCount != totalCount)
        {
            returnCode = -2;
            printf("%d: Few or all the teachers not added\n", num);
        }
        sem_post(&bin_sem);
        return returnCode;
    }
    else if(!strcmp(token, "DEL_TEACHER") || !strcmp(token, "DELT") || !strcmp(token, "DEL_T"))
    {
        int returnCode = 4;
        token = strtok(NULL, " ");
        while(token != NULL)
        {
            if(token[strlen(token)-1] == ',')
                token[strlen(token)-1] = '\0';
            
            if(teacherCount <= minTeachers)
                returnCode = -6;

            if(returnCode == 4)
            {
                for(int i = 0; i < maxTeachers; i++)
                {
                    if(allTeachers[i] != NULL && !strcmp(allTeachers[i], token))
                    {
                        allTeachers[i] = NULL;
                        for(int j = 0; j < maxCourses; j++)
                        {
                            if(allCourses[j].valid == 1)
                            {
                                for(int k = 0; k < maxTeachers; k++)
                                {
                                    if(allCourses[j].teachers[k] != NULL && !strcmp(allCourses[j].teachers[k], token))
                                    {
                                        allCourses[j].teachers[k] = NULL;
                                        break;
                                    }
                                }
                            }
                        }
                        teacherCount--;
                        break;
                    }
                }
            }

            token = strtok(NULL, " ");
        }
        sem_post(&bin_sem);
        return returnCode;
    }
    else if(!strcmp(token, "DEL_COURSE") || !strcmp(token, "DELC") || !strcmp(token, "DEL_C"))
    {
        token = strtok(NULL, " ");
        int returnCode = 3;
        while(token != NULL)
        {
            if(token[strlen(token)-1] == ',')
                token[strlen(token)-1] = '\0';
            
            if(courseCount <= minCourses)
                returnCode = -5;

            if(returnCode == 3)
            {   
                for(int i = 0; i < maxCourses; i++)
                {
                    if(allCourses[i].valid == 1 && !strcmp(allCourses[i].name, token))
                    {
                        allCourses[i].valid = 0;
                        allCourses[i].name = NULL;
                        for(int j = 0; j < maxTeachers; j++)
                            allCourses[i].teachers[j] = NULL;                         
                        courseCount--;
                        break;
                    }
                }
            }
            token = strtok(NULL, " ");
        }
        sem_post(&bin_sem);
        return returnCode;
    }
    sem_post(&bin_sem);
    return 0;
}
