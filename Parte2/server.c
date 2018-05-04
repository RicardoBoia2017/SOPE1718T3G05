#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h> //gettimeofday

#define MAX_ROOM_SEATS 9999
#define MAX_CLI_SEATS 99
#define DELAY 5000 //delay 5 seconds

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;  // mutex p/a sec.critica

typedef struct
{
	int seatNum;
	int isFree;
	int clientId;
} Seat;


Seat seats [MAX_ROOM_SEATS - 1]; //seats
int numRoomSeats; //available seats to the event
int nTicketsOffices; //number of ticket offices (number of threads)
double openTime; //Operating time of ticket offices

FILE * logFilePointer; //pointer for the logs file
struct timeval startTime; //start time of threads


//opens log file
void openLogFile ()
{
	logFilePointer = fopen ("slog.txt", "a"); //O_WRONLY|O_CREAT|O_APPEND

	if (logFilePointer == 0)
	{
		printf ("There was an error opening the register file.\n");
		exit(1);
	}
}

//writes to log file
void writeToLogFile (char* string)
{
	fprintf (logFilePointer, "%s\n",string);
}

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

//checks if seat is free
int isSeatFree(Seat *seats, int seatNum)
{
	if (seats [seatNum-1].isFree == 1)
		return 1;

	return 0;
}

// books seat seatNum for client clientID
void bookSeat(Seat *seats, int seatNum, int clientId)
{
	seats [seatNum-1].clientId = clientId;
	seats [seatNum-1].isFree = 0;
}

//frees a seat
void freeSeat(Seat *seats, int seatNum)
{
	seats [seatNum-1].clientId = 0;
	seats [seatNum-1].isFree = 1;
}

//book the seats
void * ticket_office (void * id)
{
//	pthread_mutex_lock(&mut);

	char message[9];
	sprintf (message, "%d-OPEN", *(int *) id);
	writeToLogFile (message);

//	pthread_mutex_unlock(&mut);

	sprintf (message, "%d-CLOSED", *(int *) id);
	writeToLogFile (message);

}

int main (int argc, char *argv[])
{

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


//	if (mkfifo ("requests", S_IRUSR | S_IWUSR) == -1) //creates fifo 'requests'
//	{
//		perror ("ERROR");
//		exit(2);
//	}


	pthread_t tid[nTicketsOffices];
	int count [nTicketsOffices];
	int t;

	for (t = 1; t <= nTicketsOffices; t++) {  //creates nTicketsOffices threads
		count [t-1] = t;
	    if ( pthread_create(&tid[t-1], NULL, ticket_office, (void *) &count[t-1]) != 0)
	    {
	    	perror ("ERROR");
	    	exit (3);
	    }
	}

	gettimeofday(&startTime,NULL);

	for (t = 1; t <= nTicketsOffices; t++) { //waits for the running threads
	    pthread_join(tid[t-1], NULL);
	}


	struct timeval currentTime; //current time

	gettimeofday(&currentTime,NULL);

	double diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);
	printf ("Current time: %f\n" , diff);

	while (diff < openTime)
	{
		gettimeofday(&currentTime,NULL);
		diff = (double) (currentTime.tv_usec - startTime.tv_usec) / 1000000 + (double) (currentTime.tv_sec - startTime.tv_sec);
	}

	printf ("Time is up\n");

	exit (0);
}
