#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MAX_PROCESS_NUM 100 // 최대 프로세스 개수

typedef enum { NEW, READY, RUNNING, WAITING, TERMINATED } ProcessState;

typedef struct {
    int pid;
    int cpu_burst_time;  //전체 burst time(고정)
    int arrival_time;
    int priority;

    int remaining_time; //남은시간(가변)

    // I/O 관련
    int io_count;
    int* io_request_times;    // 언제 I/O 발생할지 (시간)
    int* io_burst_times;      // I/O 하나당 소요 시간
    int current_io_index;     // 현재 몇 번째 I/O 중인지
    int io_remaining_time;    // 현재 I/O 남은 시간

    ProcessState state;  //프로세스 상태
} Process;

typedef struct { //실행 로그(간트차트나 평가용)
    int pid;
    int start_time;
    int end_time;
} Running_Log;

typedef struct Node{ //ready queue Node
    Process * process;
    struct Node * next;
}Node;

typedef struct{ //readuqueue 헤드
    Node * head;
}Queue;

//여긴 readyqueue 링크드리스트 동작함수
void init_queue(Queue* q); //ready queue 초기화
int is_empty(Queue* q);
Process* pop_front(Queue* q);
Process* peek_front(Queue* q); //첫번째 node 프로세스 확인하기
Node* get_node(Queue* q, int index);
int insert(Queue* q, int index, Process* p);
void append(Queue* q, Process* p);

void copy(Process *src, Process *dest, int n); //프로세스 리스트 복사
void Gantt_chart_display(Running_Log *log, char *name, int n); //간트차트 출력 
void Create_process(Process *p, int n); //프로세스 생성
void Evaluate(Process *p, int n, Running_Log *log, int index, char *name); //평가함수
void Schedule_FCFS(Process *p, Process *original, int n); //FCFS
void Schedule_SJF(Process *p, Process *original, int n);
void Schedule_Priority(Process *p, Process *original, int n);
void Print_process_info(Process *p, int n);
void Scheduling(Process *original, int select, int n);
void Schedule_RR(Process *p, Process *original, int n);
void Schedule_Preemptive_SJF(Process* p, Process* original, int n);


int main(void)
{
    Process original_process[MAX_PROCESS_NUM]={0}; //초기화

    int n=5;
    Create_process(original_process, n); // 프로세스 생성
 
    
    while(1)
    {
        int select;
        printf("\n\n<<select option>>\n"
               "1. FCFS\n2. Non-Preemptive SJF\n"
               "3. Non-Preemptive Priority\n4. RoundRobin\n"
               "5. Preemptive Priority\n6. Preemptive SJF\n"
               "7. Report (Evaluate All)\n8. Exit\n"
               );
        char input[100]; // 문자열 입력용 버퍼

        start:
            printf(">> ");
            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("Input error.\n");
                goto start;
            }

            // 숫자인지 확인 (입력값이 숫자가 아닐 경우 select에 안 들어감)
            if (sscanf(input, "%d", &select) != 1) {
                printf("Invalid input. Please enter a number between 1 and 8.\n");
                goto start;
            }

            if (select > 0 && select < 8) {
                Scheduling(original_process, select, n);
            }
            else if (select == 8) {
                printf("EXIT CPU_SCHEDULE_SIMULATER.\n");
                break;
            }
            else {
                printf("Invalid input. Please enter a number between 1 and 8.\n");
                goto start;
            }
    }
    return 0;

}
void init_queue(Queue* q) 
{
    q->head = NULL;  //링크드 리스트로 할 것이므로 head를 NULL로 초기화
}
int is_empty(Queue* q) 
{
    return q->head == NULL;
}
Process* pop_front(Queue* q) 
{
    if (q->head == NULL) return NULL;  // 빈 큐 처리

    Node* temp = q->head; //temp는 첫번째 node
    Process* p = temp->process; //첫 node의 프로세스

    q->head = temp->next;  // head를 두번째에 연결
    free(temp);            // 메모리 해제

    return p;
}
Process* peek_front(Queue* q) {  //첫번째 node의 프로세스 확인하기
    if (q->head == NULL) return NULL;
    return q->head->process;
}
Node* get_node(Queue* q, int index)
 {
    Node* prevNode = q->head; //현재 노드
    int i = 0;
    while (prevNode != NULL && i < index) { //노드가 null이거나 index값까지 도달할떄까지 루프(current가 null일경우는 마지막일 경우이다)
        prevNode = prevNode->next;
        i++;
    }
    return prevNode;  // index 위치의 노드 or NULL
}
int insert(Queue* q, int index, Process* p)
 {
    if (index < 0) return 0;

    Node* new_node = malloc(sizeof(Node));
    new_node->process = p;
    new_node->next = NULL;

    // 맨 앞 삽입
    if (index == 0) {
        new_node->next = q->head;
        q->head = new_node;
        return 1;
    }

    Node* prev = get_node(q, index - 1); //위조건을만족안할시 이전 노드뒤에 이어 붙여서 삽입
    if (prev == NULL) {//error 처리
        free(new_node);
        return 0;  
    }

    new_node->next = prev->next; //이전노드의 next를 새노드의 next에넣음
    prev->next = new_node; //이전노드의 next가 새노드를 가리키면 삽입완료
    return 1;
}
void append(Queue* q, Process* p) //맨뒤에 삽입
{  
    Node* new_node = malloc(sizeof(Node));
    new_node->process = p;
    new_node->next = NULL;

    if (q->head == NULL) {
        q->head = new_node;
        return;
    }

    Node* prevNode = q->head;
    while (prevNode->next != NULL) { //마지막 노드 탐색(prevNode의 next가 NULL인것이 마지막 노드이다)
        prevNode = prevNode->next;
    }

    prevNode->next = new_node;
}

