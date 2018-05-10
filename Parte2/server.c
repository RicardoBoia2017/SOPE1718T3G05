#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h> //gettimeofday


#define MAX_ROOM_SEATS 9999
#define MAX_CLI_SEATS 99
#define DELAY() sleep(1) //delay 1 second

#define WIDTH_PID 5
#define WIDTH_SEAT 4

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;  // mutex for critical section

typedef struct
{
	int seatNum; //seat number
	int isFree; //if the seat is free
	int clientId; //id of the client (initialized as 0)
} Seat;

typedef struct
{
	int clientId; // pid of process
	int nSeats; //number of desired seats
	int favoriteSeats [MAX_CLI_SEATS-1]; //array with the client's favorite spots
} Request;

Seat seats [MAX_ROOM_SEATS - 1]; //seats
int seatsBooked = 0; //number of seats booked for the event
Request * nextRequest; //stores the request that haven't been picked up by thread
int requestInHold = 0; //size of requests array

int stop = 0; //controls tickets offices open time
int numRoomSeats = 0; //available seats to the event

FILE * logFilePointer; //pointer for the logs file
FILE * bookFilePointer; //pointer for book file
int requestsFd; // requests file descriptor
struct timeval startTime; //start time of threads


//Opens log file
void openLogFile ()
{
	logFilePointer = fopen ("slog.txt", "a"); //O_WRONLY|O_CREAT|O_APPEND

	if (logFilePointer == 0)
	{
		printf ("There was an error opening the register file.\n");
		exit(2);
	}
}

void openBookFile()
{
	//opens text file
	bookFilePointer = fopen ("sbook.txt", "a"); //O_WRONLY|O_CREAT|O_APPEND

	if (bookFilePointer == 0)
	{
		printf ("There was an error opening the register file.\n");
		exit(7);
	}

}

//Writes to log file
void writeToLogFile (char* string)
{
	fprintf (logFilePointer, "%s",string);
}

//Called when a request is collected. Decrements the position of the remaining requests
//void shiftArray ()
//{
//	int i;
//	for (i = 1; i < requestsSize; ++i)
//	  requests[i-1] = requests[i];
//}

//Reads the requests from clients during openTime.
void getRequests (double openTime)
{
	struct timeval currentTime;
	char holder [100];

	gettimeofday(&currentTime,NULL);

	double diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec); //time the program has been running

	while (diff < openTime)
	{
		Request *request = malloc (sizeof (Request));

		read (requestsFd, request,sizeof (Request));

		if (request->clientId != 0) //if clientId is 0, then there is no request to be read, otherwise there is.
		{
			nextRequest = request;
			requestInHold = 1;
		}
		sprintf (holder, "%f\n", diff);

		gettimeofday(&currentTime,NULL);
		diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec); //time the program has been running

	}

	stop = 1; //global variable to signal thread to stop
}

//Initializes seats array
void initializeSeats()
{
	int s;
	for (s = 1; s <= MAX_ROOM_SEATS; s++)
	{
		Seat seat ;
		seat.seatNum = s;
		seat.isFree = 1;
		seat.clientId = 0;
		seats [s-1] = seat;
	}
}

//Checks if a seat is free
int isSeatFree(Seat *seats, int seatNum)
{
	if (seats [seatNum-1].isFree == 1)
		return 1;

	return 0;
}

//Books seat #seatNum for client with clientID
void bookSeat(Seat *seats, int seatNum, int clientId)
{
	seats [seatNum-1].clientId = clientId;
	seats [seatNum-1].isFree = 0;
}

//Frees a seat #seatNum
void freeSeat(Seat *seats, int seatNum)
{
	seats [seatNum-1].clientId = 0;
	seats [seatNum-1].isFree = 1;
}

//Creates and open 'requests' fifo
void makeRequestsFifo()
{
		if (mkfifo ("requests", 0660) == -1) //creates fifo 'requests'
		{
			perror ("ERROR");
			exit(3);
		}

	    requestsFd=open("requests" ,O_RDONLY|O_NONBLOCK);

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

	if (number > 0)
	{
		while (number >= 10)
		{
			number /= 10;
			res++;
		}
	}

	else
	{
		while (number <= -10)
		{
			number /= 10;
			res++;
		}
	}

	res ++;
	return res;
}

//Counts number of favorite seats in a request
int countFavoriteSeats (Request * request)
{
	//may not work as intended if 0 is in the middle of the seats
	int wantedSeatsCount = 0;
	int i = 0;
	int seatId  = request->favoriteSeats[0];

	while (seatId != 0)
	{
		wantedSeatsCount++;
		i++;
		seatId = request->favoriteSeats[i];
	}

	return wantedSeatsCount;
}

//Returns the message according to the rejection code
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

