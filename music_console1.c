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
//#include "serialcallLib.h"
// Uncomment to test on the PC
#include "audiocallLib.h"

/**********************************************************
 *  CONSTANTS
 *********************************************************/
#define SIMULATOR 1// 1 use simulator, 0 use serial
#define NSEC_PER_SEC 1000000000UL
#define DEV_NAME "/tyCo/1"

// Path of audio file in windows
#define FILE_NAME "host:/c/Users/str/Desktop/practica_2_2020-v1/let_it_be_1bit.raw"

// Uncomment to test on the Arduino
//#define PERIOD_TASK_SEC	0			/* Period of Task   */
//Dividir el periodo entre 8 para aumentar la frecuencia
//#define PERIOD_TASK_NSEC  512000000	/* Period of Task   */
//#define SEND_SIZE 256    /* BYTES */

// Uncomment to test on the PC
#define PERIOD_TASK_SEC	1			/* Period of Task   */
#define PERIOD_TASK_NSEC  0	/* Period of Task   */
#define SEND_SIZE 500    /* BYTES */

/**********************************************************
 *  GLOBALS
 *********************************************************/
int fd_file = -1;
int fd_serie = -1;
int ret = 0;

struct timespec start, finish, diff, cycle;
struct timespec cycle_1, cycle_2, cycle_3;

pthread_mutex_t mutex;
pthread_mutexattr_t mutex_attr;
pthread_attr_t thread_attr_1, thread_attr_2, thread_attr_3;
struct sched_param sched_param_1, sched_param_2, sched_param_3;
pthread_t thread_1, thread_2, thread_3;

int status = 1; // Status of reproduction: 0 OFF and 1 ON

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

/*
void *task_send() {
	unsigned char buf[SEND_SIZE];
	int flag;

	while (1) {
		clock_gettime(CLOCK_REALTIME,&currentTimeTask1);
		unsigned char buf[256];
		bzero(buf, sizeof(buf));

		pthread_mutex_lock(&mutex);
		if (status == 0) {
			flag = 0;
		} else {
			flag = 1;
		}
		pthread_mutex_unlock(&mutex);

		if (flag == 0) {
			// write to serial port  		
			// Uncomment to test on the Arduino
			//ret = writeSerialMod_256 (buf);

			// Uncomment to test on the PC
			// Envia la musica de 1 bit, la version sin 4000 8 bit
			ret = reproducir_1bit_4000 (buf);
			if (ret < 0) {
				printf("write: error writting serial\n");
				return NULL;
			}				
		} else {
			// read from music file
			ret=read(fd_file,buf,SEND_SIZE);
			if (ret < 0) {
				printf("read: error reading file\n");
				return NULL;
			}

			// write to serial port  		
			// Uncomment to test on the Arduino
			//ret = writeSerialMod_256 (buf);

			// Uncomment to test on the PC
			// Envia la musica de 1 bit, la version sin 4000 8 bit
			ret = reproducir_1bit_4000 (buf);
			if (ret < 0) {
				printf("write: error writting serial\n");
				return NULL;
			}			
		}

		// get end time, calculate lapso and sleep
		clock_gettime(CLOCK_REALTIME,&lastExecutionTask1);
		diffTime(lastExecutionTask1,currentTimeTask1,&diff1);
		if (0 >= compTime(cycle1,diff1)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}
		diffTime(cycle1,diff1,&diff1);
		nanosleep(&diff1,NULL);   
		//addTime(start,cycle,&start);
	}
}

void *task_resume_stop() {
	int flag;
	char buffer[5];
	while (1) {
		clock_gettime(CLOCK_REALTIME,&currentTimeTask2);
		flag = unblockRead(0, buffer, 5);
		if (flag > 0) {
			if (strcmp("0", buffer) == 0) {
				pthread_mutex_lock(&mutex);
				status = 0;
				pthread_mutex_unlock(&mutex);
			} else if (strcmp("1", buffer) == 0) {
				pthread_mutex_lock(&mutex);
				status = 1;
				pthread_mutex_unlock(&mutex);				
			}
		}
		clock_gettime(CLOCK_REALTIME,&lastExecutionTask2);
		diffTime(lastExecutionTask2,currentTimeTask2,&diff2);
		if (0 >= compTime(cycle2,diff2)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}
		diffTime(cycle2,diff2,&diff2);
		nanosleep(&diff2,NULL);   
		//addTime(start,cycle,&start);
	}
}*/

