/*
 * child.c
 *
 *  Created on: Oct 1, 2019
 *      Author: afrah
 */
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <time.h>
# include <stdlib.h>
# include <dirent.h>
# include <stdio.h>
# include <string.h>
# include <getopt.h>
# include <stdbool.h>
# include <ctype.h>
# include <sys/wait.h>
# include <signal.h>
# include <sys/mman.h>
# include <sys/time.h>
# include <stdint.h>
# include <fcntl.h>
# include <sys/shm.h>
# include "shmemsem.h"
# include <sys/ipc.h>
# include <semaphore.h>
# include <stdbool.h>
# include <pthread.h>

SharedData *shmem;
int shmid;
int mymatrixcounter;
int million=1000000;
bool resourceempty();
void printmatrix();
bool requestedall();

bool resourceempty()
{
	if(shmem->allocation[mymatrixcounter][0]==0 && shmem->allocation[mymatrixcounter][1]==0 && shmem->allocation[mymatrixcounter][2]==0
			&& shmem->allocation[mymatrixcounter][3]==0 && shmem->allocation[mymatrixcounter][4]==0 && shmem->allocation[mymatrixcounter][5]==0 &&
			shmem->allocation[mymatrixcounter][6]==0 && shmem->allocation[mymatrixcounter][7]==0 && shmem->allocation[mymatrixcounter][8]==0 &&
			shmem->allocation[mymatrixcounter][9]==0 )
		return true;
	else
		return false;
}
void printmatrix()
{
	int i,j;
	printf("\nmaxclaims\n");
	for(i=0;i<18;i++)
	{
		for(j=0;j<10;j++)
		{
			printf("%d ",shmem->maxclaims[i][j]);
		}
		printf("\n");
	}
	printf("\nallocation\n");
	for(i=0;i<18;i++)
	{
		for(j=0;j<10;j++)
		{
			printf("%d ",shmem->allocation[i][j]);
		}
		printf("\n");
	}
	printf("\nleft\n");
	for(i=0;i<18;i++)
	{
		for(j=0;j<10;j++)
		{
			printf("%d ",shmem->left[i][j]);
		}
		printf("\n");
	}
}
bool requestedall()
{

	if(shmem->left[mymatrixcounter][0]==0 && shmem->left[mymatrixcounter][1]==0 && shmem->left[mymatrixcounter][2]==0 && shmem->left[mymatrixcounter][3]==0 && shmem->left[mymatrixcounter][4]==0 &&
			shmem->left[mymatrixcounter][5]==0 && shmem->left[mymatrixcounter][6]==0 && shmem->left[mymatrixcounter][7]==0 && shmem->left[mymatrixcounter][8]==0 && shmem->left[mymatrixcounter][9]==0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
int main(int argc, char **argv)
{
		srand(time(NULL) ^ (getpid()<<16));

		//printf("hello from user %d\n",getpid());
		int sum,i;
		int resources[10]={0},resourcecounter=-1;

		int locals; int localn, checktermination,B=(rand()%300);
		int numofrequests=0;
		// Create our key
		key_t shmkey = ftok("makefile",777);
		if (shmkey == -1) {
			perror("user:Ftok failed");
			exit(-1);
		}

		// Get our shm id
		shmid = shmget(shmkey, sizeof(SharedData), 0600 | IPC_CREAT);
		if (shmid == -1) {
			perror("user:Failed to allocate shared memory region");
			exit(-1);
		}

		// Attach to our shared memory
		shmem = (SharedData*)shmat(shmid, (void *)0, 0);
		if (shmem == (void *)-1) {
			perror("user:Failed to attach to shared memory region");
			exit(-1);
		}
		shmem->pidcounter++;
		int pid=getpid();
		//getting the index for the processes in the matrices
		sem_wait(&(shmem->mutexclock));
		for(i=0;i<18;i++)
		{
			if(shmem->pcb[i][0]==pid)
			{
				mymatrixcounter=i;
				break;
			}
		}
		bool Iamalive=true,maxdone=false,sharedmsgdone=false;
		{
			shmem->maxclaims[mymatrixcounter][0]=(rand()%(shmem->available[0]+1));
			shmem->maxclaims[mymatrixcounter][1]=(rand()%(shmem->available[1]+1));
			shmem->maxclaims[mymatrixcounter][2]=(rand()%(shmem->available[2]+1));
			shmem->maxclaims[mymatrixcounter][3]=(rand()%(shmem->available[3]+1));
			shmem->maxclaims[mymatrixcounter][4]=(rand()%(shmem->available[4]+1));
			shmem->maxclaims[mymatrixcounter][5]=(rand()%(shmem->available[5]+1));
			shmem->maxclaims[mymatrixcounter][6]=(rand()%(shmem->available[6]+1));
			shmem->maxclaims[mymatrixcounter][7]=(rand()%(shmem->available[7]+1));
			shmem->maxclaims[mymatrixcounter][8]=(rand()%(shmem->available[8]+1));
			shmem->maxclaims[mymatrixcounter][9]=(rand()%(shmem->available[9]+1));
			maxdone=true;
		}
		sem_post(&(shmem->mutexclock));

		int createdn=shmem->nanoseconds;
		locals=shmem->seconds;
		localn=shmem->nanoseconds;

		bool timereached=false;
		checktermination = rand()%200;
		bool term=false;

		do
		{
			if(shmem->numofiterations>=25)
			{
				million=10000000;
			}
			sem_wait(&(shmem->mutexclock));
			/*FILE *fp;
			fp = fopen("log.dat", "a+");
		    if (fp == NULL)
		        {
		        	perror("oss: Unable to open the output file:");
		        }
		    else
		    {
		    	fprintf(fp,"user %d has the semaphore\n",getpid());
		    //	fprintf(fp,"from user %d shmem %d local %d supposed to be %d\n",getpid(),shmem->nanoseconds,localn,B);
		    }
		    fclose(fp);*/
			//checking if the process is in blocked queue if yes then it will seed the semaphore
		    //Also checks if the process has initialized its maximum claims if not then it seeds the semaphore
			if(shmem->blockedqueue[mymatrixcounter][0]==getpid() || maxdone==false)
			{
				sem_post(&(shmem->mutexclock));
				sum = 0;
				for ( i = 0; i < million; i++)
				sum += i;
			}
			else
			{
				//printf("user %d has the semaphore\n",getpid());
				if(locals>=0 && localn>=20000)
				{
					term=true;
				}
				FILE *fp;
				fp = fopen("log.dat", "a+");
			    if (fp == NULL)
			        {
			        	perror("oss: Unable to open the output file:");
			        }
			    else
			    {
			    	//fprintf(fp,"from user %d shmem %d local %d supposed to be %d\n",getpid(),shmem->nanoseconds,localn,B);
			    }
			    fclose(fp);

				//printf("shmem nano %d localn %d\n ",shmem->nanoseconds,localn);
				if((locals==shmem->seconds && ((shmem->nanoseconds-localn) >= B)) || shmem->seconds>locals)
				{
					/*FILE *fp;
					fp = fopen("log.dat", "a+");
				    if (fp == NULL)
				        {
				        	perror("oss: Unable to open the output file:");
				        }
				    else
				    {
				    	fprintf(fp,"reached time \n");
				    }
				    fclose(fp);*/
					timereached=true;
					if(shmem->sharedmsg[0]==0)
					{
				    	//fprintf(fp,"user %d shared msg=0\n",getpid());
						/*FILE *fp;
						fp = fopen("log.dat", "a+");
					    if (fp == NULL)
					        {
					        	perror("oss: Unable to open the output file:");
					        }
					    else
					    {
					    	fprintf(fp,"message in shared\n");
					    }
					    fclose(fp);*/
						sharedmsgdone=true;
						int ct;
						if(locals==shmem->seconds)
						{
							ct=shmem->nanoseconds-createdn;
						}
						else
						{
							ct=10000;
						}
						if(term==true && ct > checktermination && numofrequests>=1)
						{
							/*FILE *fp;
							fp = fopen("log.dat", "a+");
						    if (fp == NULL)
						        {
						        	perror("oss: Unable to open the output file:");
						        }
						    else
						    {
						    	fprintf(fp,"reached term \n");
						    }
						    fclose(fp);*/
							int myend=rand()%3;
							if(myend==0)
							{
								shmem->sharedmsg[0]=getpid();
								shmem->sharedmsg[1]=-1;
								shmem->sharedmsg[2]=mymatrixcounter;
								Iamalive=false;
								sem_post(&(shmem->mutexclock));
								shmdt(shmem);
								exit(0);
							}
						}
						//if nothing has been allocated to the matrix so far
						if(resourceempty()==true && Iamalive==true)
						{
							//printf("nothing has been allocated so far for the process\n");
							//sem_wait(&(shmem->mutexclock));
							shmem->sharedmsg[0]=getpid();
							shmem->sharedmsg[1]=1;
							int temp[10], tempc=-1;
							/*
							 * if shmem->maxclaims is this
							 * 8,9,0,1,2,3,4,3,0,6
							 *
							 * temp[] is like this
							 * 0,1,3,4,5,6,7,8,9
							 * tempc=8
							 */
							for(i=0;i<10;i++)
							{
								if(shmem->maxclaims[mymatrixcounter][i]>0)
								{
									tempc++;
									temp[tempc]=i;
								}
							}
							int index=rand()%(tempc+1);
							shmem->sharedmsg[2]=temp[index];//selecting a random resource to request
							int temp1=shmem->maxclaims[mymatrixcounter][shmem->sharedmsg[2]];//selecting random amount of resource

							shmem->sharedmsg[3]=(rand()%temp1)+1;
							//printf("%d requesting resource %d amount %d\n",shmem->sharedmsg[0],shmem->sharedmsg[2],shmem->sharedmsg[3]);
						}
						else if(resourceempty()==false && Iamalive==true)
						{
							shmem->sharedmsg[0]=getpid();
							//since we don't want the probability to be 50%
							//if option =1 or 3 or 4 then we request
							//if option =2 we release
							int option=(rand()%3)+1;

							if(option ==1 || option==3 || numofrequests<=2)
							{
								shmem->sharedmsg[1]=1; //sharedmsg[1]=1 means request
							}
							//releases resources only if option 2 is chosen or only 2 processes are unblocked in the entire system
							else if(option ==2)
							{
								shmem->sharedmsg[1]=2; //sharedmsg[1]=2 means release
							}
							//if the user process has requested all resources in its max claims then it must release a resource and can't ask for more
							bool rall=requestedall();
							if(shmem->allblocked==1 || rall==true)
							{
								shmem->sharedmsg[1]=2;
							}

							//if request was chosen
							/*if shmem->left is like this
							 * 0,1,1,3,1,0,1,0,1,0
							 * left[] is like this
							 * left[0]=1,2,3,4,6,8
							 * left[1]=1,1,3,1,1,1
							 * leftcounter=6
							 */
							if(shmem->sharedmsg[1]==1)
							{
								int left[2][10]={0},leftcounter=-1;
								numofrequests++;
								for(i=0;i<10;i++)
								{
									if(shmem->left[mymatrixcounter][i]>0)
									{
										leftcounter++;
										left[leftcounter][0]=i;//resource left to be allocated (ex: r0,r1)
										left[leftcounter][1]=shmem->left[mymatrixcounter][i];//amount of that resource it can ask for
									}
								}

								int index=(rand()%(leftcounter+1));
								int resourcequantity=left[index][1];


							    shmem->sharedmsg[0]=getpid();

								shmem->sharedmsg[2]	=left[index][0];
								shmem->sharedmsg[3]=(rand()%resourcequantity)+1;
								FILE *fp;
								fp = fopen("log.dat", "a+");
							    if (fp == NULL)
							        {
							        	perror("oss: Unable to open the output file:");
							        }
							    else
							    {
							    	//fprintf(fp,"from user %d requesting index %d resource %d quantitiy %d\n",getpid(),index,left[index][0],shmem->sharedmsg[3]);
							    }
							    fclose(fp);

							}

							//if release was chosen
							else if(shmem->sharedmsg[1]==2)
							{
								//doing this to make it as random as possible
								//adding all allocated resources to array resources[]
								for(i=0;i<10;i++)
								{
									//means at index i of allocation matrix the value is >0 so we put it in resources[]
									if(shmem->allocation[mymatrixcounter][i]>0)
									{
										resourcecounter++;
										resources[resourcecounter]=i;
									}
								}
								/*
								 * if shmem->allocation matrix is like this:
								 * 0,1,2,3,4,5,6,7,8,9
								 * recources[] will have
								 * 1,2,3,4,5,6,7,8,9
								 * resourcecounter=8
								 */
								//randomly selecting one index from array resources[], this will have the resource number for release
								int resourcenumber=(rand()%(resourcecounter+1));
								shmem->sharedmsg[2]=resources[resourcenumber];
								int temp=shmem->allocation[mymatrixcounter][shmem->sharedmsg[2]];
								//randomly selecting the number of resources to release for the resource
								shmem->sharedmsg[3]=(rand()%temp)+1;
								shmem->sharedmsg[0]=getpid();
							}
						}
					}//end if shared msg=0
					else
					{
						/*FILE *fp;
						fp = fopen("log.dat", "a+");
					    if (fp == NULL)
					        {
					        	perror("oss: Unable to open the output file:");
					        }
					    else
					    {
					    	fprintf(fp,"shared msg not = 0 \n");
					    }
					    fclose(fp);*/
					}

				}//endif time reached
				else
				{
					/*FILE *fp;
					fp = fopen("log.dat", "a+");
				    if (fp == NULL)
				        {
				        	perror("oss: Unable to open the output file:");
				        }
				    else
				    {
				    	fprintf(fp,"did not reach time \n");
				    }
				    fclose(fp);*/
				}
				//we will update the local time only if it had a chance to go request/release
				//if we update anyway then everytime the user checked its time its local would be always be updated and this way the threshold B would never be met
				if(timereached==true&&sharedmsgdone==true)
				{
					locals=shmem->seconds;
					localn=shmem->nanoseconds;
					timereached=false;
					sharedmsgdone=false;
				}
				sem_post(&(shmem->mutexclock));
				sum = 0;
				for ( i = 0; i < million; i++)
				sum += i;
			}//end else
		}while(Iamalive);
		exit(0);

}


