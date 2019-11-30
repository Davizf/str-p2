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
// Uncomment to test on the Arduino with the serial module
//#include "serialcallLib.h"
// Uncomment to test on the PC
#include "audiocallLib.h"



/**********************************************************
 *  CONSTANTS
 *********************************************************/
#define NSEC_PER_SEC 1000000000UL
#define DEV_NAME "/tyCo/1"

// Path of audio file in windows
#define FILE_NAME "host:/C/Users/str/Desktop/practica_2_2012/let_it_be_1bit.raw"


// Uncomment to test on the Arduino
//#define PERIOD_TASK_SEC	0			/* Period of Task   */
//#define PERIOD_TASK_NSEC  512000000	/* Period of Task   */
//#define SEND_SIZE 256    /* BYTES */

// Uncomment to test on the PC
#define PERIOD_TASK_SEC	1			/* Period of Task   */
#define PERIOD_TASK_NSEC  0	/* Period of Task   */
#define SEND_SIZE 500    /* BYTES */




/**********************************************************
 *  GLOBALS
 *********************************************************/

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

/*****************************************************************************
 * Function: main()
 *****************************************************************************/
int main()
{
    struct timespec start,end,diff,cycle;
    unsigned char buf[SEND_SIZE];
    int fd_file = -1;
    int fd_serie = -1;
    int ret = 0;
    
	// Uncomment to test on the Arduino
    //fd_serie = initSerialMod_WIN_115200 ();

	// Uncomment to test on the PC
	iniciarAudio_Windows ();

	/* Open music file */
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
		
		// write to serial port  		
		// Uncomment to test on the Arduino
		//ret = writeSerialMod_256 (buf);

		// Uncomment to test on the PC
		ret = reproducir_1bit_4000 (buf);
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
}
