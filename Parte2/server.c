#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h> //gettimeofday

#define MAX_ROOM_SEATS 9999
#define MAX_CLI_SEATS 99
#define DELAY() sleep(2) //delay 2 seconds

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
Request * requests [200]; //stores the request that haven't been picked up by thread
int requestsSize = 0; //size of requests array

int stop = 0; //controls tickets offices open time
int numRoomSeats = 0; //available seats to the event

FILE * logFilePointer; //pointer for the logs file
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

//Writes to log file
void writeToLogFile (char* string)
{
	fprintf (logFilePointer, "%s\n",string);
}

//Called when a request is collected. Decrements the position of the remaining requests
void shiftArray ()
{
	int i;
	for (i = 1; i < requestsSize; ++i)
	  requests[i-1] = requests[i];
}

//Reads the requests from clients during openTime.
void getRequests (double openTime)
{
	struct timeval currentTime;

	gettimeofday(&currentTime,NULL);

	double diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec); //time the program has been running

	while (diff < openTime)
	{
		Request *request = malloc (sizeof (Request));

		read (requestsFd, request,sizeof (Request));

		if (request->clientId != 0) //if clientId is 0, then there is no request to be read, otherwise there is.
		{
			requests [requestsSize] = request;
			requestsSize++;
		}

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

	    requestsFd=open("requests" ,O_RDONLY);

	    if (requestsFd == -1)
	    {
	    	perror ("Error");
	    	exit (4);
	    }
}

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

//Writes in log file when request is rejected
void writeRejectedLogFile (int threadId, Request *request, int rejectionMotive)
{
	char message [200];

	//threadId width = 2
	if (threadId < 10)
		sprintf (message, "0%d-", threadId);
	else
		sprintf (message, "%d-", threadId);

	//client Id width = 5
	char clientIdString [5];
	int clientIdLength = numDigits (request->clientId);

	if (clientIdLength == 5)
	{
		sprintf (clientIdString, "%d-", request->clientId);
		strcat (message, clientIdString);
	}
	else
	{
		while (clientIdLength < 5)
		{
			strcat (clientIdString, "0");
			clientIdLength++;
		}

		char clientId [4];
		sprintf (clientId, "%d", request->clientId);

		strcat(clientIdString, clientId);
		strcat(message, clientIdString);
	}

	//number of seats width = 2
	char nSeatsString [2];

	if (request->nSeats < 10)
	{
		sprintf (nSeatsString, "-0%d", request->nSeats);
		strcat (message, nSeatsString);
	}
	else
	{
		sprintf (nSeatsString, "-%d", request->nSeats);
		strcat (message, nSeatsString);
	}

	printf ("%s\n", message);
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

	char message[9];
	Request * request = malloc (sizeof (Request));
	int requestToManage = 0; //to be used as boolean

	sprintf (message, "%d-OPEN", *(int *) id);
	writeToLogFile (message);

	while (!stop)
	{
		requestToManage = 0;

		pthread_mutex_lock(&mut);

		if(requestsSize > 0) //if true, then there is a request to be collected
		{
			requestToManage = 1;
			request = requests [0];
			requestsSize--;
			shiftArray();
		}

		pthread_mutex_unlock(&mut);

		if (requestToManage) //if true, then a request was collected
		{
			//opens fifo to answer client
			int answerFd = 0;
			char fifoName[8];

			sprintf (fifoName, "ans%d", request->clientId);

		    answerFd = open(fifoName ,O_WRONLY);

		    if (answerFd == -1)
		    {
		    	perror ("Error fifo");
		    	exit (6);
		    }

		    //checks conditions
			int returnValue = checkRequest (request);

			if (returnValue != 0)
			{
				char * answer = malloc (2 * sizeof(char));
				sprintf (answer , "%d", returnValue);

				write (answerFd, answer, sizeof(answer));

				writeRejectedLogFile (*(int *) id, request, returnValue);
				continue;
			}


			//tries to book the seats
			int wantedSeatsCount = countFavoriteSeats(request); //number of seats on favorite seats

			int numberSeatsBooked = 0; //number of seats booked in a certain moment
			int seatsBooked [request->nSeats]; //id of seats booked
			int sb = 0; //to parse through seatsBooked array
			int ws = 0; //to parse  through favorite seats array

			int booked = 1; //to be used as boolean. Used to know if all seats were booked or not

			while (numberSeatsBooked < request->nSeats) //while the number of seats booked isn't the required by the client
			{

				if(wantedSeatsCount == 0) //if this happens, then server didn't book the required amount of seats
				{
					write (answerFd, "-5", 2*sizeof(char));
					booked = 0;
					break;
				}

				pthread_mutex_lock(&mut);

				if (isSeatFree(seats, request->favoriteSeats[ws]))
				{
					bookSeat (seats, request->favoriteSeats[ws], request->clientId);
					seatsBooked [sb] = request->favoriteSeats[ws];
					sb++;
					numberSeatsBooked++;
				}

				pthread_mutex_unlock(&mut);

				wantedSeatsCount--;

				ws++;
			}

			if (booked)
			{
				char * answer = malloc (200);

				sprintf (answer, "%d", request->nSeats);

				int i;
				for (i = 0; i < request->nSeats; i++)
				{
					char seat [5];
					sprintf (seat, " %d", seatsBooked[i]);

					strcat (answer, seat);
				}

				write (answerFd, answer, sizeof(answer));

			}
			pthread_mutex_lock(&mut);
			DELAY();
			pthread_mutex_unlock(&mut);

		}

	}

	sprintf (message, "%d-CLOSED", *(int *) id);
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

	makeRequestsFifo ();

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

	close (requestsFd);
	remove ("requests"); // REMOVE REMOVE REMOVE
	exit (0);
}
