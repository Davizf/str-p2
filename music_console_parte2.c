/**********************************************************
 *  INCLUDES
 *********************************************************/
#include <vxWorksCommon.h>
#include <vxWorks.h>
#include <stdio.h>
#include <fcntl.h>
#include <ioLib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <selectLib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sioLib.h>
#include <sched.h>
// Uncomment to test on the Arduino with the serial module
#include "serialcallLib.h"
// Uncomment to test on the PC
#include "audiocallLib.h"

/**********************************************************
 *  CONSTANTS
 *********************************************************/
#define SIMULATOR 1// 1 use simulator, 0 use serial
#define COUNT_TIME 1// 1 use timers for count time, 0 not
#define NSEC_PER_SEC 1000000000UL
#define DEV_NAME "/tyCo/1"

// Path of audio file in windows
#define FILE_NAME "host:/c/Users/str/Desktop/practica_2_2020-v1/let_it_be.raw"

#define ON 1
#define OFF 0

/**********************************************************
 *  GLOBALS
 *********************************************************/
int fd_file = -1;
int fd_serie = -1;
int ret = 0;

struct timespec cycle_1, cycle_2, cycle_3;
pthread_mutex_t mutex;

int state = ON; // State of reproduction: OFF and ON
int send_size;


/**********************************************************
 *  IMPLEMENTATION
 *********************************************************/

/*
 * Function: unblockRead
 */
int unblockRead(int fd, char *buffer, int size)
{
	fd_set readfd;
	struct timeval wtime;
	int ret;

	FD_ZERO(&readfd);
	FD_SET(fd, &readfd);
	wtime.tv_sec=0;
	wtime.tv_usec=0;
	ret = select(2048,&readfd,NULL,NULL,&wtime);
	if (ret < 0) {
		perror("ERROR: select");
		return ret;
	}
	if (FD_ISSET(fd, &readfd)) {
		ret = read(fd, buffer, size);
		return (ret);
	} else {
		return (0);
	}
}

/*
 * Function: diffTime
 */
void diffTime(struct timespec end, 
		struct timespec start, 
		struct timespec *diff) 
{
	if (end.tv_nsec < start.tv_nsec) {
		diff->tv_nsec = NSEC_PER_SEC - start.tv_nsec + end.tv_nsec;
		diff->tv_sec = end.tv_sec - (start.tv_sec+1);
	} else {
		diff->tv_nsec = end.tv_nsec - start.tv_nsec;
		diff->tv_sec = end.tv_sec - start.tv_sec;
	}
}

/*
 * Function: addTime
 */
void addTime(struct timespec end, 
		struct timespec start, 
		struct timespec *add) 
{
	unsigned long aux;
	aux = start.tv_nsec + end.tv_nsec;
	add->tv_sec = start.tv_sec + end.tv_sec + 
			(aux / NSEC_PER_SEC);
	add->tv_nsec = aux % NSEC_PER_SEC;
}

/*
 * Function: compTime
 */
int compTime(struct timespec t1, 
		struct timespec t2)
{
	if (t1.tv_sec == t2.tv_sec) {
		if (t1.tv_nsec == t2.tv_nsec) {
			return (0);
		} else if (t1.tv_nsec > t2.tv_nsec) {
			return (1);
		} else if (t1.tv_sec < t2.tv_sec) {
			return (-1);
		}
	} else if (t1.tv_sec > t2.tv_sec) {
		return (1);
	} else if (t1.tv_sec < t2.tv_sec) {
		return (-1);
	} 
	return (0);
}