void Scheduling(Process *original, int select, int n)
{
    system("clear");

    Process copied_process[MAX_PROCESS_NUM]={0};
    switch(select)
    {
        case 1:
            Print_process_info(original,n);
            copy(original, copied_process, n);
            Schedule_FCFS(copied_process,original,n);
            break;
        case 2:
            Print_process_info(original,n);
            copy(original, copied_process, n);
            Schedule_SJF(copied_process,original,n);
            break;
        case 3:
            Print_process_info(original,n);
            copy(original, copied_process, n);
            Schedule_Priority(copied_process,original,n);
            break;
        case 4:
            Print_process_info(original,n);
            copy(original, copied_process, n);
            Schedule_RR(copied_process,original,n);
            break;
        case 6:
            Print_process_info(original,n);
            copy(original, copied_process, n);
            Schedule_Preemptive_SJF(copied_process,original,n);
    }
    printf("Press Any Key....");
    getchar();

}

void Print_process_info(Process *p, int n) 
{
    printf("\n\033[1;36m[ Process Information ]\033[0m\n");
    printf("PID  Arrival  Burst  Priority  IO_Count  IO_Request_Times  IO_Burst_Times\n");
    printf("-----------------------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        printf("P%-3d  %-7d  %-5d  %-8d  %-8d  ", 
            p[i].pid,
            p[i].arrival_time,
            p[i].cpu_burst_time,
            p[i].priority,
            p[i].io_count
        );

        // IO Request Times
        for (int j = 0; j < p[i].io_count; j++)
            printf("%2d ", p[i].io_request_times[j]);
        if(p[i].io_count==2)
            printf("          ");
        else
            printf("       ");
        // IO Burst Times
        for (int j = 0; j < p[i].io_count; j++)
            printf("%2d ", p[i].io_burst_times[j]);

        printf("\n");
    }

    printf("-----------------------------------------------------------------------------\n\n");
}

void copy(Process* src, Process* dest, int n) 
{
    for (int i = 0; i < n; i++) {
        dest[i] = src[i];  // 얕은 복사

        // 동적 배열을 위한 깊은 복사
        dest[i].io_request_times = malloc(sizeof(int) * src[i].io_count);
        dest[i].io_burst_times = malloc(sizeof(int) * src[i].io_count);
        
        for (int j = 0; j < src[i].io_count; j++) {
            dest[i].io_request_times[j] = src[i].io_request_times[j];
            dest[i].io_burst_times[j] = src[i].io_burst_times[j];
        }
    }
}

