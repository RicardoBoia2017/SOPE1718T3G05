#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>



#define WIDTH_PID 5
#define WIDTH_SEAT 4
#define WIDTH_XXNN 5
#define SPACE 32

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

//calculates length from int
int nDigits (int number)
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

void writePid (FILE * file)
{
	int pidLength = nDigits (getpid());

	if (pidLength < 5)
	{
		int i;
		for (i = pidLength; i < WIDTH_PID; i++)
			fprintf (file, "0");
	}

	fprintf (file, "%d", getpid());
}

void writeXXNN (FILE * file, int nBookedSeats, int seatPosition) //seat position in the booked seats array
{
	int nBookedSeatsLength = nDigits (nBookedSeats);
	int seatPositionLength = nDigits (seatPosition);
	int i;

	//xx
	if (seatPositionLength != (WIDTH_XXNN-1)/2)
	{
		for (i = seatPositionLength; i < (WIDTH_XXNN-1)/2; i++)
			fprintf (file, "0");
	}

	fprintf (file, "%d.", seatPosition);

	//ss
	if (nBookedSeatsLength != (WIDTH_XXNN-1)/2)
	{
		for (i = nBookedSeatsLength; i < (WIDTH_XXNN-1)/2; i++)
			fprintf (file, "0");
	}

	fprintf (file, "%d ", nBookedSeats);
}

void writeSeat (FILE * file, int seatId)
{
	int seatLength = nDigits (seatId);

	if (seatLength != WIDTH_SEAT )
	{
		int i;
		for (i = seatLength; i < WIDTH_SEAT; i++)
			fprintf (file, "0");
	}

	fprintf (file, "%d", seatId);
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
	FILE * bookFilePointer; //pointer for the logs file

	//opens text file
	bookFilePointer = fopen ("cbook.txt", "a"); //O_WRONLY|O_CREAT|O_APPEND

	if (bookFilePointer == 0)
	{
		printf ("There was an error opening the register file.\n");
		exit(2);
	}

	char * values = strtok(answer, " ");

	values = strtok (NULL, " ");

	while (values != NULL)
	{
		writeSeat (bookFilePointer, atoi(values));

		fprintf(bookFilePointer, "\n");

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



	int s = 0;
	char * seat = strtok(argv[3], " ");

	while (seat != NULL)
	{
		request->wantedSeats[s] = atoi(seat);
		seat = strtok (NULL, " ");
		s++;
	}

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

   writeInCbook(copy); //cbook
   registerRequest(answer);  //clog


   close (answerFd);
   remove (answerFifoName);
   exit (0);
}
