#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

//maximum number of processes to be entered = 100
#define MAX 100

//sem_t empty;
//sem_t full;


typedef struct Process
{
    int id;
    int burst;// time of the process
    struct Process* next;
} Process;

typedef struct
{
    Process* head;
    Process* tail;
    int size;
    int processes_no;//number of current processes in the queue;
} Queue;

// a struct to send the three queues as arguments to the functions
typedef struct
{
    Queue*q1;
    Queue*q2;
    Queue*q3;
    int creation_flag;// flag to detect the finishing of creation
} args;

int rand_num(int min, int max)
{
    return min + rand() % (max - min + 1);
}

Process* create_process(int id)
{
    Process* process = malloc(sizeof(Process));
    process->id = id;
    process->burst = rand_num(1, 100); // select burst time random between 1 and 100
    process->next = NULL;
    return process;
}

// queue implementation functions
void initializeQueue(Queue* queue,int s)
{
    queue->head =NULL;
    queue->tail =NULL;
    queue->size =s;
    queue->processes_no=0;
}

int isEmpty(Queue* queue)
{
    if(queue->processes_no == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}

int  isFull(Queue* queue)
{
    if(queue->size == queue->processes_no)
    {
        return 1;
    }
    else
    {
        return 0;
    }


}

void insert_process_to_queue(Queue* queue,Process* process)
{
    if(isFull(queue))
    {
        printf("queue is full\n");
        return;
    }
    if (isEmpty(queue))
    {
        queue->head = process;
        queue->tail = process;
    }
    else
    {
        queue->tail->next = process;
        queue->tail = process;
    }
    queue->processes_no++;
}


Process* extract_from_queue(Queue* queue)
{
    if (isEmpty(queue))
    {
        printf("Queue is empty\n");
        return NULL;
    }
    else
    {
        Process* process = queue->head;
        queue->head = process->next;
        process->next = NULL;
        queue->processes_no--;
        return process;
    }
}

//function to print completed process
void finish_process(Process* process)
{
    printf("Process %d: completed successfully - burst_time: %d\n\n", process->id, process->burst);
}


// function to print queue elements while tracing
void print_Queue(Queue* queue)
{
    if (isEmpty(queue))
    {
        printf("Queue is empty\n");
    }
    else
    {
        Process* current = queue->head;
        printf("Queue processes: ");
        while (current != NULL)
        {
            printf("%d,%d\n", current->id, current->burst);
            current = current->next;
        }
        printf("\n");
    }
}



//thread function responsible for creating process
void* thread_create_process(void* arg)
{

    args* queues = (args*)arg;

    for (int i = 1; i <=MAX; i++)
    {
        Process* process = create_process(i);
        if(isFull(queues->q1))//waiting amount of time because the queue is full so i can insert process
        {
            usleep(100000);
        }

        insert_process_to_queue(queues->q1, process);
    }

    queues->creation_flag = 1;//after completing creation setting flag=1
    pthread_exit(NULL);

}

//thread function responsible for excuting process
void* thread_excute_process(void* arg)
{
    args* queues = (args*)arg;
    while (!isEmpty(queues->q1)||!isEmpty(queues->q2)||!isEmpty(queues->q3)||!queues->creation_flag)
    {
        //generate random number for the cpu access on the queues to check it
        int queue_enter=rand_num(1,10);
        //First queue (quantum = 8, possibility 50%)
        if(queue_enter<=5&&!isEmpty(queues->q1))
        {
            Process*process=extract_from_queue(queues->q1);
            process->burst=process->burst-8;
            if(process->burst>0)
            {
                insert_process_to_queue(queues->q2,process);
            }
            else
            {
                finish_process(process);
            }

        }
        //Second queue (quantum =16 ,possibility 30%)
        else if(queue_enter>5&&queue_enter<=8&&!isEmpty(queues->q2))
        {
            Process*process=extract_from_queue(queues->q2);
            process->burst=process->burst-16;

            if(process->burst>0)
            {
                // probability 50% to go up to queue1(quantum=8) and 50% down to queue2(FCFS)
                int rand_probability=rand_num(1,10);
                if(rand_probability>=1&&rand_probability<=5)
                {
                    if(!isFull(queues->q1))
                    {
                        insert_process_to_queue(queues->q1,process);
                    }
                }
                else if(!isFull(queues->q3))
                {
                    insert_process_to_queue(queues->q3,process);
                }

            }
            else
            {
                finish_process(process);
            }

        }

        //Third queue (FCFS , possibility=20%)
        else if(!isEmpty(queues->q3))
        {
            Process*process=extract_from_queue(queues->q3);
            finish_process(process);

        }

    }

    pthread_exit(NULL);
}


int main()
{
    Queue queue1, queue2, queue3;
    initializeQueue(&queue1,10);
    initializeQueue(&queue2, 20);
    initializeQueue(&queue3, 30);


    pthread_t create_t, excute_t;
    args args;
    args.q1= &queue1;
    args.q2= &queue2;
    args.q3= &queue3;
    args.creation_flag=0;

    // Creating threads for creating processes and executing processes
    int createthread_success=pthread_create(&create_t, NULL, thread_create_process, &args);
    if (createthread_success != 0)
    {
        printf("Error in creating (create process thread)\n");
        exit(1);
    }

    int excutethread_success=pthread_create(&excute_t, NULL, thread_excute_process, &args);
    if (excutethread_success != 0)
    {
        printf("Error in creating (excute process thread)\n");
        exit(1);
    }

    // Waiting for threads to finish
    pthread_join(create_t, NULL);
    pthread_join(excute_t, NULL);


    return 0;
}
