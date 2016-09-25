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

// prints the buffer for testing
void printjobs(jobbuffer *jb, int bsize) {

	job *p;
	int j;
	for (j = 0; j < bsize; j++) {
		p = &(jb->buffer[j]);
		printf("%d %d in buffer %d\n", p->client, p->pages, j); 
	}
}

// removes the first job in the buffer and shifts all the other jobs up 
void clearjob(jobbuffer *jb, int bsize) {
	
	int j;
	for (j = 0; j < bsize - 1; j++) {
		job *p = &(jb->buffer[j]);
		job *p2 = &(jb->buffer[j+1]);
		p->client = p2->client;
		p->pages = p2->pages;
		p->duration = p2->duration;
		p->null = p2->null;
	}
	job *p3 = &(jb->buffer[bsize - 1]);
	p3->client = 0;
	p3->pages = 0;
	p3->duration = 0;
	p3->null = 0;
}

// requires one argument (size of buffer)
int main(int argc, char *argv[]) {

        int shmid;
	jobbuffer *jb;
	
	int bsize = atoi(argv[1]);
	job buff[bsize];

    	// opening shared memory
	const char *key = "/printing";
	shmid = shm_open(key, O_CREAT | O_RDWR, 0666);
	if(shmid == -1){
		perror("shm_open:");
		exit(1);
	}
	ftruncate(shmid, sizeof(jobbuffer));
	
	// attaching buffer to memory
	jb = (struct jobbuffer*) mmap(0, sizeof(int) + sizeof(sem_t) + bsize*sizeof(job), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0); 

	// initializing semaphore for memory
	sem_init(&(jb->mutex), 0, 1);

	// initializing jobs in buffer
	sem_wait(&jb->mutex);
	int i;
	for (i = 0; i < bsize; i++) {
		job *p = &(jb->buffer[i]);
		p->client = 0;
		p->pages = 0;
		p->duration = 0;
		p->null = 0;
	}
	jb->buffersize = bsize;
	sem_post(&jb->mutex);

	printf("\nServer on\n");
	int empty = 0;
	while(1) {

		sleep(2);
		job *p = &(jb->buffer[0]);
		sem_wait(&jb->mutex); 
		if (p->null == 0) {
			// Empty buffer
			if (empty == 0) {
				printf("No request in buffer, Printer sleeps\n");
				sleep(2);
				empty = 1;
			}	
			sem_post(&jb->mutex);		
			continue;
		}
		sem_post(&jb->mutex);
		empty = 0;

		// "printing"...
		printf("Printer starts printing Client %d's request of %d pages\n", p->client, p->pages);
		sleep(p->duration);
		printf("Printer finishes printing Client %d's request of %d pages\n", p->client, p->pages);	 
		
		// clear done job from the buffer
		sem_wait(&jb->mutex);
		clearjob(jb, bsize);
		sem_post(&jb->mutex);
		sleep(2);
	}
        return 0;
}                      