void Create_process(Process *p, int n)
{   
    srand(time(NULL)); //시드값 지정

    int * priority_pool = malloc(sizeof(int)*n); //중복없는 우선순위를 위한 배열 동적할당

    for(int i=0; i<n; i++) priority_pool[i]= i; //순서대로 우선순위 풀을 초기화(0,1,2...n-1)
        
    for(int i = n-1; i>0; i--) // 카드섞기(shuffele)방식 - 이렇게 하면 중복없이 우선순위 지정가능(참조)
    {
        int j=rand()%(i+1);
        int temp = priority_pool[i];
        priority_pool[i] = priority_pool[j];
        priority_pool[j] = temp;
    }

    for(int i=0; i<n; i++) // 프로세스 정보 set
    {
        p[i].pid = i+1;
        p[i].cpu_burst_time = rand()%13 +3; //(3~15)
        p[i].arrival_time = rand()%16; // (0~15)
        p[i].priority = priority_pool[i]+1; //(1~n)
        p[i].remaining_time = p[i].cpu_burst_time;
        p[i].state = NEW;

        if(p[i].cpu_burst_time == 3)   
            p[i].io_count =2; //bursttime 이 3이면 최대 2번까지 i/o가 들어갈수있다.
            //p[i].io_count = rand()%2 + 1; //무조건 2번이상(멀티번) 일어나야하면 감점대상(1~2)
        else 
            p[i].io_count = rand()%2 +2; //그외는 (2~3) i/o

        p[i].io_request_times = malloc(sizeof(int) * p[i].io_count);
        p[i].io_burst_times = malloc(sizeof(int) * p[i].io_count);

        int *IO_used = calloc(p[i].cpu_burst_time, sizeof(int)); ///io 발생시점을 체크할 임시변수 1로 발생확인(calloc 연속된 메모리공간을 0으로 초기화해서 반환)

        for (int j = 0; j < p[i].io_count; ) //i/o 발생 시점 정하기
        { 
            int t = rand() % (p[i].cpu_burst_time - 1) + 1; // (1 ~ burst-1)

            if (!IO_used[t]) //i/o 시점 중복 체크(0이면 io가 없다는 뜻이므로 진입)
            { 
                IO_used[t] = 1; //i/o 플래그 셋
                p[i].io_request_times[j] = t;
                p[i].io_burst_times[j] = rand() % 4 + 2; // (2~5)
                j++;
            }
             
        }

        for (int j = 0; j < p[i].io_count - 1; j++) //버블정렬(io request_time 오름차순 정렬)
        {
            for (int k = 0; k < p[i].io_count - 1 - j; k++) 
            {
                if (p[i].io_request_times[k] > p[i].io_request_times[k + 1]) 
                {
                    int temp;

                    temp = p[i].io_request_times[k];
                    p[i].io_request_times[k] = p[i].io_request_times[k + 1];
                    p[i].io_request_times[k + 1] = temp;

                    temp = p[i].io_burst_times[k];
                    p[i].io_burst_times[k] = p[i].io_burst_times[k + 1];
                    p[i].io_burst_times[k + 1] = temp;
                }
            }
        }

        p[i].io_remaining_time = 0;
        p[i].current_io_index = 0;
    }

    p[rand()%n].arrival_time=0; //첫번째  idle 방지
     
    free(priority_pool);
}

void Gantt_chart_display(Running_Log *log, char *name, int n)
{
    const int MAX_LINE_WIDTH = 140;            // 줄바꿈 임계치 
    int *interval = malloc(sizeof(int) * n);  // 각 블록 너비 저장
    int *end_times = malloc(sizeof(int) * n); // 종료시간 저장
    int width = 0;  //간격

    printf("\n\033[1;33mGantt Chart - %s\033[0m\n", name);

    int i = 0;
    while (i < n) {
        int line_width = 0;  //누적 간격
        int line_start = i;  //타임 레이블 시작점

        printf("\n");
        //  블록 한 줄 출력
        printf("|");
        while (i < n) {
            int burst = log[i].end_time - log[i].start_time; //burst 계산
            if (log[i].pid == 0)
                width = printf(" \033[1;31mIDLE(%d)\033[0m |", burst) - 11;
                //width = printf(" IDLE(%d) |", burst);
            else
                width = printf(" P%d(%d) |", log[i].pid, burst);

            interval[i] = width;
            end_times[i] = log[i].end_time;
            line_width += width;

            i++;
            if (line_width >= MAX_LINE_WIDTH)  break; // 줄 너비 초과 시 개행
        }
        printf("\n");

        // 시간 라벨 출력
        printf("%*d", 1, log[line_start].start_time); // 첫 시작점
        for (int j = line_start; j < i; j++) {
            printf("%*d", interval[j], end_times[j]);
        }
        printf("\n");
    }

    free(interval);
    free(end_times);
}