void *task_send(void *p) {
	struct timespec start, end, diff, timer_start, timer_end, timer_diff;
	int actual_state;
	unsigned char buf[send_size];

	clock_gettime(CLOCK_REALTIME, &start);
	while (1) {
		if (COUNT_TIME) clock_gettime(CLOCK_REALTIME, &timer_start);

		pthread_mutex_lock(&mutex);
		actual_state = state;
		pthread_mutex_unlock(&mutex);

		// Send 0's if OFF
		//bzero(buf, sizeof(buf));
		for (int i=0; i<send_size; i++) buf[i]='0';

		if (actual_state == ON) {
			ret=read(fd_file, buf, send_size);
			if (ret < 0) {
				printf("read: error reading file\n");
				return NULL;
			}
		}

		if (SIMULATOR) ret = reproducir_4000 (buf);
		else ret = writeSerialMod_256 (buf);

		if (ret < 0) {
			printf("write: error writting serial\n");
			return NULL;
		}

		clock_gettime(CLOCK_REALTIME, &end);
		diffTime(end, start, &diff);
		if (0 >= compTime(cycle_1, diff)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}

		if (COUNT_TIME) {
			clock_gettime(CLOCK_REALTIME, &timer_end);
			diffTime(timer_end, timer_start, &timer_diff);

			unsigned long long to_ns=1000000000UL;
			unsigned long long timer_ns = timer_diff.tv_sec*to_ns + timer_diff.tv_nsec;
			unsigned long long timer_ms=(unsigned long long)timer_ns/1000;

			/*printf("start: %lld.%.9ld\n", (long long)timer_start.tv_sec, timer_start.tv_nsec);
			printf("end: %lld.%.9ld\n", (long long)timer_end.tv_sec, timer_end.tv_nsec);
			printf("diff: %lld.%.9ld\n", (long long)timer_diff.tv_sec, timer_diff.tv_nsec);

			printf("to_ns=%llu\n", to_ns);
			printf("ns=%llu\n", (unsigned long long)(timer_diff.tv_sec*to_ns));*/

			printf("---------Time task_send is: %llu ns, %llu ms\n", timer_ns, timer_ms);
		}

		diffTime(cycle_1, diff, &diff);
		nanosleep(&diff, NULL);
		addTime(start, cycle_1, &start);
	}
}

void changeState(int new_status) {
	pthread_mutex_lock(&mutex);
	state = new_status;
	pthread_mutex_unlock(&mutex);
	//printf("Changed state to %s\n", (state==OFF)?"OFF":"ON");
}

void *task_resume_stop(void *p) {
	struct timespec start, end, diff, timer_start, timer_end, timer_diff;
	int read_key, buffer_size=3;
	char buffer[3];// character and \n

	clock_gettime(CLOCK_REALTIME, &start);
	while (1) {
		if (COUNT_TIME) clock_gettime(CLOCK_REALTIME, &timer_start);

		//bzero(buffer, buffer_size);
		for (int i=0; i<buffer_size; i++) buffer[i]='0';

		read_key=unblockRead(0, buffer, buffer_size);
		if (read_key > 0) {
			if (buffer[0] == '0' && state != OFF)
				changeState(OFF);
			else if (buffer[0] == '1' && state != ON)
				changeState(ON);
		}

		clock_gettime(CLOCK_REALTIME, &end);
		diffTime(end, start, &diff);
		if (0 >= compTime(cycle_2, diff)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}

		if (COUNT_TIME) {
			clock_gettime(CLOCK_REALTIME, &timer_end);
			diffTime(timer_end, timer_start, &timer_diff);

			unsigned long long to_ns=1000000000UL;
			unsigned long long timer_ns = timer_diff.tv_sec*to_ns + timer_diff.tv_nsec;
			unsigned long long timer_ms=(unsigned long long)timer_ns/1000;

			/*printf("start: %lld.%.9ld\n", (long long)timer_start.tv_sec, timer_start.tv_nsec);
			printf("end: %lld.%.9ld\n", (long long)timer_end.tv_sec, timer_end.tv_nsec);
			printf("diff: %lld.%.9ld\n", (long long)timer_diff.tv_sec, timer_diff.tv_nsec);

			printf("to_ns=%llu\n", to_ns);
			printf("ns=%llu\n", (unsigned long long)(timer_diff.tv_sec*to_ns));*/

			printf("---------Time task_resume_stop is: %llu ns, %llu ms\n", timer_ns, timer_ms);
		}

		diffTime(cycle_2, diff, &diff);
		nanosleep(&diff, NULL);
		addTime(start, cycle_2, &start);
	}
}

