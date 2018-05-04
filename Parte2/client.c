#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define WIDTH_PID 5
#define WIDTH_XXNN 4
#define WIDTH_SEAT 5

int requestsFd; // requests file desciptor

//calculates length from int
int numDigits (int number)
{
	int res = 0;

	while (number > 10)
	{
		number /= 10;
		res++;
	}

	res ++;

	return res;
}

void makeAnswerFifo()
{
	char fifoName [8];

	sprintf (fifoName, "ans%d", getpid());

	if (mkfifo(fifoName, 0660) == -1) //creates fifo 'requests'
	{
		perror("ERROR");
		exit(2);
	}
}

void openRequestsFifo ()
{
    requestsFd=open("requests" ,O_WRONLY);
    if (requestsFd == -1)
    {
    	perror ("Error");
    	exit (3);
    }
}

int main (int argc, char *argv[])
{
	double openTime;
	struct timeval startTime, currentTime;
	char request [500]; //request

	if (argc < 4)
	{
		printf ("Usage: client <time_out> <num_wanted_seats> <pref_seat_list> \n");
		exit (1);
	}

	openTime = atof(argv[1]);

//	makeAnswerFifo();

//	openRequestsFifo ();

	//builds request
	sprintf (request, "%d %d ", getpid(), atoi (argv[1]));

    int i;
    for (i = 0; i < argc - 3; i++)
    {
    	strcat (request, argv [i+3]);
    	strcat (request, " ");
    }


	gettimeofday(&startTime,NULL);

	gettimeofday(&currentTime,NULL);

	double diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);

	while (diff < openTime)
	{
		gettimeofday(&currentTime,NULL);
		diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);
	}

 //   write (fd, request, strlen(request));
}