void Evaluate(Process *p, int n, Running_Log *log, int index, char *name)
{
    int *waiting_times=malloc(sizeof(int)*(n+1));  //프로세스별 waiting time
    int *turnaround_times=malloc(sizeof(int)*(n+1));  //프로세스별 turnaround time
    int total_waiting = 0; // 전체 waiting time
    int total_turnaround = 0; //전체 turnaround time

    // 각 프로세스별 계산
    for (int i = 0; i < n; i++) {
        int pid = p[i].pid;                  //현 프로세스의 pid, arrival, burst time 값 로드
        int arrival = p[i].arrival_time;
        int burst = p[i].cpu_burst_time;

        int complete_time = -1; //프로세스 완료시간

        for (int j = 0; j < index; j++) { // 현프로세스의  최종 완료시간 구하기
            if (log[j].pid == pid) {
                complete_time = log[j].end_time;
            }
        }

        int turnaround = complete_time - arrival; //turnaround time = 완료시간-도착시간
        int waiting = turnaround - burst;  //waiting time = turnaround time - 실행시간

        waiting_times[pid] = waiting; //프로세스별 wainting time 저장
        turnaround_times[pid] = turnaround; //프로세스별 turnaround time 저장

        total_waiting += waiting; //전체 waiting time
        total_turnaround += turnaround; // 전체 turnaround time
    }

    // 표 출력qn
    printf("\nEVALUATION \n");
    printf("          |");
    for (int i = 1; i <= n; i++)
        printf(" P%-2d", i);
    printf("\n----------------------------------------------------\n");

    printf("Waiting   |");
    for (int i = 1; i <= n; i++)
        printf(" %3d", waiting_times[i]);
    printf("\n");

    printf("Turnaround|");
    for (int i = 1; i <= n; i++)
        printf(" %3d", turnaround_times[i]);
    printf("\n");

    // 평균 출력
    printf("\nAverage Waiting Time   : %.2f\n", (float)total_waiting / n);
    printf("Average Turnaround Time: %.2f\n", (float)total_turnaround / n);

    free(waiting_times);
    free(turnaround_times);
}

void Schedule_FCFS(Process* p, Process* original, int n) 
{
    Queue ready, waiting; //ready 큐와 waiting 큐 생성및 초기화
    init_queue(&ready);
    init_queue(&waiting);

    Running_Log log[2 * MAX_PROCESS_NUM]; //간트차트와 평가용 log
    int log_index = 0;
    int current_time = 0;
    int terminated_count = 0; //종료된 프로세스 개수
    Process* running = NULL; //현재 실행중인 프로세스
    int start_time = -1;  //log 기록용
    int idle_start = -1;  //log 기록용

    while (terminated_count < n)  //모든 프로세스가 끝날떄까지
    {
        // NEW -> READY: 현재 시간에 도착한 프로세스를 ready 큐에 삽입
        for (int i = 0; i < n; i++) 
        { 
            if (p[i].arrival_time == current_time) //도착한 프로세스 삽입
            {
                p[i].state = READY;
                append(&ready, &p[i]);
            }
        }

        //IO 처리
        if (!is_empty(&waiting)) //wainting큐에 프로세스가 있으면 진행
        {
            Process* io_proc = peek_front(&waiting);  //waiting큐 첫번째 가져옴
            

            // WAITING -> READY: I/O 완료 시 ready로 이동
            if (io_proc->io_remaining_time == 0)  // i/o 가끝났을떄
            {
                pop_front(&waiting);
                io_proc->current_io_index++; //다음 i/o가 되도록index 증가
                io_proc->state = READY;
                append(&ready, io_proc);
            }
                  if(io_proc->io_remaining_time > 0) io_proc->io_remaining_time--; // i/o burst 감소
        }

        if (running != NULL) //현재 프로세스가 running일때
        {
            int executed_time = running->cpu_burst_time - running->remaining_time;
            // I/O 요청 도달(RUNNING -> WAITING)
            if (running->current_io_index < running->io_count &&
                executed_time == running->io_request_times[running->current_io_index])
            {
                running->io_remaining_time = running->io_burst_times[running->current_io_index]; // io remain set 하고  컨트롤
                running->state = WAITING;
                append(&waiting, running); //프로세스 waiting 큐에 삽입

                log[log_index++] = (Running_Log){running->pid, start_time, current_time}; // 로그기록
                running = NULL;  //running 비움
                start_time = -1;  //다음 번 프로세스를 위해 초기화 시켜놓ㅇㅁ
            }

            else if (running->remaining_time == 0)  //프로세스가 종료되었을때 (RUNNING -> TERMINATED)
            {
                running->state = TERMINATED;
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time  };
                running = NULL;
                start_time = -1;
                terminated_count++;
                
            }
        }
        //  READY -> RUNNING
        if (running == NULL && !is_empty(&ready)) //실행중인 프로세스가없고, ready큐가 empty가 아닐떄
        {
            if (idle_start != -1) //idle 로그 기록 (-1이 아니면 이전이 idle)
            {
                log[log_index++] = (Running_Log){ 0, idle_start, current_time };  //idle은 pid=0으로 처리
                idle_start = -1;  //idle이 끝났으므로 초기화
            }

            running = pop_front(&ready); //빼서 Runing으로 돌림
            running->state = RUNNING; 
            start_time=current_time;
        }

        //  IDLE 감지
        if (running == NULL && is_empty(&ready)) //runinning도 없고 프로세스도 없을떄
        {
            if (idle_start == -1) 
            {
                idle_start = current_time; //idle 시작 시간 기록
            }
        }

        current_time++; //1 tick만큼 증가
        if(running != NULL)
            running->remaining_time--; // 실행 시간만큼 남은시간 감소
    }

    Gantt_chart_display(log, "FCFS", log_index);
    Evaluate(original, n, log, log_index, "FCFS");
}