void *task_status() {
	struct timespec current_time, prev_time, diff_time;
	
	while (1) {
		clock_gettime(CLOCK_REALTIME, &current_time);
		pthread_mutex_lock(&mutex);
		if (status == 1) printf("Reproduccion en marcha\n");
		else printf("Reproduccion en pausa\n");
		pthread_mutex_unlock(&mutex);

		clock_gettime(CLOCK_REALTIME, &prev_time);
		diffTime(prev_time, current_time, &diff_time);
		
		if (0 >= compTime(cycle_3, diff_time)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}
		diffTime(cycle_3, diff_time, &diff_time);
		nanosleep(&diff_time, NULL);
	}
}

/*****************************************************************************
 * Function: main()
 *****************************************************************************/
int main()
{
	// Inicialize driver
	if (SIMULATOR)
		iniciarAudio_Windows ();
	else
		fd_serie = initSerialMod_WIN_115200 ();

	// Open music file
	printf("open file %s begin\n",FILE_NAME);
	fd_file = open (FILE_NAME, O_RDONLY, 0644);
	if (fd_file < 0) {
		printf("open: error opening file\n");
		return -1;
	}

	// Inicialize mutex
	pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_PROTECT);
	pthread_mutexattr_setprioceiling(&mutex_attr, 3);
	pthread_mutex_init(&mutex, &mutex_attr);

	// Create atributes for threads
	pthread_attr_init(&thread_attr_1);
	cycle_3.tv_sec = 0.512;
	cycle_3.tv_nsec = 0;
	sched_param_1.sched_priority = 3;// TODO
	pthread_attr_setschedparam(&thread_attr_1, &sched_param_1);
	pthread_attr_setscope(&thread_attr_1, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&thread_attr_1, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&thread_attr_1, SCHED_FIFO);

	pthread_attr_init(&thread_attr_2);
	cycle_3.tv_sec = 2;
	cycle_3.tv_nsec = 0;
	sched_param_2.sched_priority = 2;// TODO
	pthread_attr_setschedparam(&thread_attr_2, &sched_param_2);
	pthread_attr_setscope(&thread_attr_2, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&thread_attr_2, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&thread_attr_2, SCHED_FIFO);

	pthread_attr_init(&thread_attr_3);
	cycle_3.tv_sec = 5;
	cycle_3.tv_nsec = 0;
	sched_param_3.sched_priority = 1;// TODO
	pthread_attr_setschedparam(&thread_attr_3, &sched_param_3);
	pthread_attr_setscope(&thread_attr_3, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&thread_attr_3, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&thread_attr_3, SCHED_FIFO);


	// Create the treads
	/*pthread_create(&thread_1, &thread_attr_1, &task_send, NULL);
	pthread_create(&thread_2, &thread_attr_2, &task_resume_stop, NULL);*/
	pthread_create(&thread_3, &thread_attr_3, &task_status, NULL);

}

/*// Main ejemplo
/*int main()
{
	struct timespec start,end,diff,cycle;
	unsigned char buf[SEND_SIZE];
	int fd_file = -1;
	int fd_serie = -1;
	int ret = 0;

	if (SIMULATOR)
		iniciarAudio_Windows ();
	else
		fd_serie = initSerialMod_WIN_115200 ();

	// Open music file
	printf("open file %s begin\n",FILE_NAME);
	fd_file = open (FILE_NAME, O_RDONLY, 0644);
	if (fd_file < 0) {
		printf("open: error opening file\n");
		return -1;
	}

	// loading cycle time
	cycle.tv_sec=PERIOD_TASK_SEC;
	cycle.tv_nsec=PERIOD_TASK_NSEC;

	clock_gettime(CLOCK_REALTIME,&start);
	while (1) {

		// read from music file
		ret=read(fd_file,buf,SEND_SIZE);
		if (ret < 0) {
			printf("read: error reading file\n");
			return NULL;
		}

		if (SIMULATOR)
			ret = reproducir_1bit_4000 (buf);
		else
			ret = writeSerialMod_256 (buf);

		if (ret < 0) {
			printf("write: error writting serial\n");
			return NULL;
		}

		// get end time, calculate lapso and sleep
		clock_gettime(CLOCK_REALTIME,&end);
		diffTime(end,start,&diff);
		if (0 >= compTime(cycle,diff)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}
		diffTime(cycle,diff,&diff);
		nanosleep(&diff,NULL);   
		addTime(start,cycle,&start);
	}
}*/