void writeClientId (FILE * file, int clientId)
{
	int pidLength = nDigits (getpid());

	if (pidLength < 5)
	{
		int i;
		for (i = pidLength; i < WIDTH_PID; i++)
			fprintf (file, "0");
	}

	fprintf (file, "%d", clientId);
}

void writeSeat (FILE * file, int seatId)
{
	int seatLength = nDigits (seatId);

	if (seatId < 0)
		fprintf (file, "-");

	if (seatLength != WIDTH_SEAT )
	{
		int i;
		for (i = seatLength; i < WIDTH_SEAT; i++)
			fprintf (file, "0");
	}

	if (seatId < 0)
		fprintf (file, "%d", seatId/-1);
	else
		fprintf (file, "%d", seatId);

}

void writeRequestInformation (int threadId, Request *request)
{
	//threadId width = 2
	if (threadId < 10)
		fprintf (logFilePointer, "0");

	fprintf (logFilePointer, "%d-", threadId);

	//client Id width = 5
	writeClientId (logFilePointer, request->clientId);

	//number of seats width = 2
	fprintf (logFilePointer, "-");

	if (request->nSeats < 10)
		fprintf (logFilePointer, "0");

	fprintf (logFilePointer, "%d: ", request->nSeats);

	//favorite seats width = 4
	int favoriteSeatsSize = countFavoriteSeats (request);
	int s = 0;

	while (s < favoriteSeatsSize)
	{
		writeSeat(logFilePointer, request->favoriteSeats[s]);

		fprintf (logFilePointer, " ");

		s++;
	}

	fprintf (logFilePointer, "- ");
}

//Writes in log file when request is rejected
void writeRejectedLogFile (int threadId, Request *request, int rejectionMotive)
{
	writeRequestInformation (threadId, request);

	//writes type of error
	char * errorMsg = getErrorMsg (rejectionMotive);

	fprintf (logFilePointer, "%s\n", errorMsg);

}

//Writes in log file when request is booked
void writeBookedLogFile (int threadId, Request * request, int bookedSeats[])
{
	writeRequestInformation(threadId, request);

	//booked seats width = 4
	int s = 0;
	while (s < request->nSeats)
	{
		writeSeat(logFilePointer, bookedSeats[s]);

		fprintf (logFilePointer, " ");

		s++;
	}
	fprintf (logFilePointer, "\n");

}

void writeInSbook (int seatsBooked[], int numberSeatsBooked)
{
	int s = 0;

	while (s < numberSeatsBooked)
	{
		writeSeat (bookFilePointer, seatsBooked[s]);

		fprintf (bookFilePointer, "\n");
		s++;
	}
}

//Check if a request is valid.
int checkRequest (Request * request)
{
	//check conditions
	if (request->nSeats > MAX_CLI_SEATS) //condition #1 - number of seats in request cannot be higher than MAX_CLI_SEATS
		return -1;

	int nWantedSeats = countFavoriteSeats(request);
	if (nWantedSeats < request->nSeats || nWantedSeats > MAX_CLI_SEATS) //condition #2 - number of wanted spots has to be higher than nSeats and lower than MAX_CLI_SEATS
		return -2;

	int s;
	for (s = 0; s < nWantedSeats; s++) //condition #3 - wanted spot's id has to be positive and lower than numRoomSeats
	{
		if (request->favoriteSeats[s] < 1 || request->favoriteSeats[s] > numRoomSeats)
			return -3;
	}

	if (request->nSeats == 0) //client cant ask to book 0 seats
		return -4;

	if (seatsBooked == numRoomSeats) //condition 6 - checks if the room is full
		return -6;

	return 0;
}