void Schedule_SJF(Process* p, Process* original, int n) 
{
    Queue ready, waiting; // ready 큐와 waiting 큐 생성 및 초기화
    init_queue(&ready);
    init_queue(&waiting);

    Running_Log log[2 * MAX_PROCESS_NUM]; // 간트차트와 평가용 log
    int log_index = 0;
    int current_time = 0;
    int terminated_count = 0; // 종료된 프로세스 개수
    Process* running = NULL; // 현재 실행중인 프로세스
    int start_time = -1;  //log 기록용
    int idle_start = -1;  //log 기록용

    while (terminated_count < n)  //모든 프로세스가 끝날떄까지
    {
        //  NEW  ->  READY
        for (int i = 0; i < n; i++) // ready 큐에 삽입
        { 
            if (p[i].arrival_time == current_time) //도착한 프로세스 삽입
            {
                p[i].state = READY;

                // remaining_time 기준 정렬 삽입
                int idx = 0;
                Node* cur = ready.head;
                while (cur) {
                    if (p[i].remaining_time < cur->process->remaining_time) break;
                    idx++;
                    cur = cur->next;
                }
                insert(&ready, idx, &p[i]);
            }
        }

        //IO 처리
        if (!is_empty(&waiting)) //wainting큐에 프로세스가 있으면 진행
        {
            Process* io_proc = peek_front(&waiting);  //waiting큐 첫번째 가져옴

            //WAITING -> READY
            if (io_proc->io_remaining_time == 0)  // i/o 가끝났을떄
            {
                pop_front(&waiting);
                io_proc->current_io_index++;
                io_proc->state = READY;

                // remaining_time 기준 정렬 삽입
                int idx = 0;
                Node* cur = ready.head;
                while (cur) {
                    if (io_proc->remaining_time < cur->process->remaining_time) break;
                    idx++;
                    cur = cur->next;
                }
                insert(&ready, idx, io_proc);
            }
            if(io_proc->io_remaining_time > 0) io_proc->io_remaining_time--; // i/o burst 감소
        }

        if (running != NULL) //현재 프로세스가 running일때
        {
            int executed_time = running->cpu_burst_time - running->remaining_time;

            // I/O 요청 도달(RUNNING -> WAITING)
            if (running->current_io_index < running->io_count &&
                executed_time == running->io_request_times[running->current_io_index])
            {
                running->io_remaining_time = running->io_burst_times[running->current_io_index];
                running->state = WAITING;
                append(&waiting, running);

                log[log_index++] = (Running_Log){ running->pid, start_time, current_time };
                running = NULL;
                start_time = -1;
            }
            //RUNNING -> TERMINATED
            else if (running->remaining_time == 0)  //프로세스가 종료되었을때
            {
                running->state = TERMINATED;
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time };
                running = NULL;
                start_time = -1;
                terminated_count++;
            }
        }

        //  READY -> RUNNING
        if (running == NULL && !is_empty(&ready)) //실행중인 프로세스가없고, ready큐가 empty가 아닐떄
        {
            if (idle_start != -1)
            {
                log[log_index++] = (Running_Log){ 0, idle_start, current_time };  //idle은 pid=0으로 처리
                idle_start = -1;
            }

            running = pop_front(&ready);
            running->state = RUNNING;
            start_time = current_time;
        }

        //  IDLE 감지
        if (running == NULL && is_empty(&ready))
        {
            if (idle_start == -1)
            {
                idle_start = current_time;
            }
        }

        current_time++; //1 tick만큼 증가
        if (running != NULL)
            running->remaining_time--; // 실행 시간만큼 남은시간 감소
    }

    Gantt_chart_display(log, "SJF", log_index);
    Evaluate(original, n, log, log_index, "SJF");
}

