#include <signal.h> //sigaction
#include <stdio.h> //printf
#include <stdlib.h> //exit
#include <unistd.h> //sleep


void sigint_handler (int signo)
{
	char input;

	printf ("\nAre you sure you want to terminate the program? (Y/N)");
	input = getchar();

	if (input == 'n' || input == 'N')
		return;

	else if (input == 'y' || input == 'Y')
		exit(0);

	else
	{
		printf ("Invalid input! Try again.\n");
		sigint_handler(signo);
		return;
	}

}

int main()
{
	struct sigaction action;
	action.sa_handler = sigint_handler;
	sigemptyset(& (action.sa_mask));
	action.sa_flags = 0;

	if (sigaction(SIGINT, &action,  NULL) < 0)
	{
		printf ("Unable to install SIGINT handler\n");
		exit (0);
	}

	while (1)
	{
		printf ("On infinite loop\n");
		sleep (1);
	}

	exit(0);
}
