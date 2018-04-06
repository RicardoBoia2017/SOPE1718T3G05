#include <signal.h> //sigaction
#include <stdio.h> //printf
#include <stdlib.h> //exit
#include <unistd.h> //sleep
#include <string.h> //strcmp

int ignore = 0;
int fileName = 0;
int numberLines = 0;
int count = 0;
int completeWord = 0;
int allFiles = 0;

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

int main(int argc, char *argv[])
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

	int i = 1;

	for (i; i < argc; i++)
	{
		if (strcmp (argv[i], "-i") == 0)
		{
			ignore = 1;
			continue;
		}

		else if (strcmp (argv[i], "-l") == 0)
		{
			fileName = 1;
			continue;
		}

		else if (strcmp (argv[i], "-n") == 0)
		{
			numberLines = 1;
			continue;
		}

		else if (strcmp (argv[i], "-c") == 0)
		{
			count = 1;
			continue;
		}

		else if (strcmp (argv[i], "-w") == 0)
		{
			completeWord = 1;
			continue;
		}

		else if (strcmp (argv[i], "-r") == 0)
		{
			allFiles = 1;
			continue;
		}

		else
		{
			printf ("%s is not a valid option.\n", argv[i]);
			printf ("The program will now exit.\n");
			exit(0);
		}
	}

	exit(0);
}