void Schedule_Priority(Process* p, Process* original, int n) 
{
    Queue ready, waiting; // ready 큐와 waiting 큐 생성 및 초기화
    init_queue(&ready);
    init_queue(&waiting);

    Running_Log log[2 * MAX_PROCESS_NUM]; // 간트차트와 평가용 log
    int log_index = 0;
    int current_time = 0;
    int terminated_count = 0; // 종료된 프로세스 개수
    Process* running = NULL; // 현재 실행중인 프로세스
    int start_time = -1;  //log 기록용
    int idle_start = -1;  //log 기록용

    while (terminated_count < n) // 모든 프로세스가 끝날 때까지
    {
        //  NEW  ->  READY
        for (int i = 0; i < n; i++) // 도착한 프로세스 ready 큐에 삽입(NEW -> READY)
        {
            if (p[i].arrival_time == current_time)
            {
                p[i].state = READY;
                // priority 기준 정렬 삽입
                int idx = 0;
                Node* cur = ready.head; // 첫번째 노드
                while (cur) // priority 기준으로 탐색
                {
                    if (p[i].priority < cur->process->priority) break; // cur보다 높을 때 break
                    idx++;
                    cur = cur->next; // 다음 노드
                }
                insert(&ready, idx, &p[i]);
            }
        }

        //IO 처리
        if (!is_empty(&waiting)) // waiting 큐 처리
        {
            Process* io_proc = peek_front(&waiting); //waiting 큐의 첫번쨰 가져옴

            //WAITING -> READY
            if (io_proc->io_remaining_time == 0)  //io가 끝났을떄
            {
                pop_front(&waiting);
                io_proc->current_io_index++;  //다음 io로 index증가
                io_proc->state = READY;

                int idx = 0;
                Node* cur = ready.head; //첫번쨰 노드
                while (cur)//노드 탐색
                {
                    if (io_proc->priority < cur->process->priority) break; //curNode보다 높을때 break, curnode 앞에 삽입
                    idx++;
                    cur = cur->next; //다음노드
                }
                insert(&ready, idx, io_proc);
            }

            if(io_proc->io_remaining_time > 0) io_proc->io_remaining_time--;
        }

        if (running != NULL) //현재 프로세스가 running일때
        {
            int executed_time = running->cpu_burst_time - running->remaining_time;

            // IO 요청 도달(RUNNING -> WAITING)
            if (running->current_io_index < running->io_count &&       //유효한 io 인덱스인지
                executed_time == running->io_request_times[running->current_io_index]) // 실행시간 = I/O 요청시간
            {
                running->io_remaining_time = running->io_burst_times[running->current_io_index];//i/o remain time set 해서 컨트롤
                running->state = WAITING;
                append(&waiting, running); //waiting에 삽입

                log[log_index++] = (Running_Log){ running->pid, start_time, current_time }; //기록
                running = NULL; //running 초기화
                start_time = -1;
            }

            //RUNNING -> TERMINATED
            else if (running->remaining_time == 0) //프로세스 종료 시
            {
                running->state = TERMINATED;
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time }; //기록 
                running = NULL;
                start_time = -1;
                terminated_count++;
            }
        }

        //  READY -> RUNNING
        if (running == NULL && !is_empty(&ready)) //실행중인 프로세스가없고, ready큐가 empty가 아닐떄
        {
            if (idle_start != -1)
            {
                log[log_index++] = (Running_Log){ 0, idle_start, current_time };  //idle은 pid=0으로 처리
                idle_start = -1;
            }

            running = pop_front(&ready); //빼서 Runing으로 돌림
            running->state = RUNNING; 
            start_time=current_time;
        }

        //  IDLE 감지
        if (running == NULL && is_empty(&ready)) //runinning도 없고 프로세스도 없을떄
        {
            if (idle_start == -1) 
            {
                idle_start = current_time; //idle 시작 시간 기록
            }
        }

        current_time++; //1 tick만큼 증가
        if (running != NULL)
            running->remaining_time--; // 실행 시간만큼 남은시간 감소
    }

    Gantt_chart_display(log, "Priority", log_index);
    Evaluate(original, n, log, log_index, "Priority");
}

