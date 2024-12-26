#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSIZE 100
#define MAX_QUEUE_PROCESSES 20
#define MAXQUEUES 10
#define MAXPOLICIES 10

typedef struct
{
    char name;
    int arrival;
    int serviceOrPriority;
    int start;
    int finish;
    int executed;
    int remaining;
    int enteredQueue;

    // for aging
    int waiting;
    int priority;
    char progress[MAXSIZE];
} Process;

typedef struct
{
    Process *queue[MAX_QUEUE_PROCESSES];
    int front;
    int rear;
} Queue;

void initQueue(Queue *q)
{
    q->front = q->rear = 0;
}

int isEmpty(Queue *q)
{
    return q->front == q->rear;
}

void enqueue(Queue *q, Process *process)
{
    q->queue[q->rear] = process;
    q->rear = (q->rear + 1) % MAX_QUEUE_PROCESSES;
}

Process *dequeue(Queue *q)
{
    Process *p = q->queue[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_PROCESSES;
    return p;
}

void initProgress(Process *processes, int lastInstant, int numProcess)
{
    for (int i = 0; i < numProcess; i++)
    {
        processes[i].start = 0;
        processes[i].finish = 0;
        processes[i].executed = 0;
        processes[i].remaining = processes[i].priority = processes[i].serviceOrPriority;
        processes[i].waiting = 0;
        processes[i].enteredQueue = 0;
        for (int j = 0; j <= lastInstant; j++)
            processes[i].progress[j] = ' ';
    }
}

void printPolicyName(int policy, int isTrace, int quantum)
{
    switch (policy)
    {
    case 1:
        printf("%s", isTrace == 1 ? "FCFS  " : "FCFS");
        break;
    case 2:
        char title[] = "RR-";
        char number[3];
        sprintf(number, "%d", quantum);
        strcat(title, number);
        printf(isTrace ? "%-6s" : "%s", title);
        break;
    case 3:
        printf("%s", isTrace == 1 ? "SPN   " : "SPN");
        break;
    case 4:
        printf("%s", isTrace == 1 ? "SRT   " : "SRT");
        break;
    case 5:
        printf("%s", isTrace == 1 ? "HRRN  " : "HRRN");
        break;
    case 6:
        printf("%s", isTrace == 1 ? "FB-1  " : "FB-1");
        break;
    case 7:
        printf("%s", isTrace == 1 ? "FB-2i " : "FB-2i");
        break;
    case 8:
        printf("%s", isTrace == 1 ? "Aging " : "Aging");
        break;
    default:
        printf("      ");
        break;
    }
}

void printStats(int policy, Process *processes, int isTrace, int numProcess, int quantum)
{
    int sumTurnaround = 0;
    float sumNormTurn = 0;
    printPolicyName(policy, isTrace, quantum);

    printf("\nProcess    |");
    for (int i = 0; i < numProcess; i++)
    {
        printf("  %c  |", processes[i].name);
    }
    printf("\n");

    printf("Arrival    |");
    for (int i = 0; i < numProcess; i++)
    {
        printf(" %2d  |", processes[i].arrival);
    }
    printf("\n");

    printf("Service    |");
    for (int i = 0; i < numProcess; i++)
    {
        printf(" %2d  |", processes[i].serviceOrPriority);
    }
    printf(" Mean|\n");

    printf("Finish     |");
    for (int i = 0; i < numProcess; i++)
    {
        printf(" %2d  |", processes[i].finish);
    }
    printf("-----|\n");

    printf("Turnaround |");
    for (int i = 0; i < numProcess; i++)
    {
        int turnaround = processes[i].finish - processes[i].arrival;
        printf(" %2d  |", turnaround);
        sumTurnaround += turnaround;
    }
    printf("%5.2f|\n", sumTurnaround * 1.0 / numProcess);

    printf("NormTurn   |");
    for (int i = 0; i < numProcess; i++)
    {
        float normTurn = (processes[i].finish - processes[i].arrival) * 1.0 / processes[i].serviceOrPriority;
        printf(" %.2f|", normTurn);
        sumNormTurn += normTurn;
    }
    printf("%5.2f|\n\n", sumNormTurn * 1.0 / numProcess);
}

void printSeparator(int num)
{
    for (int i = 0; i < num; i++)
    {
        printf("-");
    }
    printf("\n");
}

void printTrace(int policy, Process *processes, int numProcess, int lastInstant, int isTrace, int quantum)
{
    int totalLineSize = (lastInstant + 1) * 2;
    printPolicyName(policy, isTrace, quantum);

    for (int i = 0; i <= lastInstant; i++)
    {
        printf("%d ", i % 10);
    }
    printf("\n");

    printSeparator(totalLineSize + 6);

    for (int i = 0; i < numProcess; i++)
    {
        printf("%c     ", processes[i].name);
        for (int j = 0; j <= lastInstant; j++)
        {
            if (processes[i].progress[j] == '*')
            {
                printf("|%c", processes[i].progress[j]);
            }
            else if (j >= processes[i].arrival && j < processes[i].finish)
            {
                printf("|.");
            }
            else
            {
                printf("| ");
            }
        }
        printf("\n");
    }

    printSeparator(totalLineSize + 6);
    printf("\n");
}

void FCFS(Process *processes, int numProcess)
{
    int second = 0, completedProcess = 0;

    while (completedProcess < numProcess)
    {
        int minIndex = -1;
        int minArrival = 10000;

        for (int i = 0; i < numProcess; i++)
        {
            if (processes[i].arrival <= second && processes[i].executed == 0 && processes[i].arrival < minArrival)
            {
                minArrival = processes[i].arrival;
                minIndex = i;
            }
        }

        if (minIndex == -1)
        {
            second++;
            continue;
        }

        // Process the minIndex process
        processes[minIndex].start = second;
        second += processes[minIndex].serviceOrPriority;
        processes[minIndex].executed = 1;
        processes[minIndex].finish = second;

        completedProcess++;
        for (int i = processes[minIndex].start; i < processes[minIndex].finish; i++)
        {
            processes[minIndex].progress[i] = '*';
        }
    }
}

void roundRobin(Process *processes, int numProcess, int lastInstant, int quantum)
{
    int second = 0;
    Queue queue;
    initQueue(&queue);
    int addedToQueue[numProcess];

    for (int i = 0; i < numProcess; i++)
    {
        addedToQueue[i] = 0;
    }

    Process *unfinished = NULL;

    while (second < lastInstant)
    {
        for (int i = 0; i < numProcess; i++)
        {
            if (processes[i].arrival <= second && !processes[i].executed && !addedToQueue[i])
            {
                enqueue(&queue, &processes[i]);
                addedToQueue[i] = 1;
            }
        }
        if (unfinished != NULL)
        {
            enqueue(&queue, unfinished);
        }

        if (!isEmpty(&queue))
        {
            Process *process = dequeue(&queue);
            int execTime = process->remaining < quantum ? process->remaining : quantum;
            if (process->remaining == process->serviceOrPriority)
            {
                process->start = second;
            }
            for (int i = 0; i < execTime; i++)
            {
                process->progress[second + i] = '*';
            }
            process->remaining -= execTime;
            second += execTime;
            if (process->remaining == 0)
            {
                process->finish = second;
                process->executed = 1;
                unfinished = NULL;
            }
            else
            {
                unfinished = process;
            }
        }
        else
        {
            second++;
        }
    }
}

void SPN(Process *processes, int numProcess)
{
    int second = 0, completedProcess = 0;

    while (completedProcess < numProcess)
    {
        int minIndex = -1;
        int minService = 10000;

        for (int i = 0; i < numProcess; i++)
        {
            if (processes[i].arrival <= second && processes[i].executed == 0 && processes[i].serviceOrPriority < minService)
            {
                minService = processes[i].serviceOrPriority;
                minIndex = i;
            }
        }

        if (minIndex == -1)
        {
            second++;
            continue;
        }

        // Process the minIndex process
        processes[minIndex].start = second;
        second += processes[minIndex].serviceOrPriority;
        processes[minIndex].executed = 1;
        processes[minIndex].finish = second;

        completedProcess++;
        for (int i = processes[minIndex].start; i < processes[minIndex].finish; i++)
        {
            processes[minIndex].progress[i] = '*';
        }
    }
}

void SRT(Process *processes, int numProcess)
{
    int second = 0, completedProcess = 0;
    while (completedProcess < numProcess)
    {
        int minIndex = -1;
        int minRemaining = 10000;
        for (int i = 0; i < numProcess; i++)
        {
            if (processes[i].arrival <= second && processes[i].executed == 0 && processes[i].remaining < minRemaining)
            {
                minRemaining = processes[i].remaining;
                minIndex = i;
            }
        }
        if (minIndex == -1)
        {
            second++;
            continue;
        }

        if (processes[minIndex].serviceOrPriority == processes[minIndex].remaining)
            processes[minIndex].start = second;

        processes[minIndex].progress[second] = '*';
        processes[minIndex].remaining--;

        if (processes[minIndex].remaining == 0)
        {
            processes[minIndex].finish = second + 1;
            processes[minIndex].executed = 1;
            completedProcess++;
        }
        second++;
    }
}

void HRRN(Process *processes, int numProcess)
{
    int second = 0, completedProcess = 0;
    while (completedProcess < numProcess)
    {
        int maxIndex = -1;
        float maxRatio = -1;

        for (int i = 0; i < numProcess; i++)
        {
            if (processes[i].arrival <= second && processes[i].executed == 0)
            {
                int waiting = second - processes[i].arrival;
                float responseRatio = (waiting + processes[i].serviceOrPriority) * 1.0 / processes[i].serviceOrPriority;

                if (responseRatio > maxRatio)
                {
                    maxRatio = responseRatio;
                    maxIndex = i;
                }
            }
        }

        if (maxIndex == -1)
        {
            second++;
            continue;
        }

        // Process the maxIndex process
        processes[maxIndex].start = second;
        second += processes[maxIndex].serviceOrPriority;
        processes[maxIndex].executed = 1;
        processes[maxIndex].finish = second;

        completedProcess++;
        for (int i = processes[maxIndex].start; i < processes[maxIndex].finish; i++)
        {
            processes[maxIndex].progress[i] = '*';
        }
    }
}

void FB(Process *processes, int numProcess, int lastInstant, int constantQuantum)
{
    Queue queues[MAXQUEUES];
    for (int i = 0; i < MAXQUEUES; i++)
    {
        initQueue(&queues[i]);
    }

    int arrivedProcesses = 0;
    int second = 0;
    while (second < lastInstant)
    {
        for (int i = 0; i < numProcess; i++)
        {
            if (processes[i].arrival <= second && !processes[i].enteredQueue)
            {
                enqueue(&queues[0], &processes[i]);
                arrivedProcesses++;
                processes[i].enteredQueue = 1;
            }
        }

        int processed = 0;
        for (int i = 0; i < MAXQUEUES; i++)
        {
            if (!isEmpty(&queues[i]))
            {
                Process *p = dequeue(&queues[i]);
                processed = 1;
                if (p->remaining == p->serviceOrPriority)
                {
                    p->start = second;
                }

                int quantum = constantQuantum ? 1 : 1 << i;
                int execTime = p->remaining < quantum ? p->remaining : quantum;

                for (int j = 0; j < execTime; j++)
                {
                    p->remaining--;
                    p->progress[second++] = '*';
                }

                if (p->remaining == 0)
                {
                    p->finish = second;
                    arrivedProcesses--;
                    break;
                }

                int willArrive = 0;
                for (int j = 0; j < numProcess; j++)
                {
                    if (processes[j].arrival <= second && !processes[j].enteredQueue)
                    {
                        willArrive++;
                    }
                }
                if (arrivedProcesses + willArrive > 1)
                {
                    enqueue(&queues[i + 1], p);
                }
                else
                {
                    enqueue(&queues[i], p);
                }
                break;
            }
        }

        if (!processed)
        {
            second++;
        }
    }
}

void FB1(Process *processes, int numProcess, int lastInstant)
{
    FB(processes, numProcess, lastInstant, 1);
}

void FB2i(Process *processes, int numProcess, int lastInstant)
{
    FB(processes, numProcess, lastInstant, 0);
}

void Aging(Process *processes, int numProcess, int lastInstant, int quantum)
{
    int second = 0;
    while (second < lastInstant)
    {
        int maxIndex = -1;
        int maxPriority = -1;
        int maxWaiting = -1;
        for (int i = 0; i < numProcess; i++)
        {
            if (processes[i].arrival <= second && (processes[i].priority > maxPriority || (processes[i].priority == maxPriority && processes[i].waiting > maxWaiting)))
            {
                maxPriority = processes[i].priority;
                maxWaiting = processes[i].waiting;
                maxIndex = i;
            }
        }
        if (maxIndex == -1)
        {
            second++;
            continue;
        }
        processes[maxIndex].priority = processes[maxIndex].serviceOrPriority;
        processes[maxIndex].waiting = 0;
        if (processes[maxIndex].executed == 0)
        {
            processes[maxIndex].start = second;
            processes[maxIndex].executed = 1;
        }
        for (int i = 0; i < quantum; i++)
        {
            processes[maxIndex].progress[second] = '*';
            second++;
            if (second >= lastInstant)
                break;
        }

        for (int i = 0; i < numProcess; i++)
        {
            if (processes[i].arrival <= second && i != maxIndex)
            {
                processes[i].priority++;
                processes[i].waiting++;
            }
        }
    }
    for (int i = 0; i < numProcess; i++)
    {
        processes[i].finish = lastInstant;
    }
}
void callFunctions(int policies[MAXPOLICIES][2], int numPolicies, Process *processes, int numProcess, int lastInstant, int isTrace)
{
    for (int i = 0; i < numPolicies; i++)
    {
        initProgress(processes, lastInstant, numProcess);

        int policy = policies[i][0];
        int quantum = policies[i][1];

        switch (policy)
        {
        case 1:
            FCFS(processes, numProcess);
            break;
        case 2:
            roundRobin(processes, numProcess, lastInstant, quantum);
            break;
        case 3:
            SPN(processes, numProcess);
            break;
        case 4:
            SRT(processes, numProcess);
            break;
        case 5:
            HRRN(processes, numProcess);
            break;
        case 6:
            FB1(processes, numProcess, lastInstant);
            break;
        case 7:
            FB2i(processes, numProcess, lastInstant);
            break;
        case 8:
            Aging(processes, numProcess, lastInstant, quantum);
            break;
        default:
            break;
        }
        if (isTrace == 1)
            printTrace(policy, processes, numProcess, lastInstant, isTrace, quantum);
        else
            printStats(policy, processes, isTrace, numProcess, quantum);
    }
}

int main(int argc, char *argv[])
{
    FILE *input;
    char str[50];
    if (argc == 1)
    {
        input = stdin;
    }
    // else
    // {
    //     input = fopen(argv[2] /* or whatever */, "r");
    // }

    int line = 1;
    int isTrace;
    int lastInstant;
    int numProcess;

    char strPolicies[MAXPOLICIES][8];
    int policies[MAXPOLICIES][2];
    int numPolicies = 0;

    Process *processes;

    while (fgets(str, 50, input) != NULL)
    {
        if (line == 1)
            isTrace = strcmp(str, "trace\n") == 0 ? 1 : 0;
        if (line == 2)
        {
            char *token = strtok(str, ",");
            while (token != NULL)
            {
                strcpy(strPolicies[numPolicies], token);
                numPolicies++;
                token = strtok(NULL, ",");
            }

            for (int i = 0; i < numPolicies; i++)
            {
                char *t = strtok(strPolicies[i], "-");
                int policy = atoi(t);
                policies[i][0] = policy;
                if ((policy == 2 || policy == 8))
                {
                    t = strtok(NULL, "-");
                    policies[i][1] = atoi(t);
                }
            }
        }

        if (line == 3)
            lastInstant = atoi(str);
        if (line == 4)
        {
            numProcess = atoi(str);
            processes = malloc(numProcess * sizeof *processes);
        }
        if (line > 4)
        {
            char *token = strtok(str, ",");
            processes[line - 5].name = token[0];
            token = strtok(NULL, ",");
            processes[line - 5].arrival = atoi(token);
            token = strtok(NULL, ",");
            processes[line - 5].serviceOrPriority = atoi(token);
            processes[line - 5].remaining = atoi(token);
        }

        line++;
    }

    callFunctions(policies, numPolicies, processes, numProcess, lastInstant, isTrace);
    // printf("%d\n", isTrace);
    // printf("%d\n", policy);
    // printf("%d\n", quantum);
    // printf("%d\n", lastInstant);
    // printf("%d\n", numProcess);
    return 0;
}