#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <linux/sched.h>

volatile int running = 1;

pthread_barrier_t sync_start_barrier;

pthread_mutex_t mutex;

int nthreads;
int BUFFER_SIZE;
int globalPolicy;
int globalPriority;
char* buffer = NULL;
int currIndex = 0;

void *run(void *data)
{
	int threadId = (int)data;

	//printf("%c arrived\n", (char)((0x41)+threadId));
	pthread_barrier_wait(&sync_start_barrier);
	//printf("%c begun\n", (char)((0x41)+threadId));

	printf("threadId: %d threadChar: %c\n", threadId, (char)((0x41)+threadId));

	while(currIndex < BUFFER_SIZE){
		pthread_mutex_lock(&mutex);
		buffer[currIndex] = 0x41 + threadId;
		currIndex++;
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

void print_sched(int policy)
{
	int priority_min, priority_max;

	switch(policy){
		case SCHED_DEADLINE:
			printf("SCHED_DEADLINE");
			break;
		case SCHED_FIFO:
			printf("SCHED_FIFO");
			break;
		case SCHED_RR:
			printf("SCHED_RR");
			break;
		case SCHED_NORMAL:
			printf("SCHED_OTHER");
			break;
		case SCHED_BATCH:
			printf("SCHED_BATCH");
			break;
		case SCHED_IDLE:
			printf("SCHED_IDLE");
			break;
		default:
			printf("unknown\n");
	}
	priority_min = sched_get_priority_min(policy);
	priority_max = sched_get_priority_max(policy);
	printf(" PRI_MIN: %d PRI_MAX: %d\n", priority_min, priority_max);
}

int setpriority(pthread_t *thr, int newpolicy, int newpriority)
{
	int policy, ret;
	struct sched_param param;

	if (newpriority > sched_get_priority_max(newpolicy) || newpriority < sched_get_priority_min(newpolicy)){
		printf("Invalid priority: MIN: %d, MAX: %d", sched_get_priority_min(newpolicy), sched_get_priority_max(newpolicy));

		return -1;
	}

	pthread_getschedparam(*thr, &policy, &param);
	printf("current: ");
	print_sched(policy);

	param.sched_priority = newpriority;
	ret = pthread_setschedparam(*thr, newpolicy, &param);
	if (ret != 0)
		perror("perror(): ");

	pthread_getschedparam(*thr, &policy, &param);
	printf("new: ");
	print_sched(policy);

	return 0;
}

void buffer_post_processing(){
	int contaEscalonadas[4] = {0, 0, 0, 0};
	char* rply = (char*)malloc(sizeof(char) * BUFFER_SIZE);
	int rplyIndex = 0;
	char curr_char;
	for(int i=0; i<BUFFER_SIZE; i++){
		if(curr_char != buffer[i]){
			curr_char = buffer[i];
			rply[rplyIndex] = curr_char;
			rplyIndex++;
			if(curr_char=='A'){
				contaEscalonadas[0]++;
			}else if(curr_char=='B'){
				contaEscalonadas[1]++;
			}else if(curr_char=='C'){
				contaEscalonadas[2]++;
			}else if(curr_char=='D'){
				contaEscalonadas[3]++;
			}
		}
	}
	for(int i=0; i<BUFFER_SIZE; i++){
		printf("%c", rply[i]);
	}
	printf("\nNumero de vezes escalonadas:\n");
	for(int i=0; i<4; i++){
		printf("%d, ", contaEscalonadas[i]);
	}
	printf("\n");
	printf("A, B, C, D\n");
}

int main(int argc, char **argv)
{
	pthread_mutex_init(&mutex, NULL);
	currIndex = 0;
	BUFFER_SIZE = atoi(argv[1]);
	if(BUFFER_SIZE>0){
		//printf("BUFFER_SIZE: %d ", BUFFER_SIZE);
		buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
	}

	nthreads = atoi(argv[2]);
	globalPolicy = atoi(argv[3]);
	globalPriority = atoi(argv[4]);
	pthread_barrier_init(&sync_start_barrier, NULL, nthreads);
	print_sched(globalPolicy);
	printf("\nnthreads: %d buffersize: %d\n", nthreads, BUFFER_SIZE);

	pthread_t thr[nthreads];

	for(int i=0; i<nthreads; i++){
		pthread_create(&thr[i], NULL, run, (void*)(int)i);
		if(i==0){
			setpriority(&thr[i], globalPolicy, 1);
		}else if(i==1){
			setpriority(&thr[i], globalPolicy, 99);
		}else if(i==2){
			setpriority(&thr[i], globalPolicy, 1);
		}else if(i==3){
			setpriority(&thr[i], globalPolicy, 20);
		}	
	}

	for(int i=0; i<nthreads; i++){
		pthread_join(thr[i], NULL);
	}

	for(int i=0; i<BUFFER_SIZE; i++){
		printf("%c", buffer[i]);
	}
	printf("************************************************\n");
	buffer_post_processing();
	pthread_barrierattr_destroy(&sync_start_barrier);
	// if (argc < 2){
	// 	printf("usage: ./%s <execution_time>\n\n", argv[0]);

	// 	return 0;
	// }

	// timesleep = atoi(argv[1]);
	// pthread_create(&thr, 'A', run, NULL);
	// setpriority(&thr, SCHED_FIFO, 1);
	// sleep(timesleep);
	// running = 0;
	// pthread_join(thr, NULL);

	// return 0;
}
