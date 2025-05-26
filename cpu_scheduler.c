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


int main(void)
{
    Process original_process[MAX_PROCESS_NUM]={0}; //초기화
    Process copied_process[MAX_PROCESS_NUM]={0};
    int n=10;

    Create_process(original_process, n); // 프로세스 생성
    copy(original_process, copied_process, n);
    Schedule_FCFS(copied_process,original_process,n); 
    copy(original_process, copied_process, 10);
    Schedule_SJF(copied_process,original_process,10);
    /*copy(original_process,copied_process,10);
    Schedule_Priority(copied_process,original_process,10);*/
    return 0;

}
void init_queue(Queue* q) {
    q->head = NULL;  //링크드 리스트로 할 것이므로 head를 NULL로 초기화
}
int is_empty(Queue* q) {
    return q->head == NULL;
}
Process* pop_front(Queue* q) {
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
Node* get_node(Queue* q, int index) {
    Node* prevNode = q->head; //현재 노드
    int i = 0;
    while (prevNode != NULL && i < index) { //노드가 null이거나 index값까지 도달할떄까지 루프(current가 null일경우는 마지막일 경우이다)
        prevNode = prevNode->next;
        i++;
    }
    return prevNode;  // index 위치의 노드 or NULL
}
int insert(Queue* q, int index, Process* p) {
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
void append(Queue* q, Process* p) {  //맨뒤에 삽입
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

void copy(Process* src, Process* dest, int n) {
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
            p[i].io_count = rand()%2 + 2; //그외는 (2~3) i/o

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
        free(IO_used);

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

    printf("\nGantt Chart - %s", name);

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
                width = printf(" IDLE(%d) |", burst);
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


void Schedule_FCFS(Process* p, Process* original, int n) {
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
        for (int i = 0; i < n; i++) // ready 큐에 삽입
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
            io_proc->io_remaining_time--; // i/o burst 감소

            if (io_proc->io_remaining_time == 0)  // i/o 가끝났을떄
            {
                pop_front(&waiting);
                io_proc->current_io_index++; //다음 i/o가 되도록index 증가
                io_proc->state = READY;
                append(&ready, io_proc);
            }
        }

        
        if (running != NULL) //현재 프로세스가 running일때
        {
            if (start_time == -1) start_time = current_time;  //시작시간 기록

            running->remaining_time--; // 실행 시간만큼 남은시간 감소

            // I/O 요청 도달(running -> waiting)
            if (running->current_io_index < running->io_count &&         //실행할 i/o인덱스가 전체 i/o보다 작아야함
                running->cpu_burst_time - running->remaining_time ==     //cpu_burst - 남은시간 = 실행한 시간이 나오, 이게 i/o request 시간이 같을시 i/o 시작
                running->io_request_times[running->current_io_index]) 
            {
                running->io_remaining_time = running->io_burst_times[running->current_io_index]; // io remain set 하고  컨트롤
                running->state = WAITING;
                append(&waiting, running); //프로세스 waiting 큐에 삽입
              
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time }; //로그기록
                running = NULL;  //running 비움
                start_time = -1;  //다음 번 프로세스를 위해 초기화 시켜놓ㅇㅁ
            }
            else if (running->remaining_time == 0)  //프로세스가 종료되었을때 (running->terminated)
            {
                running->state = TERMINATED;
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time + 1 };
                running = NULL;
                start_time = -1;
                terminated_count++;
            }
        }

        //  ReadyQueue -> Running
        if (running == NULL && !is_empty(&ready)) //현재실행중인 프로세스 없고, ready 큐가 비어있지않을떄
          {
            if (idle_start != -1) //idle 로그 기록 (-1이 아니면 이전이 idle)
            {
                log[log_index++] = (Running_Log){ 0, idle_start, current_time };  //idle은 pid=0으로 처리
                idle_start = -1;  //idle이 끝났으므로 초기화
            }

            running = pop_front(&ready); //빼서 Runing으로 돌림
            running->state = RUNNING; 
        }

        // 5. IDLE 감지
        if (running == NULL && is_empty(&ready)) //runinning도 없고 프로세스도 없을떄
        {
            if (idle_start == -1) 
            {
                idle_start = current_time; //idle 시작 시간 기록
            }
        }

        current_time++; //1 tick만큼 증가
    }

    Gantt_chart_display(log, "FCFS", log_index);
    Evaluate(original, n, log, log_index, "FCFS");
}


