#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESS_NUM 100 // 최대 프로세스 개수

typedef struct // 프로세스 구조체
{
    int pid;
    int cpu_burst_time;
    int arrival_time;
    int priority;
}Process;

typedef struct { //실행 로그(간트차트나 평가용)
    int pid;
    int start_time;
    int end_time;
} Running_Log;


void copy(Process *src, Process *dest, int n); //프로세스 리스트 복사
void Gantt_chart_display(Running_Log *log, char *name, int n); //간트차트 출력 
void Create_process(Process *p, int n); //프로세스 생성
void Evaluate(Process *p, int n, Running_Log *log, int index, char *name); //평가함수
void Schedule_FCFS(Process *p, Process *original, int n); //FCFS
void Schedule_SJF(Process *p, Process *original, int n);


int main(void)
{
    Process original_process[MAX_PROCESS_NUM]={0}; //초기화
    Process copied_process[MAX_PROCESS_NUM]={0}; 

    Create_process(original_process, 10); // 프로세스 생성
    copy(original_process, copied_process, 10);
    Schedule_FCFS(copied_process,original_process,10); 
    copy(original_process, copied_process, 10);
    Schedule_SJF(copied_process,original_process,10);
    return 0;

}

void copy(Process *src, Process *dest, int n)
{
    for(int i=0; i<n; i++){
        dest[i]=src[i];
    }

}

void Create_process(Process *p, int n)
{   
    int * priority_pool = malloc(sizeof(int)*n); //중복없는 우선순위를 위한 배열 동적할당

    srand(time(NULL)); //시드값 지정

    for(int i=0; i<n; i++) //순서대로 우선순위 풀을 초기화(0,1,2...n-1)
    {
        priority_pool[i]= i;
    }
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
    }

    p[rand()%n].arrival_time=0; //첫번째  idle 방지
     
    free(priority_pool);
}

void Gantt_chart_display(Running_Log *log, char *name, int n)
{
    int *time_lable = malloc(sizeof(int)*n); //간트차트 레이블
    int *interval = malloc(sizeof(int)*n); //레이블 간격
    int index = 0; // time_lable용 인덱스
    int width = 0; // time_lable 간격 (intervaul에 들어감)
    
    printf("\nGantt Chart - %s\n", name);
    printf("|");

    for(int i=0; i<n; i++) //간트차트 출력부
    {
        int burst_time = log[i].end_time - log[i].start_time; // log에 기록된 실행시간

        if(log[i].pid == 0) // idle check 및 표시(idle 프로세스는 0으로 표시)
        {
            width = printf(" IDLE(%d) |", burst_time); // printf 리턴값 저장(간격)
        }
        else
        {
            width = printf(" P%d(%d) |",log[i].pid, burst_time); // 위와 동일
        }

        time_lable[index] = log[i].end_time; //label 값 저장
        interval[index] = width; // 간격 저장
        index++;
    }

    printf("\n0"); //개행 및 time_lable 시작값

    for(int i=0; i<index; i++) //레이블 출력부
    {
        printf("%*d",interval[i],time_lable[i]);
    }
    printf("\n");
    free(time_lable);
    free(interval);
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


void Schedule_FCFS(Process *p,Process *original, int n)
{
    Running_Log log[2 * MAX_PROCESS_NUM]; //실행 기록 로그 배열
    int log_index = 0;
    int current_time = 0;

    for(int i=0; i<n-1; i++) //버블정렬 
    {
        for(int j=0; j<n-i-1; j++)
        {
            if(p[j].arrival_time>p[j+1].arrival_time)
            {
                Process temp = p[j];
                p[j] = p[j+1];
                p[j+1] = temp;
            }
        }
    }

    for (int i = 0; i < n; i++) {   //process log 기록
        if (current_time < p[i].arrival_time) //IDLE 기록
        {  
            log[log_index].pid = 0;  //IDLE 프로세스는 pid = 0 으로 정의
            log[log_index].start_time = current_time; //시작시간은 현재시간
            log[log_index].end_time = p[i].arrival_time; //종료시간(다음 프로세스가 도착할떄까지)
            log_index++;

            current_time = p[i].arrival_time; //현재시간 업데이트
        }
        //여기서부턴 프로세스 기록
        log[log_index].pid = p[i].pid; 
        log[log_index].start_time = current_time;
        log[log_index].end_time = current_time + p[i].cpu_burst_time; //종료시간= 현시간burst time
        log_index++;

        current_time += p[i].cpu_burst_time; //현재 시간 업데이트
    }

    Gantt_chart_display(log,"FCFS",log_index);
    Evaluate(original, n, log, log_index, "FCFS");
}

void Schedule_SJF(Process *p, Process *original, int n)
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
        int min_burst = 1e9; // 최소 burst time(초기값은 int_max--최댓값)

     
        for (int i = 0; i < n; i++) { // 프로세스중 burst time 최소인거 찾기
            if (original[i].arrival_time <= current_time && !is_completed[i]) { //도착여부 확인하고, 완료안된 프로세스인지 체크
                if (original[i].cpu_burst_time < min_burst) { //최소 bursttime찾기
                    min_burst = original[i].cpu_burst_time;
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

    // 실행 로그로 간트차트 출력
    Gantt_chart_display(log, "SJF", log_index);
    // 평가 함수 호출 (waiting/turnaround 계산)
    Evaluate(original, n, log, log_index, "SJF");
    free(is_completed);
}
