/*
 ============================================================================
 Name        : simulator.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
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
# include <semaphore.h>
# include "shmemsem.h"
# include <stdbool.h>

//global variables
int billion=1000000000;
int emptypcbcounter;
int allocation[18][10],left[18][10],available[10];
int requestsgranted=0;
int requestsdenied=0;
int released=0;
int terminated=0;
pid_t alive[110];
int million=1000000;
SharedData *shmem;
int shmid;
FILE *fp;
int rann,rans;
char *logname="log.dat";//log file name for program -l
bool timeup=false;
bool filelines=true;


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

void checklines()
{
	FILE *fp;
    int count = 0;  // Line counter (result)

    char c;  // To store a character read from file


    // Open the file
    fp = fopen("log1.dat", "r");

    // Check if file exists
    if (fp == NULL)
    {
        printf("Could not open file ");

    }

    // Extract characters from file and store in character c
    for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n') // Increment count if this character is newline
            count = count + 1;
    if(count>=1000)
    	filelines=false;
    else
    	filelines= true;
}
void killprocesses()
{
	printf("\nkilling the following processes:\n");
	int i;
	for(i=0;i<18;i++)
	{
		printf("%d->%d\n",i,shmem->pcb[i][0]);
	}
	printf("OSS %d\n",getpid());

	int corpse,status;
	for(i=0;i<18;i++)
	{
		if(shmem->pcb[i][0]>0)
		{
			int pid=shmem->pcb[i][0];
			kill(pid,SIGKILL);
			while ((corpse = waitpid(pid, &status, 0)) != pid && corpse != -1)
 		   {
 			char pmsg[64];
 			snprintf(pmsg, sizeof(pmsg), "oss: PID %d exited with status 0x%.4X", corpse, status);
 			perror(pmsg);
 		   }
		}
	}

}
void cleanup()
{
	int bc=shmem->blockedcounter+1;
	int pc=shmem->pidcounter+1;
	if(timeup==true)
	{
		printf("time is up due to alarm\n");
	}
	printf("Doing cleanup\n");
	printf("Number of processes generated = %d\n",pc);
	printf("Number of: requests granted =%d requests denied=%d processes that released resources=%d processes that terminated=%d \nprocesses in blocked queue before termination=%d\n",requestsgranted,requestsdenied,released,terminated,bc);
	printf("Number of iterations performed by OSS(request/release/terminate): %d \nOSS Exiting at time %d:%d\n",shmem->numofiterations,shmem->seconds,shmem->nanoseconds);
	fp = fopen(logname, "a+");
    if (fp == NULL)
    {
    	perror("oss: Unable to open the output file:");
    }
    else
    {
    	fprintf(fp,"Number of processes generated=%d\n",pc);
    	fprintf(fp,"Number of: requests granted =%d requests denied=%d processes that released resources=%d processes that terminated=%d \nprocesses in blocked queue before termination=%d\n",requestsgranted,requestsdenied,released,terminated,bc);
    	fprintf(fp,"Number of iterations performed by OSS(request/release/terminate): %d \nOSS Exiting at time %d:%d\n",shmem->numofiterations,shmem->seconds,shmem->nanoseconds);
    }
    fclose(fp);

	killprocesses();
	shmdt(shmem);// Free up shared memory
	shmctl(shmid, IPC_RMID,NULL);

	exit(0);
}
void updatePCB(int process)
{
	int i;
	for(i=0;i<18;i++)
	{
		if(shmem->pcb[i][0]==process)
		{
			shmem->pcb[i][0]=0;
			shmem->pcb[i][1]=0;
			shmem->pcb[i][2]=0;
			break;
		}
	}
}
//clearing matrix for process that has terminated
void clearmatrix(int counter)
{
	int i;

	//going through all the resources for the process
	for(i=0;i<10;i++)
	{
		if(shmem->allocation[counter][i]>0)
		{
			//since resource r3-r9 are not shared
			if(i>=3)
			{
				shmem->available[i]=shmem->available[i]+shmem->allocation[counter][i];
				shmem->allocation[counter][i]=0;
			}
			else//resource r0,r1,r2
			{
				shmem->allocation[counter][i]=0;
			}
		}
		shmem->maxclaims[counter][i]=0;
		shmem->left[counter][i]=0;
	}
}
void printmatrixfile()
{

	fp = fopen(logname, "a+");
    if (fp == NULL)
    {
    	perror("oss: Unable to open the output file:");
    }
    else
    {
    	int i,j;
    	fprintf(fp,"\nmaxclaims\n");
    	//fprintf(fp,"0 1 2 3 4 5 6 7 8 9 <-resource number\n");

    	for(i=0;i<18;i++)
    	{
    		if(shmem->pcb[i][0]>0)
    		{
    			for(j=0;j<10;j++)
    	    	{
    	    		fprintf(fp,"%d ",shmem->maxclaims[i][j]);
    	    	}
    	    	fprintf(fp,"<-%d",shmem->pcb[i][0]);
    	    	fprintf(fp,"\n");
    		}
    	}
    	fprintf(fp,"\nallocation\n");

    	for(i=0;i<18;i++)
    	{
    		if(shmem->pcb[i][0]>0)
    	    {
    			for(j=0;j<10;j++)
    	    	{
    	    			fprintf(fp,"%d ",shmem->allocation[i][j]);
    	    			//printf("%d ",shmem->allocation[i][j]);
    	    	}
    	    	fprintf(fp,"<-%d",shmem->pcb[i][0]);
    	    	fprintf(fp,"\n");
    	    }
    	}
    	fprintf(fp,"\nleft\n");

    	for(i=0;i<18;i++)
    	{
    		if(shmem->pcb[i][0]>0)
    		{
    			for(j=0;j<10;j++)
    			{
    				fprintf(fp,"%d ",shmem->left[i][j]);
    			}
    			fprintf(fp,"<-%d",shmem->pcb[i][0]);
    			fprintf(fp,"\n");
    		}
    	}
    	fprintf(fp,"\navailable\n");
    	for(i=0;i<10;i++)
    	{
    		fprintf(fp,"%d ",shmem->available[i]);
    	}
    	fprintf(fp,"\n");

    	if(shmem->blockedcounter>-1)
        {
    		fprintf(fp,"\nBlocked queue\n");
        	for(i=0;i<18;i++)
        	{
        		if(shmem->blockedqueue[i][0]>0)
        		{
        			fprintf(fp,"%d %d %d <- %d\n",shmem->blockedqueue[i][0],shmem->blockedqueue[i][1],shmem->blockedqueue[i][2],i);
        		}
        	}
        }
    }

    fclose(fp);
}
void printmatrix()
{
	int i,j;
	printf("\nmaxclaims\n");
	for(i=0;i<18;i++)
	{
		printf("%d->",shmem->processnum[i]);
		for(j=0;j<10;j++)
		{
			printf("%d ",shmem->maxclaims[i][j]);
		}
		printf("\n");
	}
	printf("\nallocation\n");
	for(i=0;i<18;i++)
	{
		printf("%d->",shmem->processnum[i]);
		for(j=0;j<10;j++)
		{
			printf("%d ",shmem->allocation[i][j]);
		}
		printf("\n");
	}
	printf("\nleft\n");
	for(i=0;i<18;i++)
	{
		printf("%d->",shmem->processnum[i]);
		for(j=0;j<10;j++)
		{
			printf("%d ",shmem->left[i][j]);
		}
		printf("\n");
	}
	printf("\navailable\n");
	for(i=0;i<10;i++)
	{
			printf("%d ",shmem->available[i]);
	}
	printf("\n");

}
void printsimulatedmatrix()
{
	int i,j;
	printf("\n simulated allocated\n");
	for(i=0;i<18;i++)
	{
		printf("%d->",shmem->processnum[i]);
		for(j=0;j<10;j++)
		{

			printf("%d ",allocation[i][j]);
		}
		printf("\n");
	}
	printf("\n simulated left\n");
	for(i=0;i<18;i++)
	{
		printf("%d->",shmem->processnum[i]);
		for(j=0;j<10;j++)
		{

			printf("%d ",left[i][j]);
		}
		printf("\n");
	}
	printf("\n simulated available\n");
	printf("       ");
	for(i=0;i<10;i++)
	{
			printf("%d ",available[i]);
	}
	printf("\n");

}
void printpcb()
{
	printf("\ncontents of PCB:\n");
	int i;
	for(i=0;i<18;i++)
	{
		if(shmem->pcb[i][0]>0)
		{
			printf("%d-> %d %d %d \n",i,shmem->pcb[i][0],shmem->pcb[i][1],shmem->pcb[i][2]);
		}
	}
}
void catch_alarm (int sig)//SIGALRM handler
{
	timeup=true;
	cleanup();
}
int emptyPCB()
{
	int i;
	for(i=0;i<18;i++)
	{
		if(shmem->pcb[i][0]==0)
		{
			//printf("%d is empty\n",i);
			return (i);
		}
	}
	//printf("No empty spot in PCB\n");
	return (-1);
}
bool algo(int allocation [18][10],int left[18][10],int r,int amount,int c, int available[10])
{
	int i,j,processleft[18]={0},count=18,processleftc=0,pl[18]={0},plc=0;
	int doiterations=-1,initial=-1;
	do
	{
		doiterations++;
		if(doiterations==0)
		{
			//printf("doiterations=0\n");
			for(i=0;i<18;i++)
			{
				if(left[i][0]<=available[0] && left[i][1]<=available[1] && left[i][2]<=available[2]
				   &&  left[i][3]<=available[3]&&  left[i][4]<=available[4]&&  left[i][5]<=available[5]
				  &&  left[i][6]<=available[6]&&  left[i][7]<=available[7]&&  left[i][8]<=available[8]
				   &&  left[i][9]<=available[9])
				{
					available[0]=available[0]+allocation[i][0];
					allocation[i][0]=0;
					left[i][0]=0;
					available[1]=available[1]+allocation[i][1];
					allocation[i][1]=0;
					left[i][1]=0;
					available[2]=available[2]+allocation[i][2];
					allocation[i][2]=0;
					left[i][2]=0;
					available[3]=available[3]+allocation[i][3];
					allocation[i][3]=0;
					left[i][3]=0;
					available[4]=available[4]+allocation[i][4];
					allocation[i][4]=0;
					left[i][4]=0;
					available[5]=available[5]+allocation[i][5];
					allocation[i][5]=0;
					left[i][5]=0;
					available[6]=available[6]+allocation[i][6];
					allocation[i][6]=0;
					left[i][6]=0;
					available[7]=available[7]+allocation[i][7];
					allocation[i][7]=0;
					left[i][7]=0;
					available[8]=available[8]+allocation[i][8];
					allocation[i][8]=0;
					left[i][8]=0;
					available[9]=available[9]+allocation[i][9];
					allocation[i][9]=0;
					left[i][9]=0;
					count--;
					//printf("finished process %d count= %d \n",i ,count);
				}
				else
				{
					processleft[processleftc]=i;
					processleftc++;
					//printf("process %d cannot be finished right now\n",i);
				}
			}//endfor
			for(i=0;i<processleftc;i++)
			{
				pl[plc]=processleft[i];
				processleft[i]=0;
				plc++;
			}
			processleftc=0;
		}//endif
		else
		{
			//printf("doiterations else\n");
			//printf("plc=%d\n",plc);
			initial=plc;
			plc=0;
			for(j=0;j<initial;j++)
			{
				int i=pl[j];
				if(left[i][0]<=available[0] && left[i][1]<=available[1] && left[i][2]<=available[2]
				     &&  left[i][3]<=available[3]&&  left[i][4]<=available[4]&&  left[i][5]<=available[5]
				    &&  left[i][6]<=available[6]&&  left[i][7]<=available[7]&&  left[i][8]<=available[8]
				      &&  left[i][9]<=available[9])
				 {
					available[0]=available[0]+allocation[i][0];
					allocation[i][0]=0;
					left[i][0]=0;
					available[1]=available[1]+allocation[i][1];
					allocation[i][1]=0;
					left[i][1]=0;
					available[2]=available[2]+allocation[i][2];
					allocation[i][2]=0;
					left[i][2]=0;
					available[3]=available[3]+allocation[i][3];
					allocation[i][3]=0;
					left[i][3]=0;
					available[4]=available[4]+allocation[i][4];
					allocation[i][4]=0;
					left[i][4]=0;
					available[5]=available[5]+allocation[i][5];
					allocation[i][5]=0;
					left[i][5]=0;
					available[6]=available[6]+allocation[i][6];
					allocation[i][6]=0;
					left[i][6]=0;
					available[7]=available[7]+allocation[i][7];
					allocation[i][7]=0;
					left[i][7]=0;
					available[8]=available[8]+allocation[i][8];
					allocation[i][8]=0;
					left[i][8]=0;
					available[9]=available[9]+allocation[i][9];
					allocation[i][9]=0;
					left[i][9]=0;
					count--;
					//printf("finished process %d count= %d \n",i ,count);
				}
				else
				{
					processleft[processleftc]=i;
					processleftc++;
					//printf("process %d cannot be finished right now\n",i);
				}
			}//endfor
			if(initial==processleftc)
			{
				printf("processes that could not be finished\n");

				int k;
				for(k=0;k<processleftc;k++)
				{
					printf("%d ",processleft[k]);
				}
				printf("\n");
				return false;
			}
			else if (initial!=processleftc)
			{
				for(i=0;i<processleftc;i++)
				{
					pl[plc]=processleft[i];
					processleft[i]=0;
					plc++;
				}
				processleftc=0;
			}

		}//endelse
	}while(count>0);
	if(count==0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
int processnumber(int pid)
{
	int i;
	for(i=0;i<18;i++)
	{
		if(shmem->pcb[i][0]==pid)
		{
			return i;
		}
	}
		return -1;
}
void calcleft()
{
	//printf("calcleft\n\n");
	int i,j;
	for(i=0;i<18;i++)
	{
		for(j=0;j<10;j++)
		{
			//printf("left[%d][%d]=maxclaims[%d][%d]-allocation[%d][%d]\n",i,j,i,j,i,j);
			//printf("%d=%d-%d\n",shmem->left[i][j],shmem->maxclaims[i][j],shmem->allocation[i][j]);
			shmem->left[i][j]=shmem->maxclaims[i][j]-shmem->allocation[i][j];
		}
	}

}

bool avoidance(int r, int amount,int currentprocess)
{
	int i,j;

	calcleft();

	//if requested is more than available
	//printf("shmem->available[%d]=%d amount=%d \n",r,shmem->available[r],amount);
	if(shmem->available[r]-amount<0)
		{
			//printf("%d %d ",available[r],amount);
			printf("user %d request is more than available\n",currentprocess);
			fp = fopen(logname, "a+");
		    if (fp == NULL)
		    {
		    	perror("oss: Unable to open the output file:");
		    }
		    else
		    {
		    	fprintf(fp,"user %d request is more than available\n",currentprocess);
		    }
		    fclose(fp);
			return false;
		}
	else
	{
		//new matrices for simulated calculation
		for(i=0;i<10;i++)
		{
			available[i]=shmem->available[i];
		}
		for(i=0;i<18;i++)
		{
			for(j=0;j<10;j++)
			{
				allocation[i][j]=shmem->allocation[i][j];
				left[i][j]=shmem->left[i][j];

			}
		}
		int c=processnumber(currentprocess);
		if(c==-1)
		{
			printf("the process pid was not found in the list\n");
			cleanup();
		}
		available[r]=available[r]-amount;

		allocation[c][r]=allocation[c][r]+amount;
		left[c][r]=left[c][r]-amount;
		//printf("r= %d amount = %d c = %d\n",r,amount,c);
		//printf("this is what the matrices should look like if the request is granted:\n");
		//printsimulatedmatrix();
		bool p=algo(allocation,left,r,amount,c,available);
		if(p==true)
		{
			shmem->allocation[c][r]+=amount;
			shmem->available[r]-=amount;
			shmem->left[c][r]-=amount;
			//printmatrixfile();
			return true;
		}
		else
		{
			fp = fopen(logname, "a+");
		    if (fp == NULL)
		    {
		    	perror("oss: Unable to open the output file:");
		    }
		    else
		    {
		    	fprintf(fp,"Granting %d request may lead to deadlock\n",currentprocess);
		    }
		    fclose(fp);
			return false;
		}
	}
}

void checkblocked()
{
	int i;
	for(i=0;i<18;i++)
	{
		if(shmem->blockedqueue[i][0]>0)
		{
			int pid=shmem->blockedqueue[i][0];
			int resourcenum=shmem->blockedqueue[i][1];
			int resourceamount=shmem->blockedqueue[i][2];
			if(resourceamount<=shmem->available[resourcenum])
			{
				bool releaseornot=avoidance(resourcenum,resourceamount,pid);
				if(releaseornot==true)
				{
					printf("OSS removing process %d from blocked queue and granting its request for resource %d in the amount %d\n",pid,resourcenum,resourceamount);
					fp = fopen(logname, "a+");
					if (fp == NULL)
					{
						perror("oss: Unable to open the output file:");
					}
					else
					{
						fprintf(fp,"OSS removing process %d from blocked queue and granting its request for resource %d in the amount %d i=%d\n",pid,resourcenum,resourceamount,i);
					}
					shmem->blockedqueue[i][0]=0;
					shmem->blockedqueue[i][1]=0;
					shmem->blockedqueue[i][2]=0;
					shmem->blockedcounter--;
					fclose(fp);
					printmatrixfile();
				}
				else if(releaseornot==false)
				{
					printf("OSS cannot remove process %d from blocked queue because it may cause a deadlock\n",pid);

					fp = fopen(logname, "a+");
				    if (fp == NULL)
				    {
				    	perror("oss: Unable to open the output file:");
				    }
				    else
				    {
				    	fprintf(fp,"OSS cannot remove process %d from blocked queue\n",pid);
				    }
				    fclose(fp);

				}

			}
			else
			{
				printf("OSS cannot remove process %d from blocked queue since its request for resource %d in amount>available %d>%d i=%d\n",pid,resourcenum,resourceamount,shmem->available[resourcenum],i);

				fp = fopen(logname, "a+");
			    if (fp == NULL)
			    {
			    	perror("oss: Unable to open the output file:");
			    }
			    else
			    {
			    	fprintf(fp,"OSS cannot remove process %d from blocked queue since its request for resource %d in amount>available %d>%d i=%d\n",pid,resourcenum,resourceamount,shmem->available[resourcenum],i);
			    }
			    fclose(fp);

			}

		}
	}
}
//this function checks if all the processes in the system at the time are blocked if yes it make processes release resources.
void checknumofblocked()
{
	int i,nblocked=0,nfree=0;

	for(i=0;i<18;i++)
	{
		if(shmem->pcb[i][0]>0)
		{
			if(shmem->blockedqueue[i][0]==shmem->pcb[i][0])//is the process in the blocked queue
			{
				nblocked++;
			}
			else//the process is not blocked, it this is true even for one process then we don't set shmem->allblocked
			{
				nfree++;
			}
		}
	}
	//if only 1 processes is free and rest are blocked then we start releasing resources
	if(nfree==1 && shmem->processnumcounter>=2)
	{
		shmem->allblocked=1;
	}
	else
	{
		shmem->allblocked=0;
	}

	fp = fopen(logname, "a+");
    if (fp == NULL)
    {
    	perror("oss: Unable to open the output file:");
    }
    else
    {
    //	fprintf(fp,"blocked =%d free= %d shmem->allblocked=%d\n",nblocked,nfree,shmem->allblocked);
    }
    fclose(fp);

}
void startprocess()
{
	signal (SIGALRM, catch_alarm);
    alarm (10);
	int i,sum,err;
	while(1)
	{
		if(shmem->numofiterations>=25)
		{
			million=100000000;
		}

		if((err=sem_wait(&(shmem->mutexclock)))==-1)
		{
			perror("OSS %d error in semwait clock");
			cleanup();
		}
		/*fp = fopen(logname, "a+");
	    if (fp == NULL)
	    {
	    	perror("oss: Unable to open the output file:");
	    }
	    else
	    {
	    	fprintf(fp,"oss has semaphore \n");
	    }
	    fclose(fp);*/
		while(shmem->nanoseconds<=rann)
		{
			shmem->nanoseconds=shmem->nanoseconds+1000; //(500 for overhead + 500 for actual clock )
			if(shmem->nanoseconds>=billion)
			{
				shmem->seconds=shmem->seconds+1;
				shmem->nanoseconds=0;
			}
		}

		rann = shmem->nanoseconds+(rand() % 500);
		rans=shmem->seconds;

		if((emptypcbcounter=emptyPCB())>=0)
		{
			//printf("empty pcb counter =%d\n",emptypcbcounter);

			pid_t pID = fork();

			if (pID < 0)
			{
				perror("oss: Failed to fork:");
				//exit(EXIT_FAILURE);
			}

			else if (pID == 0)
			{
				static char *args[]={"./user",NULL};
				int status;
				if(( status= (execv(args[0], args)))==-1)
				{
					perror("oss:failed to execv");
					//exit(EXIT_FAILURE);
				}
			}

			if(pID>0)
			{
				shmem->processnum[emptypcbcounter]=pID;
				shmem->pcb[emptypcbcounter][0]=pID;
				shmem->pcb[emptypcbcounter][1]=shmem->seconds;
				shmem->pcb[emptypcbcounter][2]=shmem->nanoseconds;
				//printpcb();
				printf("created process %d at time %d:%d at PCB position %d \n",pID,shmem->seconds,shmem->nanoseconds,emptypcbcounter);
				if(shmem->processnumcounter<18)
				{
					shmem->processnumcounter++;//used to keep track of how many total processes have started so far.
				}
				//printpcb();
				FILE *fp;
				fp = fopen("log.dat", "a+");
			    if (fp == NULL)
			        {
			        	perror("oss: Unable to open the output file:");
			        }
			    else
			    {
			    	fprintf(fp,"created process %d at time %d:%d at PCB position %d \n",pID,shmem->seconds,shmem->nanoseconds,emptypcbcounter);
			    }
			    fclose(fp);
			}
		}


		if(shmem->sharedmsg[0]!=0)
		{
			shmem->numofiterations++;
			calcleft();
			if(shmem->sharedmsg[1]==-1)
			{
				printf("\n process %d wants to terminate at time %d %d\n",shmem->sharedmsg[0],shmem->seconds,shmem->nanoseconds);

				fp = fopen(logname, "a+");
	            if (fp == NULL)
	            {
	               	perror("oss: Unable to open the output file:");
	            }
	            else
                {
                	fprintf(fp,"\n process %d wants to terminate at time %d %d\n",shmem->sharedmsg[0],shmem->seconds,shmem->nanoseconds);
                }

	            fclose(fp);
	            terminated++;
	            int pidforwait=shmem->sharedmsg[0];
                int corpse,status;
	            while ((corpse = waitpid(pidforwait, &status, 0)) != pidforwait && corpse != -1)
	    		   {
	    			char pmsg[64];
	    			snprintf(pmsg, sizeof(pmsg), "oss: PID %d exited with status 0x%.4X", corpse, status);
	    			perror(pmsg);
	    		   }
	            //clearing PCB
	            updatePCB(pidforwait);
	            //clearing matrices
	            clearmatrix(shmem->sharedmsg[2]);
                shmem->sharedmsg[0]=0;
                shmem->sharedmsg[1]=0;
                shmem->sharedmsg[2]=0;
			}
			else if(shmem->sharedmsg[1]==1)
			{
				printf("\n process %d is requesting resource %d in the amount %d at time %d %d\n",shmem->sharedmsg[0], shmem->sharedmsg[2], shmem->sharedmsg[3],shmem->seconds,shmem->nanoseconds);

				fp = fopen(logname, "a+");
                if (fp == NULL)
                {
                	perror("oss: Unable to open the output file:");
                }
                else
                {
                	//printf("\nentering data in file");
                	fprintf(fp,"\nprocess %d is requesting resource %d in the amount %d at time %d %d\n",shmem->sharedmsg[0], shmem->sharedmsg[2], shmem->sharedmsg[3],shmem->seconds,shmem->nanoseconds);
                }
                fclose(fp);
                int currentprocess=shmem->sharedmsg[0];
                int requestingresource=shmem->sharedmsg[2];
                int requestingamount=shmem->sharedmsg[3];
                int currentcounter=processnumber(currentprocess);

                //if the process is requesting a shared resource
                if(requestingresource==0 || requestingresource==1 || requestingresource==2)
                {
                	shmem->allocation[currentcounter][requestingresource]+=requestingamount;
                	shmem->left[currentcounter][requestingresource]-=requestingamount;
                	printf("the resource can be granted\n");

             	    fp = fopen(logname, "a+");
                    if (fp == NULL)
                    {
                    	perror("oss: Unable to open the output file:");
                    }
                    else
                    {
                    	//printf("\nentering data in file");
                    	fprintf(fp,"OSS granted the request\n");
                    }
                    fclose(fp);
                    requestsgranted++;
                    //printmatrixfile();
                }
                else
                {
                	bool result= avoidance(requestingresource, requestingamount,currentprocess);
                    if(result==true)
                    {
                 	   printf("the resource can be granted\n");

                 	   fp = fopen(logname, "a+");
                       if (fp == NULL)
                       {
                    	   perror("oss: Unable to open the output file:");
                       }
                       else
                       {
                        	//printf("\nentering data in file");
                    	   fprintf(fp,"OSS granted the request\n");
                       }
                       fclose(fp);
                       if(shmem->numofiterations%2==0)//comment this out if you want to see matrices after each entry
                       {
                    	   printmatrixfile();
                       }
                       requestsgranted++;
                    }
                    else
                    {
                 	  // printf("the resource cannot be granted\n");

                        printf("OSS cannot grant the request at this time, Putting %d in blocked queue at time %d:%d\n",currentprocess,shmem->seconds,shmem->nanoseconds);

                        shmem->blockedcounter++;
                        int c=processnumber(currentprocess);
                        shmem->blockedqueue[c][0]=currentprocess;
                        shmem->blockedqueue[c][1]=requestingresource;
                        shmem->blockedqueue[c][2]=requestingamount;

                 	   fp = fopen(logname, "a+");
                        if (fp == NULL)
                        {
                        	perror("oss: Unable to open the output file:");
                        }
                        else
                        {
                        	//printf("\nentering data in file");
                        	fprintf(fp,"OSS cannot grant the request at this time, Putting %d in blocked queue at time %d:%d\n",currentprocess,shmem->seconds,shmem->nanoseconds);
                        }
                        fclose(fp);
                        printmatrixfile();
                        requestsdenied++;
                    }
                }
                shmem->sharedmsg[0]=0;
                shmem->sharedmsg[1]=0;
                shmem->sharedmsg[2]=0;
                shmem->sharedmsg[3]=0;
			}
			else if(shmem->sharedmsg[1]==2)
			{
				printf("\n process %d is releasing resource %d in the amount %d at time %d %d\n",shmem->sharedmsg[0], shmem->sharedmsg[2], shmem->sharedmsg[3],shmem->seconds,shmem->nanoseconds);

				fp = fopen(logname, "a+");
	            if (fp == NULL)
                {
                	perror("oss: Unable to open the output file:");
                }
                else
                {
                	//printf("\nentering data in file");
                	fprintf(fp,"\n process %d is releasing resource %d in the amount %d at time %d %d\n",shmem->sharedmsg[0], shmem->sharedmsg[2], shmem->sharedmsg[3],shmem->seconds,shmem->nanoseconds);
                }
                fclose(fp);

                released++;
                int resourcenum=shmem->sharedmsg[2];
                int resourceamount=shmem->sharedmsg[3];
                if(resourcenum==0 || resourcenum==1 ||resourcenum==2)
                {
                	int c=processnumber(shmem->sharedmsg[0]);
                    shmem->allocation[c][resourcenum]-=resourceamount;
                    calcleft();
                   // printmatrixfile();
                }
                else
                {
                	shmem->available[resourcenum]+=resourceamount;
                    int c=processnumber(shmem->sharedmsg[0]);
                    shmem->allocation[c][resourcenum]-=resourceamount;
                    calcleft();
                    printmatrixfile();
                    //checking to see if we can grant any requests in the blocked queue
                    if(shmem->blockedcounter>-1)
                    {
                    	checkblocked();
                    }
                }

                shmem->sharedmsg[0]=0;
                shmem->sharedmsg[1]=0;
                shmem->sharedmsg[2]=0;
                shmem->sharedmsg[3]=0;
			}//end else if
			calcleft();
		}//end ifsharedmsg=0
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
		    	fprintf(fp,"shared msg==0\n");
		    }
		    fclose(fp);*/
		}
		checknumofblocked();
		sem_post(&(shmem->mutexclock));
		//delay
		sum=0;
	    for (i = 0; i < million; i++)
	    sum += i;
	}//end while
}

