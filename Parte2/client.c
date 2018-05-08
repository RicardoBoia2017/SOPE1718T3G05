#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define WIDTH_PID 5
#define WIDTH_XXNN 5
#define WIDTH_SEAT 4

typedef struct
{
	int clientId; // pid of process
	int nSeats; //number of desired seats
	int wantedSeats [99]; //array with the client's favorite spots
} Request;

int requestsFd; // requests file descriptor
char answerFifoName [8]; //answer file descriptor

//Opens log file
void openLogFile ()
{

}

//Writes to log file
void writeToLogFile (char* string)
{
}

//calculates length from int
int numDigits (int number)
{
	int res = 0;

	while (number >= 10)
	{
		number /= 10;
		res++;
	}

	res ++;

	return res;
}

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

void writePid(FILE * file)
{
	int pidLength = numDigits (getpid());

	if (pidLength < 5)
	{
		int i;
		for (i = pidLength; i < WIDTH_PID; i++)
			fprintf (file, "0");
	}

	fprintf (file, "%d ", getpid());
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

	//if server returns an error (negative number)
	if (strlen(answer) == 2)
	{
		//writes pid with WIDTH_PID
		writePid(logFilePointer);

		int errCode = atoi(answer);
		char * err = getErrorMsg (errCode);

		fprintf (logFilePointer, "%s\n", err);
		return;
	}

	//write the booked
	int s = 1;
	char * values = strtok(answer, " ");
	int seatsBooked = atoi(values);
	int seatsBookedLength = numDigits(seatsBooked);

	printf ("%d\n", seatsBooked);
	values = strtok (NULL, " ");

	while (values != NULL)
	{
		int seatLength = numDigits (atoi(values));
		int sLength = numDigits (s);
		int i;

		//writes with WIDTH_PID
		writePid(logFilePointer);

		//xx.nn ssss
		//xx
		if (sLength != (WIDTH_XXNN-1)/2)
		{
			for (i = sLength; i < (WIDTH_XXNN-1)/2; i++)
				fprintf (logFilePointer, "0");
		}

		fprintf (logFilePointer, "%d.", s);

		//ss
		if (seatsBookedLength != (WIDTH_XXNN-1)/2)
		{
			for (i = seatsBookedLength; i < (WIDTH_XXNN-1)/2; i++)
				fprintf (logFilePointer, "0");
		}

		fprintf (logFilePointer, "%d ", seatsBooked);

		//ssss (seat id)
		if (seatLength != WIDTH_SEAT )
		{
			for (i = seatLength; i < WIDTH_SEAT; i++)
				fprintf (logFilePointer, "0");
		}

		fprintf (logFilePointer, "%d\n", atoi(values) );

		values = strtok (NULL, " ");
		s ++;
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

    char answer [800];

	while (diff < openTime)
	{
        read (answerFd, answer, sizeof(answer));

        if (strlen(answer) != 0)
        	break;

		gettimeofday(&currentTime,NULL);
		diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);
	}

   registerRequest(answer);


   close (answerFd);
   remove (answerFifoName);
   exit (0);
}