void Schedule_SJF(Process* p, Process* original, int n) {
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
        for (int i = 0; i < n; i++) // 도착한 프로세스 ready 큐에 삽입
        {
            if (p[i].arrival_time == current_time)
            {
                p[i].state = READY;
                // remaining_time 기준 정렬 삽입
                int idx = 0;
                Node* cur = ready.head; //첫번쨰 노드
                while (cur) //남은 실행시간기준으로 탐색
                {
                    if (p[i].remaining_time < cur->process->remaining_time) break; //cur Node보다 짧으면 break하고 cur node 앞에 삽입
                    idx++;
                    cur = cur->next;  //다음 노드로 이동
                }
                insert(&ready, idx, &p[i]);
            }
        }
        //IO처리
        if (!is_empty(&waiting)) // waiting 큐 처리
        {
            Process* io_proc = peek_front(&waiting); //waiting 큐의 첫번쨰 가져옴
            io_proc->io_remaining_time--;

            if (io_proc->io_remaining_time == 0)  //io가 끝났을떄
            {
                pop_front(&waiting);
                io_proc->current_io_index++;  //다음 io로 index증가
                io_proc->state = READY;

                int idx = 0;
                Node* cur = ready.head; //첫번쨰 노드
                while (cur)//노드 탐색
                {
                    if (io_proc->remaining_time < cur->process->remaining_time) break; //curNode보다 짧을떄 break, curnode앞에 삽입
                    idx++;
                    cur = cur->next; //다음노드
                }
                insert(&ready, idx, io_proc);
            }
        }

        if (running != NULL) // 현재 실행 중인 프로세스 처리
        {
            if (start_time == -1) start_time = current_time;

            running->remaining_time--;

            // IO 요청 도달
            if (running->current_io_index < running->io_count &&       //유효한 io 인덱스인지
                running->cpu_burst_time - running->remaining_time ==   //io_request_time탐지 (총 burst-remain=실행한시간)
                running->io_request_times[running->current_io_index])
            {
                running->io_remaining_time = running->io_burst_times[running->current_io_index];//i/o remain time set 해서 컨트롤
                running->state = WAITING;
                append(&waiting, running); //waiting에 삽입

                log[log_index++] = (Running_Log){ running->pid, start_time, current_time }; //기록
                running = NULL; //runniㅜg 초기화
                start_time = -1;
            }
            else if (running->remaining_time == 0) // 프로세스 종료 시
            {
                running->state = TERMINATED;
                log[log_index++] = (Running_Log){ running->pid, start_time, current_time + 1 }; //기록
                running = NULL;
                start_time = -1;
                terminated_count++;
            }
        }

        if (running == NULL && !is_empty(&ready)) // ReadyQueue -> Running
        {
            if (idle_start != -1)
            {
                log[log_index++] = (Running_Log){ 0, idle_start, current_time }; // idle 로그 기록
                idle_start = -1;
            }

            running = pop_front(&ready);
            running->state = RUNNING;
        }

        if (running == NULL && is_empty(&ready)) // IDLE 감지
        {
            if (idle_start == -1)
            {
                idle_start = current_time;
            }
        }

        current_time++; // 1 tick 증가
    }

    Gantt_chart_display(log, "SJF", log_index);
    Evaluate(original, n, log, log_index, "SJF");
}


void Schedule_Priority(Process *p, Process *original, int n)
{
    Running_Log log[2 * MAX_PROCESS_NUM];  // 실행 기록 로그 배열
    int log_index = 0;                     // 로그 인덱스 
    int current_time = 0;                  // 현재 시각
    int complete_Pnum = 0;                     // 완료된 프로세스 개수
    int *is_completed = malloc(sizeof(int)*n);  // 각 프로세스 완료 여부 (0: 미완료, 1: 완료)
    int idle_start = -1; // IDLE 시작시간
    for(int i=0; i<n; i++) is_completed[i]=0; //모두 미완료로 초기화
    

    while (complete_Pnum < n) {  // 모든 프로세스가 완료될 때까지
        int idx = -1;        // 현재 시점에 실행할 프로세스 인덱스
        int min_priority = n+1; // 최소 burst time(초기값은 int_max--최댓값)

     
        for (int i = 0; i < n; i++) { // 프로세스중 burst time 최소인거 찾기
            if (original[i].arrival_time <= current_time && !is_completed[i]) { //도착여부 확인하고, 완료안된 프로세스인지 체크
                if (original[i].priority < min_priority) { //최소 bursttime찾기
                    min_priority = original[i].priority;
                    idx = i; // 조건을 만족한 최소 bursttime의 index값
                }
            }
        }
 
        if (idx == -1) //IDLE일 경우
        { 
            if(idle_start == -1)
            {
                idle_start = current_time;
            }
            current_time++; 
        }
        else
        {
            if (idle_start != -1) //idle 기록(-1이 아니라는건 앞쪽이 idle)
            {
                log[log_index].pid = 0; //idle 기록(idle은 pid== 0정의)
                log[log_index].start_time = idle_start;
                log[log_index].end_time = current_time;
                log_index++;
                idle_start = -1;  //초기화(idle 끝남)
             }
             //프로세스 기록
            log[log_index].pid = original[idx].pid;
            log[log_index].start_time = current_time;
            log[log_index].end_time = current_time + original[idx].cpu_burst_time;

            current_time += original[idx].cpu_burst_time;   // 현재 시간 갱신
            is_completed[idx] = 1;                   // 해당 프로세스 완료 표시
            p[complete_Pnum]=original[idx]; //프로세스 ready queue 정렬
            complete_Pnum++;                     // 완료 개수 증가
            log_index++; 
        }
    }

    
    Gantt_chart_display(log, "Priority", log_index);
    Evaluate(original, n, log, log_index, "Priority");
    free(is_completed);
}