//Thread to book seats
void * ticket_office (void * id)
{
	char message[11];
	Request * request = malloc (sizeof (Request)); //where the requests are gonna be stored
	int requestToBook = 0; //to be used as boolean

	sprintf (message, "%d-OPEN\n", *(int *) id);
	writeToLogFile (message);

	while (!stop)
  	{
		requestToBook = 0;

		pthread_mutex_lock(&mut);

		if(requestInHold) //if true, then there is a request to be collected
		{
			printf ("%d Getting request\n", * (int *) id);
			requestToBook = 1;
			request = nextRequest;
			requestInHold = 0;
		}

		pthread_mutex_unlock(&mut);

		if (requestToBook) //if true, then a request was collected
		{
			//opens fifo to answer client
			int answerFd = 0;
			char fifoName[8];

			sprintf (fifoName, "ans%d", request->clientId);

		    answerFd = open(fifoName ,O_WRONLY);

		    if (answerFd == -1) //client timed out
		    {
		    	continue;
		    }

		    //checks conditions
			int returnValue = checkRequest (request);


			if (returnValue != 0)
			{
				char * answer = malloc (2 * sizeof(char));
				sprintf (answer , "%d", returnValue);

				if (write (answerFd, answer, strlen(answer)) < 0) //client timed out
				{
	//				printf ("WRITING TO REJECTED NOT WORLING\n");
				}

				writeRejectedLogFile (*(int *) id, request, returnValue);

				continue;
			}

			//tries to book the seats
			int favoriteSeatsCount = countFavoriteSeats(request); //number of favorite seats

			int numberSeatsBooked = 0; //number of seats booked in a certain moment
			int seatsBooked [request->nSeats]; //id of seats booked

			int fs = 0; //to parse  through favorite seats array

			int booked = 1; //to be used as boolean. Used to know if all seats were booked or not

			pthread_mutex_lock(&mut);

			while (numberSeatsBooked < request->nSeats) //while the number of seats booked isn't the required by the client
			{

				if(favoriteSeatsCount == 0) //condition #5 - if this happens, then server didn't book the required amount of seats
				{
					printf ("Cant book for client %d\n", request->clientId);
					booked = 0;
					pthread_mutex_unlock(&mut);
					break;
				}

				if (isSeatFree(seats, request->favoriteSeats[fs]))
				{
	//				DELAY();
					bookSeat (seats, request->favoriteSeats[fs], request->clientId);
	//				printf ("Booking seat %d for client %d\n", request->favoriteSeats[fs], request->clientId);
	//				DELAY();
					seatsBooked [numberSeatsBooked] = request->favoriteSeats[fs];
					numberSeatsBooked++;
				}

				favoriteSeatsCount--;

				fs++;
				pthread_mutex_unlock(&mut);

				DELAY();
			}

			printf ("Client %d\n", request->clientId);

			if (!booked)
			{
				write (answerFd, "-5", 2*sizeof(char));

				int i;

				pthread_mutex_lock(&mut);

				for (i = 0; i < numberSeatsBooked; i++)
				{
					freeSeat (seats, seatsBooked [i]);
					printf ("Client %d\n freeing spot #%d", request->clientId, seatsBooked[i]);

//						DELAY();
				}
				writeRejectedLogFile (*(int *) id, request, -5);

				pthread_mutex_unlock(&mut);

			}


			//sends number of seats and ids to cliente
			else
			{
//				write (answerFd, "Hello", 5);
				char answer [600];
				char seat [5];

				sprintf (answer, "%d", request->nSeats);

				int i;
				for (i = 0; i < request->nSeats; i++)
				{
					sprintf (seat, " %d", seatsBooked[i]);

					strcat (answer, seat);
				}
				if (write (answerFd, answer, strlen(answer)) < 0) //client timed out
				{				printf ("Couldn't write to client %d\n", request->clientId);

					pthread_mutex_lock(&mut);

					int i;
					for (i = 0; i < numberSeatsBooked; i++)
					{
						printf ("Freeing spot\n");
						freeSeat (seats, seatsBooked [i]);
//						DELAY();
					}

					pthread_mutex_unlock(&mut);
					continue;

				}
				pthread_mutex_lock(&mut);

				writeBookedLogFile (*(int *) id, request, seatsBooked);
				writeInSbook (seatsBooked, numberSeatsBooked);

				pthread_mutex_unlock(&mut);

			}


		}
	}

	sprintf (message, "%d-CLOSED\n", *(int *) id);
	writeToLogFile (message);

	free (request);

	return 0;
}

int main (int argc, char *argv[])
{
	int nTicketsOffices; //number of ticket offices (number of threads)
	double openTime; //Operating time of ticket offices

	if (argc != 4)
	{
		printf ("Usage: server <num_room_seats> <num_ticket_offices> <open_time> \n");
		exit (1);
	}

	numRoomSeats = atoi(argv[1]);
	nTicketsOffices = atoi(argv [2]);
	openTime = atof(argv[3]);

	initializeSeats();

	openLogFile();
	openBookFile();

	makeRequestsFifo ();

	signal (SIGPIPE, SIG_IGN); //assures the programs runs correctly if a client times out

	pthread_t tid[nTicketsOffices];
	int t;
	int count [nTicketsOffices];

	for (t = 1; t <= nTicketsOffices; t++) {  //creates nTicketsOffices threads
		count [t-1] = t;
	    if ( pthread_create(&tid[t-1], NULL, ticket_office, (void *) &count[t-1]) != 0)
	    {
	    	perror ("ERROR");
	    	exit (5);
	    }
	}

	gettimeofday(&startTime,NULL);

	getRequests (openTime);

	for (t = 1; t <= nTicketsOffices; t++) {  //creates nTicketsOffices threads

		pthread_join (tid[t-1], NULL);
	}

	close (requestsFd);
	remove ("requests");
	exit (0);
}
