// JOSH LIU ID:260612384

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>

#define SMSIZE 100

// null determines whether it is an actually job
typedef struct job {

	int client;
	int pages;
	int duration;
	int null;
} job;

// holds the buffer and semaphore
typedef struct jobbuffer {

	int buffersize;
	sem_t mutex;
	job buffer[];

} jobbuffer;


// 3 arguments, client, pages, duration 
int main(int argc, char *argv[]) {

        int shmid;
	jobbuffer *jb;

    	// setting up shared memory
	const char *key = "/printing";
	shmid = shm_open(key, O_CREAT | O_RDWR, 0666);
	if(shmid == -1) {
		perror("shm_open:");
		exit(1);
	}	
	ftruncate(shmid, sizeof(jobbuffer));

	// attached shared memory
	jb = (struct jobbuffer*) mmap(0, sizeof(int) + sizeof(sem_t) + 100*sizeof(job), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0); 
	
	// get job params
	int client = atoi(argv[1]);
	int pages  = atoi(argv[2]); 
	int duration = atoi(argv[3]);
	
	int full = 0;
	while(1) {
		
		// loops through buffer to find the first empty space
		int i;
		for (i = 0; i < jb->buffersize; i++) {
		
			job *p = &(jb->buffer[i]);
			if (p->null != 0) continue;
			sem_wait(&jb->mutex);	
			p->client = client;
			p->pages = pages;
			p->duration = duration;
			p->null = 1;
			if (full == 0) printf("Client %d has %d pages to print, puts request in Buffer[%d]\n", p->client, p->pages, i);
			else printf("Client %d wakes up, has %d pages to print, puts request in Buffer[%d]\n", p->client, p->pages, i);
			sem_post(&jb->mutex);	
			
			// unmapping memory
			munmap(&jb, sizeof(int) + sizeof(sem_t) + 100*sizeof(job));
			exit(1);
		}
		// Buffer is full
		if (full == 0) {
			printf("Buffer full... waiting.\n");
			full = 1;
		}
	}
	// unmapping memory
	munmap(&jb, sizeof(int) + sizeof(sem_t) + 100*sizeof(job));
	exit(1);
	return 0;
}
