// Programmer: Andrew McCormick 
// Function:   main.c
// Class:      CS-4760 PA 1
// Date:       2/7/2019

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include "optArg.h"
#include "sharedTime.h"
#include "detachAndRemove.h"
#define PERM (S_IRUSR | S_IWUSR) 

static SharedTime *sharedSum;
static const char *optString = "ho:i:n:s:";

int main(int argc, char * argv[])
{
	FILE *readptr;
	FILE *writeptr;

	int opt = 0;
	int shmID = 0;
	int parentCounter = 0;
	int status;
	int actChild = 0;
	int getNewLaunch = 0;
	int secLaunch = 0;
	int terminate = 0;
	int numChild = 0;
		
	double timeInc = 0.0;
	double nanoLaunch = 0.0;
	char *duration;

	char fileInput[100];
	char *end;

	pid_t childpid;
	pid_t testpid;

	OptArg args = {"input.txt", "output.txt", 4, 2};

	opt = getopt(argc, argv, optString);

	while(opt != -1)
	{
		switch(opt)
		{
			// for option -o outputfile
			case 'o':
				args.outputFileName = optarg;
				break;

			// for option -i inputfile
			case 'i':
				args.inputFileName = optarg;
				break;
					
			// for option -n number of child processes
			case 'n':
				args.numChild = atoi(optarg);
				break;
			
			// for option -s number of childern running at a time
			case 's':
				args.childAtTime = atoi(optarg);
				break;

			case 'h':
				printf("Program Defaults:\n-i: input.txt,\n-o output.txt,\n-n: 4 total child processes,\n-s: Max of 2 childs running at a time\n");
				return EXIT_SUCCESS;
		}
		
		opt = getopt(argc, argv, optString);
	}

	// Opening input file and error checking
	if((readptr = fopen(args.inputFileName, "r")) == NULL)
	{
		perror("oss: Error");
		return EXIT_FAILURE;
	}

	if((writeptr = fopen(args.outputFileName, "w")) == NULL)
	{
		perror("oss: Error");
		return EXIT_FAILURE;
	}

/*************************************************************
 *
 * 	              Creating Key 
 *
 *************************************************************/ 	              

	key_t key = ftok("main.c", 50);
	if(key == -1)
	{
		perror("Failed to derive key:");
		return EXIT_FAILURE;
	}

/************************************************************
 *
 * 		Creating shared Memory
 *
 ************************************************************/ 

	shmID = shmget(key, sizeof(SharedTime), 0666 | IPC_CREAT);
	
	if((shmID == -1) && (errno != EEXIST))
	{
		return EXIT_FAILURE;
	}

	if(shmID == -1)
	{
		if(((shmID = shmget(key, sizeof(SharedTime), PERM)) == -1) || ((sharedSum = (SharedTime *)shmat(shmID, NULL, 0)) == (void *)-1))
		{
			return EXIT_FAILURE;
		}
	}
		
	else
	{
		sharedSum = (SharedTime *)shmat(shmID, NULL, 0);
		
		if(sharedSum == (void *)-1)
		{
			return EXIT_FAILURE;
		}
		
		sharedSum -> seconds = 0;
		sharedSum -> nanoSecs = 0;
	}

/*****************************************************************
 *
 * 		Creating Fork and Exec
 *
 *****************************************************************/
		
	 // Pulling first line from input file and checking if not data exists
	if(fgets(fileInput, 100, readptr) == NULL)
	{
	        perror("logParse: Error: Line 1 in the input file is empty");
		return EXIT_FAILURE;
	}

	timeInc = (double) strtod(fileInput, NULL); // Converting char* into integer

	if(fgets(fileInput, 100, readptr) == NULL)
	{
		perror("oss: Error: Line 2 in the input file is empty");
		return EXIT_FAILURE;
	}

	secLaunch = (int) strtol(fileInput, &end, 10);
	nanoLaunch = (double) strtod(end, &end);
	duration = strtok(end, " \n\t");
	numChild = args.numChild;

	while(terminate < args.numChild)
	{
		if((timeInc + sharedSum -> nanoSecs) >= 1000000000.0)
		{
			sharedSum -> seconds += 1;
			sharedSum -> nanoSecs = 0.0;
		}
		else
		{
			sharedSum -> nanoSecs += timeInc;
		}
			
		if(numChild > 0)
		{
			if(actChild < args.childAtTime)
			{
				if(secLaunch == (sharedSum -> seconds))
				{	
					if(nanoLaunch <=  (sharedSum -> nanoSecs))
					{ 
						if((childpid = fork()) == 0)
						{		
							execl("./user", duration, NULL);
							perror("exec Failed:");
							return EXIT_FAILURE;
						}	
					
						fprintf(writeptr, "Child Process: %d    Started at: %d Seconds, %f NanoSeconds\n", childpid, sharedSum -> seconds, sharedSum -> nanoSecs);

						getNewLaunch = 1;
						numChild--;
						actChild++;
					}
				}
			
				else if(secLaunch < sharedSum -> seconds)
				{
					if((childpid = fork()) == 0)
					{
						execl("./user", "hello", NULL);
						perror("exec Failed:");
						return EXIT_FAILURE;
					}
				
					getNewLaunch = 1;
					numChild--;
					actChild++; 
				}

				if(getNewLaunch == 1 && numChild > 0)
				{
        				if(fgets(fileInput, 100, readptr) == NULL)
        				{			
                				perror("oss: Error: Line 2 in the input file is empty");
                				return EXIT_FAILURE;
        				}

              				secLaunch = (int) strtol(fileInput, &end, 10);
					nanoLaunch = (double) strtod(end, &end);
					duration = strtok(end, " \n\t");		
	
					getNewLaunch = 0;
				}
			}
		}
		
		testpid = waitpid(-1, &status, WNOHANG);
		
		if(testpid > 0)
		{
			fprintf(writeptr, "Child Process: %d Terminated at: %d Seconds, %f Nanoseconds\n", testpid, sharedSum -> seconds, sharedSum -> nanoSecs);
			terminate++;
			actChild--;
		}
	}

/*****************************************************************
 *
 * 		Detaching Shared Memory
 *
 *****************************************************************/ 		

	if(detachAndRemove(shmID, sharedSum) == -1)
	{
		perror("Failed to destory shared memory segment");
		return EXIT_FAILURE;
	}
	
	fclose(readptr);
	fclose(writeptr);
	return EXIT_SUCCESS;
}