void main()
{
	int i,j;

	// Create our key
	key_t shmkey = ftok("makefile",777);
	if (shmkey == -1) {
		perror("oss:Ftok failed");
		exit(-1);
	}

	// Get our shm id
	shmid = shmget(shmkey, sizeof(SharedData), 0666 | IPC_CREAT);
	if (shmid == -1) {
		perror("oss:Failed to allocate shared memory region");
		exit(-1);
	}

	// Attach to our shared memory
	shmem = (SharedData*)shmat(shmid, (void *)0, 0);
	if (shmem == (void *)-1) {
		perror("oss:Failed to attach to shared memory region");
		exit(-1);
	}
	for(i=0;i<18;i++)
	{
		for(j=0;j<3;j++)
		{
			shmem->pcb[i][j]=0;

		}
	}

	for(i=0;i<18;i++)
	{
		shmem->processnum[i]=0;
		for(j=0;j<10;j++)
		{
			shmem->allocation[i][j]=0;
			shmem->maxclaims[i][j]=0;
		}
	}
	shmem->pidcounter=-1;
	emptypcbcounter=0;
	sem_init(&(shmem->mutexclock),1,1);
	shmem->nanoseconds=0;
	shmem->seconds=0;
	shmem->sharedmsg[0]=0;//stores current pid
	shmem->sharedmsg[1]=0;//request or release
	shmem->sharedmsg[2]=0;//resource number
	shmem->sharedmsg[3]=0;//quantity
	for(i=0;i<10;i++)
	{
		int selection = rand()%3;
		if(selection==0)
		{
			shmem->available[i]=10;
		}
		else if(selection==1)
		{
			shmem->available[i]=10;
		}
		else
		{
			shmem->available[i]=10;
		}
	}

	fp = fopen(logname, "w+");
    if (fp == NULL)
        {
        	perror("oss: Unable to open the output file:");
        }
    else
    {
    	fprintf(fp,"Initial Available resources (0-9)\n");
    	for(i=0;i<10;i++)
    	{
    		fprintf(fp,"%d ",shmem->available[i]);
    	}
    	fprintf(fp,"\n");
    }
    fclose(fp);
    shmem->processnumcounter=0;
    for(i=0;i<18;i++)
    {
    	for(j=0;j<3;j++)
    	{
    		shmem->blockedqueue[i][j]=0;
    	}
    }
    shmem->blockedcounter=-1;
    shmem->allblocked=0;
    shmem->numofiterations=0;
	rann = (rand() % 500);
	rans=shmem->seconds;
	srand(time(NULL) ^ (getpid()<<16));
	startprocess();
}
