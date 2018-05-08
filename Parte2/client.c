#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


#include "Utilities.h"

typedef struct
{
	int clientId; // pid of process
	int nSeats; //number of desired seats
	int wantedSeats [99]; //array with the client's favorite spots
} Request;

int requestsFd; // requests file descriptor
char answerFifoName [8]; //answer file descriptor

//makes fifo that waits for answer
void makeAnswerFifo()
{
	sprintf (answerFifoName, "ans%d", getpid());

	printf ("%s\n",answerFifoName);

	if (mkfifo(answerFifoName, 0660) == -1) //creates fifo 'requests'
	{
		perror("ERROR");
		exit(2);
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

char * getErrorMsg (int code)
{
	switch (code)
	{
	case -1:
		return "MAX";
		break;

	case -2:
		return "NST";
		break;

	case -3:
		return "IID";
		break;

	case -4:
		return "ERR";
		break;

	case -5:
		return "NAV";
		break;

	case -6:
		return "FUL";
		break;
	}

	return " OK";
}

void registerRequest (char answer[800])
{
	FILE * logFilePointer; //pointer for the logs file

	//opens text file
	logFilePointer = fopen ("clog.txt", "a"); //O_WRONLY|O_CREAT|O_APPEND

	if (logFilePointer == 0)
	{
		printf ("There was an error opening the register file.\n");
		exit(2);
	}

	if (strlen(answer) == 0)
	{
		writePid(logFilePointer);

		fprintf (logFilePointer, " OUT\n");

		return;
	}

	//if server returns an error (negative number)
	if (strlen(answer) == 2)
	{
		//writes pid with WIDTH_PID
		writePid(logFilePointer);

		fprintf(logFilePointer," ");

		int errCode = atoi(answer);
		char * err = getErrorMsg (errCode);

		fprintf (logFilePointer, "%s\n", err);

		return;
	}

	//write the booked seats
	int s = 1;
	char * values = strtok(answer, " ");
	int seatsBooked = atoi(values);

	values = strtok (NULL, " ");

	while (values != NULL)
	{
		//writes pid with WIDTH_PID
		writePid(logFilePointer);

		fprintf(logFilePointer," ");

		//xx.nn ssss
		writeXXNN (logFilePointer, seatsBooked, s);

		//ssss (seat id)
		writeSeat (logFilePointer, atoi(values));

		fprintf(logFilePointer, "\n");

		values = strtok (NULL, " ");
		s ++;
	}

}

void writeInCbook (char answer [800])
{
	FILE * logFilePointer; //pointer for the logs file

	//opens text file
	logFilePointer = fopen ("cbook.txt", "a"); //O_WRONLY|O_CREAT|O_APPEND

	if (logFilePointer == 0)
	{
		printf ("There was an error opening the register file.\n");
		exit(2);
	}

	char * values = strtok(answer, " ");

	values = strtok (NULL, " ");

	while (values != NULL)
	{
		writeSeat (logFilePointer, atoi(values));

		fprintf(logFilePointer, "\n");

		values = strtok (NULL, " ");
	}
}

int main (int argc, char *argv[])
{
	double openTime;
    char answer [800] = {0};
	struct timeval startTime, currentTime;
	Request *request = malloc(sizeof(Request));;

	if (argc < 4)
	{
		printf ("Usage: client <time_out> <num_wanted_seats> <pref_seat_list> \n");
		exit (1);
	}

	openTime = atof(argv[1]);

	makeAnswerFifo();

	openRequestsFifo ();

	//builds request
	request->clientId = getpid();
	request->nSeats = atoi(argv[2]);

    int i;
    for (i = 0; i < argc - 3; i++)
    	request->wantedSeats [i] = atoi (argv [i+3]);


	gettimeofday(&startTime,NULL);

	write (requestsFd, request, sizeof (Request));

	gettimeofday(&currentTime,NULL);

	double diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);

   	int answerFd;
	answerFd = open(answerFifoName ,O_RDONLY|O_NONBLOCK);

	if (answerFd == -1)
	{
		perror ("Error");
		exit (3);
	}


	while (diff < openTime)
	{
        read (answerFd, answer, sizeof(answer));

        if (strlen(answer) != 0)
        {
        	break;
        }

		gettimeofday(&currentTime,NULL);
		diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);
	}

   char copy[800];
   strcpy (copy, answer);

   printf ("B4 cbook\n");
   writeInCbook(copy); //cbook
   printf ("B4 clog\n");
   registerRequest(answer);  //clog


   close (answerFd);
//   remove (answerFifoName);
   exit (0);
}
