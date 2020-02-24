Functions and flow of the program :

Shared memory:

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
int allblocked;

The seconds and nanoseconds hold the clock value incrementing in OSS. shared msg[4] - index 0 contains the PID for the user process, index 1 contains if the process wants to request (1), release (2), or terminate (-1), index 2 contians which resource number (r0-r9) it wants to request/release or the process matrix index if its terminating(-1) and index 3 contains the amount of that resource to release or request . In my program r0,r1,r2 are shared resources and r3-r9 are non sharable resources. pidcounter holds the number of processes generated so far by the OSS. maxclaims, allocation, available and left are all matrices for the deadlock avoidance algorithm. We have a blocked queue and blockedcounter holds the number of processes in blocked queue. allblocked is set if there is only 1 free process in the system and there are atleast 3 processes in the system and the rest are blocked. 

OSS:
void cleanup();
void catch_alarm(int sig);
void killprocesses();
void updatePCB(int process);
void clearmatrix(int counter);
void printmatrix();
void printsimulatedmatrix();
void calcleft();
void printpcb();
int emptyPCB();
bool algo(int allocation [18][10],int left[18][10],int r,int amount,int c, int available[10]);
bool avoidance(int r, int amount,int currentprocess);
void startprocess();
int processnumber(int pid);
void checkblocked();
void checknumofblocked();
void printmatrixfile();
void checklines();

The master starts by initializing all the values in shared memory, matrices, blocked queue, semaphore, available resource matrix, etc. It calls startprocess() which does the actual work. In this function the OSS does sem_wait on the semaphore and first checks for an empty spot in PCB and forks a child, second it checks shared message for a message from user processes. If the user process requests the resource it calls avoidance() which inturn calls algo() which contains my main bankers algorithm. A local copy of the allocated and left matrices are created to simulate the request granted. After the algorithm runs twice with the same number of processes that could still not finish, it returns false to startprocess() indicating a deadlock may occur if the process is granted and the request will be denied and the process will enter the blocked queue. If all the processes in the system were able to finish it returns true and the request is granted. Everytime a resource is released by any user process, the matrices are updated and the blocked queue is checked by calling checkblocked() (for resources 3-9) to see if the OSS can grant any requests that were blocked. The OSS also calls checknumofblocked() before each sem_post which checks the number of free and blocked processes in the system. If there is only 1 free (non-blocked) process and the system has atleast 3 processes then the shared memory variable allblocked is set to 1 this variable is checked by the user process everytime it has the semaphore, if this is equal to 1 then the user process must keep releasing resources till atleast one process is removed from the blocked queue. This prevents all the processes in the system to be blocked at a time.

User processes:
bool resourceempty();
void printmatrix();
bool requestedall();

The user process has 2 critical sections. It starts by attaching to the shared memory and then sem_waits on the semaphore. The process searches for its PCB index position by iterating through the PCB and saves this as mymatrixcounter. The user process randomly chooses the max claims for each resource from the available resource matrix, sets maxdone=true then sem_posts. maxdone prevents the user processes from requesting resources before declaring its max claims. Then the do-while loop starts where the process sem_waits on the semaphore, if the process is in blocked queue or it has not declared its max claims it seeds the semaphore immediately. If not, it then checks if its local nanoseconds and shared memory nanoseconds have a difference of B (randomly chosen threshold as mentioned in description) if yes then it goes on to check if the shared memory msg is equal to 0. It calls resourceempty() to check if nothing has been allocated for the process so far, if this function returns true then the user process can only chose to request reaources. If allocated matrix for the process is not empty then it chooses randomly between (1-3) if the selection is 1 or 3 then it requests, if its 2 it releases. The probability of release being choosen is 0 till the process has requested atleast 2 times and after that it is 1/3. Everytime the user process makes a request or release decision, it also calls requestedall() which returns true if all the max claims declared at the beginning for the process has been allocated. If this function returns true then the process can only release resources. The process also checks if the shared memory variable allblocked is set to 1 as mentioned in above paragraph. The amount or resource to request/release is chosen randomly from the left[] matrix. For termination the description says that it must run for atleast 1 second before terminating and since I am incrementing my clock only 1000 ns (500 for overhead and 500 for actual clock) at a time it never really reaches 1 second. So I made the termination criteria once the shared memory clock reaches 20,000 and the difference between the time when the process was created and shared memory clock is 10,000 and the process has requested resources atleast once, then a random number between 0-3 is selected if 0 was choosen then it terminates so the probability is 1/3 after the time check. 

Problems during the project

1. Again my biggest problem is that my program is a bit slow. The reason is I had to add delays after every sem_post or the process that released it grabbed it back again. Another reason is once there are around 15 processes in the system they all sem_wait on the semaphore delaying the OSS. 

2. Initially my OSS had 2 critical sections, one for incrementing the clock and one for checking shared message and this made the delays worse so I decided to make it into one big critical section In this way the OSS can finish around 4-5 more iterations than it did before. 

3.The termination criteria is a little different from description but I only did this so you could see some processes terminating as well. 

4. Since the probability of release is so small we can barely see the processes in the blocked queue being granted its requests. However, if we limit the number of processes to 3 (make emptypcb() for loop <3 instead of <18) the processes get to run more frequently, there are less delays and we can see some processes from blocked queue being freed and granted their requests.

5. I really wanted to add scheduling to the project but I am out of time. It would have really made everything work faster.

6. The alarm is currently 10 even though you mentioned 3 life seconds in class because with 3 seconds the OSS barely did 15 iterations which I felt was too small.

7. I didn't really understand the verbose option mentioned in the description. So right now I am printing matrices after every second request that has been granted for resources 3-9 and I print them everytime resources 3-9 are released. If the request/release was for resource 0,1 or 2 then I never print the matrices since these are shared resources. I know in the description it says every 20 requests but on the whole my OSS does around 30-35 iterations and I really wanted you to have a good picture of what was going on so I did every second request instead of 20. If you want to see the matrices printed at all times for resources 3-9 you can comment out the line in OSS: if(shmem->numofiterations%2==0).

8. For shared resources(0-2) everytime it is requested or released I add and deduct from the allocated and left matrices but never from and to the available matrice. Because no matter how many processes are using this resource it will always have the initial amount available. 

9. When resources 0,1 or 2 are released I don't check the blocked queue like I do with other resource release because a process is never blocked on shared resource so its release does not make a difference to the blocked process.

10. The blocked processes also sem_wait on the semaphore even if they are blocked. This was a big problem. I tried to go around it but sleep() was making things worse. In the end I just left it as is and added as much delay as I could after sem_post so it doesn't try to get back in immediately.

11. sometimes after 25-26 iterations the oss was grabbing and releasing the semaphore continuously so after the OSS finishes 25 iterations I increased the delays after sem_post in both user and OSS. This seemed to stop this problem but also made it slower.

12. The program barely reaches 2000 lines in log file after I keep the alarm to 20 so I did not put the check for number of lines in the log file each time it writes to the file. I do however have a checklines() function setup to check this, but I deleted out the lines from the code to avoid further delays. By deleting I could again make the OSS do 2-3 more iterations.

13. Sometimes when you run the program it might feel like it hangs a bit but this is towards the end when its killing the remaining processes or when a process is terminating it waits till its out of the system.

14. If no resources were released during your run just run it few more times it should release during some of them. 

15. I really appreciate professor marks help and all the guidance. Specially showing me how to run the program on gdb, that was soo helpful! I feel very happy if I submit even a half working project which I worked really hard on. Typing out the README and talking about what I did during the project is my favorite part of submiting a project. I really enjoyed working on this project. I hope you enjoy running it too. Thank you!