void Schedule_RR(Process* p, Process* original, int n) 
{
    Queue ready, waiting; // ready 큐와 waiting 큐 생성 및 초기화
    init_queue(&ready);
    init_queue(&waiting);

    Running_Log log[2 * MAX_PROCESS_NUM]; // 간트차트와 평가용 log
    int log_index = 0;
    int current_time = 0;
    int terminated_count = 0; // 종료된 프로세스 개수
    Process* running = NULL; // 현재 실행중인 프로세스
    int start_time = -1;  //log 기록용
    int idle_start = -1;  //log 기록용
    int quantum_counter = 0; //RR용 퀀텀 카운터

    const int TIME_QUANTUM = 4; // 타임 퀀텀 설정

    while (terminated_count < n)  //모든 프로세스가 끝날떄까지
    {
        //  NEW  ->  READY
        for (int i = 0; i < n; i++) // ready 큐에 삽입(NEW -> READY)
        {
            if (p[i].arrival_time == current_time) //도착한 프로세스 삽입
            {
                p[i].state = READY;
                append(&ready, &p[i]);
            }
        }

        //IO 처리
        if (!is_empty(&waiting)) //wainting큐에 프로세스가 있으면 진행
        {
            Process* io_proc = peek_front(&waiting);  //waiting큐 첫번째 가져옴

            //WAITING -> READY
            if (io_proc->io_remaining_time == 0)  // i/o 가끝났을떄
            {
                pop_front(&waiting);
                io_proc->current_io_index++; //다음 i/o가 되도록index 증가
                io_proc->state = READY;
                append(&ready, io_proc);
            }

            if(io_proc->io_remaining_time>0) io_proc->io_remaining_time--; // I/O 감소
        }

        if (running != NULL) //현재 프로세스가 running일때
        {
            int executed_time = running->cpu_burst_time - running->remaining_time;

            // I/O 요청 도달(RUNNING -> WAITING)
            if (running->current_io_index < running->io_count &&
                executed_time == running->io_request_times[running->current_io_index])
            {
                running->io_remaining_time = running->io_burst_times[running->current_io_index]; // io remain set 하고  컨트롤
                running->state = WAITING;
                append(&waiting, running); //프로세스 waiting 큐에 삽입

                log[log_index++] = (Running_Log){ running->pid, start_time, current_time }; //로그기록
                running = NULL;  //running 비움
                start_time = -1;  //다음 번 프로세스를 위해 초기화
                quantum_counter = 0;  //퀀텀 리셋
            }

            //RUNNING -> TERMINATED
            else if (running->remaining_time == 0)  //프로세스가 종료되었을때
            {
                running->state = TERMINATED;
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time };
                running = NULL;
                start_time = -1;
                terminated_count++;
                quantum_counter = 0; //퀀텀 리셋
            }

            //RUNNING -> READY (퀀텀 만료)
            else if (quantum_counter == TIME_QUANTUM)
            {
                running->state = READY;
                append(&ready, running);
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time };
                running = NULL;
                start_time = -1;
                quantum_counter = 0;
            }
        }

        //  READY -> RUNNING
        if (running == NULL && !is_empty(&ready)) //실행중인 프로세스가없고, ready 큐가 비어있지 않을 때
        {
            if (idle_start != -1) //idle 로그 기록 (-1이 아니면 이전이 idle)
            {
                log[log_index++] = (Running_Log){ 0, idle_start, current_time }; //idle은 pid=0으로 처리
                idle_start = -1; //idle이 끝났으므로 초기화
            }

            running = pop_front(&ready); //빼서 Runing으로 돌림
            running->state = RUNNING;
            start_time = current_time; //시작시간 기록
            quantum_counter = 0; //퀀텀 초기화
        }

        //  IDLE 감지
        if (running == NULL && is_empty(&ready)) //runinning도 없고 프로세스도 없을떄
        {
            if (idle_start == -1)
            {
                idle_start = current_time; //idle 시작 시간 기록
            }
        }

        current_time++; //1 tick만큼 증가
        if (running != NULL)
        {
            running->remaining_time--; // 실행 시간만큼 남은시간 감소
            quantum_counter++; //퀀텀 증가
        }
    }

    Gantt_chart_display(log, "RR", log_index);
    Evaluate(original, n, log, log_index, "RR");
}

