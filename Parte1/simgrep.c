#include <signal.h> //sigaction
#include <stdio.h> //printf
#include <stdlib.h> //exit
#include <unistd.h> //sleep
#include <string.h> //strcmp
#include <fcntl.h> // open
#include <dirent.h> //DIR
#include <sys/stat.h> //lstat
#include <sys/wait.h> //wait

int ignore = 0; //ignores if letter is upper or lower case.
int displayFileNames = 0; //displays names of files that have that pattern
int numberLines = 0; // displays the number of the line before it prints the line
int lineCounter = 0; //displays the number of lines in which the pattern is found
int completeWord = 0; //the pattern must form a complete word
int allFiles = 0; //searchs in all files and sub-directories

char* pattern; //word we are looking for
char* fileName; //name of the file where we are looking
int counter = 0; //counter number of lines that have pattern.

FILE * registerFilePointer;

void openRegisterFile (char * registerFileName)
{
	registerFilePointer = fopen (registerFileName, "a"); //O_WRONLY|O_CREAT|O_APPEND

	if (registerFilePointer == 0)
	{
		printf ("There was an error opening the register file.\n");
		exit(1);
	}
}

void writeToRegisterFile (char* string, int pid)
{
	fprintf (registerFilePointer, "%s %8i \n", string, pid);
}

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

int checkWord (char* word)
{
	if (strcmp (word, pattern) == 0) //Only returns 1 when the pattern forms a full word
		return 1;

	else if (ignore == 1 && strcasecmp (word,pattern) == 0) // ignores if letter is in upper case or lower case
		return 1;

	else if (completeWord == 0 && strstr(word,pattern) != NULL) //checks if pattern is a subset of a word
		return 1;

	return 0;
}

void printLine (char * line, int lineNumber)
{
	if (numberLines)
		printf ("#%d - ", lineNumber);

	printf ("%s\n", line);
}

void searchFile (char * fileName)
{
	char * line = NULL;
	size_t lineLength;
	int charRead;

	FILE* file = fopen (fileName, "r");

	if (file == 0)
	{
		printf ("Wasn't able to open file %s.\n", fileName);
		exit(0);
	}

	int lineNumber = 1;
	int Found = 0;
	char *words;


	while ( (charRead = getline(&line, &lineLength, file)) != -1)
	{
		char * tmp = malloc(charRead);
		strcpy (tmp,line);


		words = strtok (tmp," ,.-\"");
		Found = 0;

		while (words != NULL) {

			if (checkWord (words))
				Found = 1;

			words = strtok(NULL, " ,.-"); // goes to next word
		}

		if (Found == 1)
		{
			printLine(line, lineNumber);
			counter ++;
		}

		lineNumber++;
	}

}

void searchDirs (const char * dirName)
{
	 DIR *dir;
	 struct dirent *direntp;
	 struct stat stat_buf;
	 char name [200];
	 int status;

	 int pid = fork ();

	 if (pid > 0) //parent process
	 {
		 wait(&status);
	 }

	 else if (pid == 0) //child process
	 {
		 if ((dir = opendir(dirName)) == NULL)
		 {
			 printf ("Wasn't able to open directory %s.\n", dirName);
			 exit(1);
		 }

		 while ((direntp = readdir (dir)) != NULL)
		 {
			sprintf(name,"%s/%s",dirName,direntp->d_name);
			printf ("%s\n",name);

			if(lstat(name, &stat_buf) == -1)
			{
				perror("Error: lstat");
				exit(1);
			}

			if (S_ISREG(stat_buf.st_mode))
			{
				searchFile (direntp->d_name);
			}

			else if (S_ISDIR (stat_buf.st_mode))
				searchDirs (direntp->d_name);
			else
				continue;
		 }
	 }
}

int main(int argc, char *argv[], char* envp[])
{
	if (argc < 3)
	{
		printf ("Invalid number of arguments.\n");
		exit(1);
	}

	char * name = getenv("LOGFILENAME");

	if (name == NULL) // if it doesn't found environment variable
	{
		printf ("Environment variable LOGFILENAME wasn't found.\n");
		exit (1);
	}

	openRegisterFile(name);

	writeToRegisterFile ("COMANDO ", getpid());

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
			lineCounter = 1;
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

	if (allFiles)
		searchDirs (fileName);
	else
		searchFile(fileName);

	if (lineCounter == 1)
		printf ("\nThe word was found in %d lines.\n", counter);

	exit(0);

}