void *task_state(void *p) {
	struct timespec start, end, diff, timer_start, timer_end, timer_diff;

	clock_gettime(CLOCK_REALTIME, &start);
	while (1) {
		if (COUNT_TIME) clock_gettime(CLOCK_REALTIME, &timer_start);

		pthread_mutex_lock(&mutex);
		if (state == ON) printf("Reproduccion en marcha\n");
		else printf("Reproduccion en pausa\n");
		pthread_mutex_unlock(&mutex);

		clock_gettime(CLOCK_REALTIME, &end);
		diffTime(end, start, &diff);

		if (0 >= compTime(cycle_3, diff)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}

		if (COUNT_TIME) {
			clock_gettime(CLOCK_REALTIME, &timer_end);
			diffTime(timer_end, timer_start, &timer_diff);

			unsigned long long to_ns=1000000000UL;
			unsigned long long timer_ns = timer_diff.tv_sec*to_ns + timer_diff.tv_nsec;
			unsigned long long timer_ms=(unsigned long long)timer_ns/1000;

			/*printf("start: %lld.%.9ld\n", (long long)timer_start.tv_sec, timer_start.tv_nsec);
			printf("end: %lld.%.9ld\n", (long long)timer_end.tv_sec, timer_end.tv_nsec);
			printf("diff: %lld.%.9ld\n", (long long)timer_diff.tv_sec, timer_diff.tv_nsec);

			printf("to_ns=%llu\n", to_ns);
			printf("ns=%llu\n", (unsigned long long)(timer_diff.tv_sec*to_ns));*/

			printf("---------Time task_state is: %llu ns, %llu ms\n", timer_ns, timer_ms);
		}

		diffTime(cycle_3, diff, &diff);
		nanosleep(&diff, NULL);
		addTime(start, cycle_3, &start);
	}
}

/*****************************************************************************
 * Function: main()
 *****************************************************************************/
int main()
{
	if (SIMULATOR) {
		iniciarAudio_Windows ();
		send_size = 4000;
	} else {
		fd_serie = initSerialMod_WIN_115200 ();
		send_size = 256;
	}

	// Open music file
	printf("open file %s begin\n",FILE_NAME);
	fd_file = open (FILE_NAME, O_RDONLY, 0644);
	if (fd_file < 0) {
		printf("open: error opening file\n");
		return -1;
	}

	// Inicialize mutex
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_PROTECT);
	pthread_mutexattr_setprioceiling(&mutex_attr, 3);
	pthread_mutex_init(&mutex, &mutex_attr);

	// Create atributes for threads
	pthread_attr_t thread_attr_1, thread_attr_2, thread_attr_3;
	struct sched_param sched_param_1, sched_param_2, sched_param_3;

	pthread_attr_init(&thread_attr_1);
	if (SIMULATOR) {
		cycle_1.tv_sec = 1;
		cycle_1.tv_nsec = 0;
	} else {
		cycle_1.tv_sec = 0;
		cycle_1.tv_nsec = 64000000;
	}
	sched_param_1.sched_priority = 3;
	pthread_attr_setschedparam(&thread_attr_1, &sched_param_1);
	pthread_attr_setscope(&thread_attr_1, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&thread_attr_1, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&thread_attr_1, SCHED_FIFO);

	pthread_attr_init(&thread_attr_2);
	cycle_2.tv_sec = 2;
	cycle_2.tv_nsec = 0;
	sched_param_2.sched_priority = 2;
	pthread_attr_setschedparam(&thread_attr_2, &sched_param_2);
	pthread_attr_setscope(&thread_attr_2, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&thread_attr_2, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&thread_attr_2, SCHED_FIFO);

	pthread_attr_init(&thread_attr_3);
	cycle_3.tv_sec = 5;
	cycle_3.tv_nsec = 0;
	sched_param_3.sched_priority = 1;
	pthread_attr_setschedparam(&thread_attr_3, &sched_param_3);
	pthread_attr_setscope(&thread_attr_3, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&thread_attr_3, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&thread_attr_3, SCHED_FIFO);

	// Create the treads
	pthread_t thread_1, thread_2, thread_3;
	pthread_create(&thread_1, &thread_attr_1, &task_send, NULL);
	pthread_create(&thread_2, &thread_attr_2, &task_resume_stop, NULL);
	pthread_create(&thread_3, &thread_attr_3, &task_state, NULL);

	pthread_join(thread_1, NULL);
	pthread_join(thread_2, NULL);
	pthread_join(thread_3, NULL);

	return;
}
