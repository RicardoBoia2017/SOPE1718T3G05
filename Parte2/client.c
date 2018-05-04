#include <stdio.h>
#include <stdlib.h>


int main (int argc, char *argv[])
{
	if (argc != 4)
	{
		printf ("Usage: client <time_out> <num_wanted_seats> <pref_seat_list> \n");
		exit (1);
	}
}
