#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"

int main (int argc, char **argv)
{
    extern void insertFreqInOrder(FreqRecord freqinput,
	FreqRecord *freqArray, int numRecorded, int bound);
    char ch;
    char path[PATHLENGTH];
    char *startdir = ".";
    // if user used option tag d then use the directory provide by the user
    // else use the current working directory
    while ((ch = getopt (argc, argv, "d:")) != -1) {
      switch (ch)
	{
	case 'd':
	  startdir = optarg;
	  break;
	default:
	  fprintf (stderr, "Usage: queryone [-d DIRECTORY_NAME]\n");
	  exit (1);
	}
    }
    // Open the directory
    DIR *dirp;
    if ((dirp = opendir (startdir)) == NULL) {
      perror ("opendir");
      exit (1);
    }
    // set up two sets of pipes
    struct dirent *dp;
    // one set of pipe for sending word to child
    int pipefd[30][2];
    int i;
    for(i = 0; i < 30; i++) {
        if (pipe(pipefd[i])) {
	    perror("pipe");
	}
    }
    // one more set of pipe for reading FreqRecord from child
    int pipefd2[30][2];
    int j;
    for(j = 0; j < 30; j++) {
        if (pipe(pipefd2[j])) {
            perror("pipe");
        }
    }
    /*
     * Loop through each file in the directory
     * For each entry in the directory, ignore . and .., and check
     * to make sure that the entry is a directory, then call run_worker
     * to process the index file contained in the directory.
     * Note that this implementation of the query engine iterates
     * sequentially through the directories, and will expect to read
     * a word from standard input for each index it checks.
     */
    int n;
    int numDir = 0;
    while ((dp = readdir (dirp)) != NULL) {
        // ignore . and .. and .svn
        if (strcmp (dp->d_name, ".") == 0 ||
	    strcmp (dp->d_name, "..") == 0 || strcmp (dp->d_name, ".svn") == 0) {
	    continue;
	}
        // concat the sub directory name to the path
        strncpy (path, startdir, PATHLENGTH);
        strncat (path, "/", PATHLENGTH - strlen (path) - 1);
        strncat (path, dp->d_name, PATHLENGTH - strlen (path) - 1);
        // check permission
        struct stat sbuf;
        if (stat (path, &sbuf) == -1) {
	    //This should only fail if we got the path wrong
	    // or we don't have permissions on this entry.
	    perror ("stat");
	    exit (1);
	}
        // if it is a directory, fork a child porcess to run run_worker
        if (S_ISDIR (sbuf.st_mode)) {
	    // create two more set of pipe
	    n = fork();
	    if(n < 0) {
		perror("fork");
	    // child
	    } else if (n == 0) {
		// close the wrong side of pipe
		close(pipefd[numDir][1]);
		close(pipefd2[numDir][0]);
		// run_worker
		run_worker(path, pipefd[numDir][0], pipefd2[numDir][1]);
		// close pipe after finished
		close(pipefd[numDir][0]);
                close(pipefd2[numDir][1]);
		exit(0);
	    } else {
		// close the wrong side of pipe from parent
		close(pipefd[numDir][0]);
                close(pipefd2[numDir][1]);
	    }
	    numDir++;
	}
    }
    // get word from stdin
    int found = 0;
    char word[MAXWORD];
    while(printf("please enter a word\n"), fgets(word, MAXWORD - 1, stdin)) {
        char *p;
        if ((p = strchr(word, '\n'))) {
                *p = '\0';
	}
	// write to every child
	int k;
	for(k = 0; k < numDir; k++) {
	    write(pipefd[k][1], word, MAXWORD - 1);
	}
	// read FreqRecord from every child
	FreqRecord freqread;
	FreqRecord freqArray[MAXRECORDS];
        int numRecorded = 0;
	int l;
	for(l = 0; l < numDir; l++ ) {
	    found = 0;
	    // read the FreqRecords back from childs
	    while((!found) && (read(pipefd2[l][0], &freqread, sizeof(FreqRecord)) > 0)) {
		// ignore 0 frequency
		if (freqread.freq == 0) {
		    found = 1;
	        } else {
		    insertFreqInOrder(freqread, freqArray, numRecorded, MAXRECORDS);
		    // increment numRecorded
		    numRecorded++;
		}
	    }
        }
	// add the terminating FreqRecord
	FreqRecord end;
	end.freq = 0;
	if (numRecorded < MAXRECORDS) {
	    freqArray[numRecorded] = end;
	}
	print_freq_records(freqArray);
    }
    // close all the pipe
    int c;
    for (c = 0; c < 30; c ++) {
	close(pipefd[c][1]);
	close(pipefd2[c][0]);
    }
    return 0;
}