void Schedule_Preemptive_SJF(Process* p, Process* original, int n) {
    Queue ready, waiting; // ready 큐와 waiting 큐 생성 및 초기화
    init_queue(&ready);
    init_queue(&waiting);

    Running_Log log[2 * MAX_PROCESS_NUM]; // 간트차트와 평가용 log
    int log_index = 0;
    int current_time = 0; // 현재 시간 tick
    int terminated_count = 0; // 종료된 프로세스 개수
    Process* running = NULL; // 현재 실행중인 프로세스
    int start_time = -1;  // log 기록용 (시작시간)
    int idle_start = -1;  // log 기록용 (idle 시작시간)

    while (terminated_count < n) //모든 프로세스가 끝날때까지
    {
        //  NEW  ->  READY
        for (int i = 0; i < n; i++) // 현재 시간에 도착한 프로세스 ready 큐에 삽입
        {
            if (p[i].arrival_time == current_time)
            {
                p[i].state = READY;

                // remaining_time 기준 정렬 삽입
                int idx = 0;
                Node* cur = ready.head;
                while (cur) // 탐색
                {
                    if (p[i].remaining_time < cur->process->remaining_time) break;
                    idx++;
                    cur = cur->next;
                }
                insert(&ready, idx, &p[i]);
            }
        }

        // IO 처리
        if (!is_empty(&waiting)) // waiting 큐에 프로세스가 있으면
        {
            Process* io_proc = peek_front(&waiting); //waiting 큐 첫번째

            // WAITING -> READY
            if (io_proc->io_remaining_time == 0) // I/O 완료 시
            {
                pop_front(&waiting);
                io_proc->current_io_index++;
                io_proc->state = READY;

                // remaining_time 기준 정렬 삽입
                int idx = 0;
                Node* cur = ready.head;
                while (cur)
                {
                    if (io_proc->remaining_time < cur->process->remaining_time) break;
                    idx++;
                    cur = cur->next;
                }
                insert(&ready, idx, io_proc);
            }

            if (io_proc->io_remaining_time > 0) io_proc->io_remaining_time--; // I/O 남은 시간 감소
        }

        //  선점 조건 확인
        if (running != NULL && !is_empty(&ready)) // 실행 중일 때 ready에 더 짧은 프로세스가 있으면
        {
            Process* front = peek_front(&ready);
            if (front->remaining_time < running->remaining_time) // 실행시간 비교(SJF)
            {
                running->state = READY;

                // 다시 ready 큐에 삽입(RUNNING -> READY)리스케줄링
                int idx = 0;
                Node* cur = ready.head;
                while (cur)  //탐색
                {
                    if (running->remaining_time < cur->process->remaining_time) break;
                    idx++;
                    cur = cur->next;
                }
                insert(&ready, idx, running);

                log[log_index++] = (Running_Log){ running->pid, start_time, current_time }; // 실행 종료 로그 기록
                running = NULL; // running 프로세스 종료
                start_time = -1;
            }
        }

        //  READY -> RUNNING
        if (running == NULL && !is_empty(&ready)) // 실행 중인 프로세스가 없고 ready에 있을 때
        {
            if (idle_start != -1) // 이전이 idle이면 idle 로그 기록
            {
                log[log_index++] = (Running_Log){ 0, idle_start, current_time };  //idle은 pid=0으로 처리
                idle_start = -1;
            }

            running = pop_front(&ready); // ready에서 꺼내 실행
            running->state = RUNNING;
            start_time = current_time; // 실행 시작 시간 기록
        }

        if (running != NULL) // 현재 프로세스 실행 중일 때
        {
            int executed_time = running->cpu_burst_time - running->remaining_time;

            // RUNNING -> WAITING: I/O 요청 도달 시
            if (running->current_io_index < running->io_count &&
                executed_time == running->io_request_times[running->current_io_index])
            {
                running->io_remaining_time = running->io_burst_times[running->current_io_index];
                running->state = WAITING;
                append(&waiting, running);

                log[log_index++] = (Running_Log){ running->pid, start_time, current_time }; // 로그 기록
                running = NULL;
                start_time = -1;
            }

            // RUNNING -> TERMINATED
            else if (running->remaining_time == 0) // 실행 완료 시
            {
                running->state = TERMINATED;
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time }; // 로그 기록
                running = NULL;
                start_time = -1;
                terminated_count++;
            }
        }

        //  IDLE 감지
        if (running == NULL && is_empty(&ready)) // running도 없고 ready도 없으면 idle
        {
            if (idle_start == -1)
            {
                idle_start = current_time; // idle 시작 시간 기록
            }
        }

        current_time++; // 1 tick 증가
        if (running != NULL)
            running->remaining_time--; // 실행 시간 감소
    }

    Gantt_chart_display(log, "Preemptive SJF", log_index); // 간트차트 출력
    Evaluate(original, n, log, log_index, "Preemptive SJF"); // 평가 출력
}
