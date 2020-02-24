#ifndef SHMEMSEM_H
#include <semaphore.h>

typedef struct {
	int seconds;
	int nanoseconds;
	int sharedmsg[4];
	int pcb[18][3];
	sem_t mutexclock;
	int pidcounter;
	int numofiterations;
	int maxclaims[18][10];
	int allocation[18][10];
	int left[18][10];
	int available[10];
	int processnum[18];
	int processnumcounter;

	int blockedqueue[18][3];
	int blockedcounter;
	int allblocked;//set to one when there is only 1 free process in the system

} SharedData;

#define SHMEMSEM_H
#endif
/*r0,r1,r2 are shared resources and r3-r9 are non sharable.
 * PCB
[0]=PID
[1]=clock seconds
[2]=clock nanoseconds

 sharedmsg
 [0]-carries PID
 [1]- 1 for request 2 for release
 [2]- resource number
 [3]- quantity

 blockedqueue:
 [0]-carries PID
 [1]-carried resource number that it is blocked on
 [2]-amount of resource it requested

 blockedcounter has the number of processes in blocked queue
*/
