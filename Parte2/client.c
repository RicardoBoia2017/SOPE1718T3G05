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

typedef struct
{
	int clientId; // pid of process
	int nSeats; //number of desired seats
	int wantedSeats [99]; //array with the client's favorite spots
} Request;

int requestsFd; // requests file descriptor
int answerFd; //answer file descriptor

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

//opens fifo that waits for answer
void makeAnswerFifo()
{
	char fifoName [8];

	sprintf (fifoName, "ans%d", getpid());

	if (mkfifo(fifoName, 0660) == -1) //creates fifo 'requests'
	{
		perror("ERROR");
		exit(2);
	}

	answerFd = open("requests" ,O_RDONLY);

	if (answerFd == -1)
	{
    	perror ("Error");
    	exit (3);
	}
}

//opens fifo that sends requests
void openRequestsFifo ()
{
    requestsFd = open("requests" ,O_WRONLY);

    if (requestsFd == -1)
    {
    	perror ("Error");
    	exit (4);
    }
}

int main (int argc, char *argv[])
{
	double openTime;
	struct timeval startTime, currentTime;
	Request *request = malloc(sizeof(Request));;

	if (argc < 4)
	{
		printf ("Usage: client <time_out> <num_wanted_seats> <pref_seat_list> \n");
		exit (1);
	}

	openTime = atof(argv[1]);

//	makeAnswerFifo();

	openRequestsFifo ();

	//builds request
	request->clientId = getpid();
	request->nSeats = atoi(argv[2]);

    int i;
    for (i = 0; i < argc - 3; i++)
    	request->wantedSeats [i] = atoi (argv [i+3]);


	gettimeofday(&startTime,NULL);

	gettimeofday(&currentTime,NULL);

	double diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);

	while (diff < openTime)
	{
		gettimeofday(&currentTime,NULL);
		diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);
	}

   write (requestsFd, request, sizeof (Request));
}
