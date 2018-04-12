#include <signal.h> //sigaction
#include <stdio.h> //printf
#include <stdlib.h> //exit
#include <unistd.h> //sleep
#include <string.h> //strcmp
#include <fcntl.h> // open

int ignore = 0; //ignores if letter is upper or lower case.
int displayFileNames = 0; //displays names of files that have that pattern
int numberLines = 0; // displays the number of the line before it prints the line
int count = 0; //displays the number of lines in which the pattern is found
int completeWord = 0; //the pattern must form a complete word
int allFiles = 0; //searchs in all files and sub-directories

char* pattern; //word we are looking for
char* fileName; //name of the file where we are looking

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
		printf ("\nInvalid input! Try again.\n");
		sigint_handler(signo);
		return;
	}

}

//int checkLineLength (FILE * file)
//	int counter = 0;
//
//	while (fgetc(file) != '\n')
//	{
//		counter++;
//	}
//
//}

int checkWord (char* word)
{
	if (strcmpi (word, pattern) == 0) //Only returns 1 when the pattern forms a full word
		return 1;

	else if (completeWord == 0 && strstr(word,pattern) != NULL) //checks if pattern is a subset of a word
	{
		return 1;
	}

	return 0;
}

void printLine (char * line, int lineNumber)
{
	if (numberLines)
		printf ("#%d - ", lineNumber);

	printf ("%s\n", line);
}

void search (char * fileName)
{
	char * line = (char*)malloc(500);;
	size_t read;


	FILE* file = fopen (fileName, "r");

	if (file == 0)
	{
		printf ("Wasn't able to open %s.\n", fileName);
	}

	int wordCounter = 0;
	int lineNumber = 1;
	int Found = 0;
	char *words;

	while (fgets(line, 200, file))
	{
		char tmp [200];
		strcpy (tmp,line);
		words = strtok (tmp," ,.-");
		Found = 0;

		while (words != NULL) {

			if (checkWord (words))
			{
				Found = 1;
				wordCounter++;
			}

			words = strtok(NULL, " ,.-"); // goes to next word
		}

		lineNumber++;

		if (Found == 1)
		{
			printLine(line, lineNumber);
		}
	}

}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf ("Invalid number of arguments.\n");
		exit(1);
	}

	struct sigaction action;
	action.sa_handler = sigint_handler;
	sigemptyset(& (action.sa_mask));
	action.sa_flags = 0;

	if (sigaction(SIGINT, &action,  NULL) < 0)
	{
		printf ("Unable to install SIGINT handler\n");
		exit (0);
	}

	pattern = argv [argc-2];
	char * fileName = argv[argc-1];

	int i = 1;

	for (i; i < argc - 2; i++)
	{
		if (strcmp (argv[i], "-i") == 0)
		{
			ignore = 1;
			continue;
		}

		else if (strcmp (argv[i], "-l") == 0)
		{
			displayFileNames = 1;
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
			exit(1);
		}
	}

	search(fileName);

	exit(0);
}
