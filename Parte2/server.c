#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

int numRoomSeats; //available seats to the event
int nTicketsOffices; //number of ticket offices (number of threads)
int openTime; //Operating time of ticket offices

typedef struct
{

} Seat;

int isSeatFree(Seat *seats, int seatNum) //checks if seat is free
{

}

void bookSeat(Seat *seats, int seatNum, int clientId) // books seat seatNum for client clientID
{

}

void freeSeat(Seat *seats, int seatNum) //frees a seat
{

}

void * printHello ()
{
	printf ("Hello\n");
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
	openTime = atoi(argv[3]);

	if (mkfifo ("requests", S_IRUSR | S_IWUSR) == -1) //creates fifo 'requests'
	{
		perror ("ERROR");
		exit(2);
	}

	pthread_t tid[nTicketsOffices];

	int t;
	for (t = 1; t <= nTicketsOffices; t++) {  //creates nTicketsOffices threads
	    if ( pthread_create(&tid[t-1], NULL, printHello, NULL) != 0)
	    {
	    	perror ("ERROR");
	    	exit (3);
	    }
	}

	for (t = 1; t <= nTicketsOffices; t++) { //waits for the running threads
	    pthread_join(tid[t-1], NULL);
	}

}
