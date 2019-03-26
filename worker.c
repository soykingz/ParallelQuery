#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"

/* The function get_word should be added to this file */

/*
 * Populate a word list(linked struct) and a filename list(array of filenames)
 * The node in the word linked list contains a word and a frequency array of
 * the word containing frequency of this word in different files.
 * The ith element in the frequency list represents the frequency of
 * the word in a file and the name of that file is the
 * ith file in the filenames array.
 */
void make_list(Node **head, char **filenames)
{
   // assume that binary index file and binary filenames file
   // are in the current working directory
   read_list("index", "filenames", head, filenames);
}

/*
 * Look up the frequency of a given word in a word list with
 * reference of filename list.
 * Populate an array of FreqRecord, where every FreqRecord contains
 * the frequence of the word in a single file.
 * REQ: frArray point to an array of size MAXFILES
 */
void test_word(char *word, FreqRecord *frArray)
{
    // construct word list and filename list
    // declar a null pointer to Node
    Node *head = NULL;
    // declar an array of MAXFILES strings of PATHLENGHTH character
    char *filenames[MAXFILES];
    make_list(&head, filenames);
    // try to find a Node of word in word link list
    Node *cur = head;
    int found = 0;
    while (cur->next != NULL && (!found)) {
	if (strcmp(cur->word, word) == 0) {
	    found = 1;
	} else {
	    cur = cur->next;
	}
    }
    // populate array of FreqRecord
    FreqRecord end;
    int recorded = 0;
    if (found) {
        // loop through every frequency in the Node
	int i;
	for (i = 0; i < MAXFILES; i ++){
	    // ignore 0 frequency
	    if (cur->freq[i] != 0) {
		FreqRecord new;
		// set the frequency and filename
		new.freq = cur->freq[i];
		// have to use strcpy because array type is not assignable
		strcpy(new.filename, filenames[i]);
		// add to the array and increment recorded
		frArray[recorded] = new;
		recorded++;
	    }
	}
    }
    // add the terminating FreqRecord
    end.freq = 0;
    frArray[recorded] = end;
}

/* Print to standard output the frequency records for a word.
 * Used for testing.
 */
void print_freq_records(FreqRecord *frp) {
	int i = 0;
	while(frp != NULL && frp[i].freq != 0) {
		printf("%d    %s\n", frp[i].freq, frp[i].filename);
		i++;
	}
}

/**
 * Read a word from in and write to out frequency records of the word in directory dirname.
 * - load the word list and filenames list found in directory dirname
 * - read a word from the file descriptor "in"
 * - find the word in the word list
 * - write the frequency records to the file descriptor "out"
 */
void run_worker(char *dirname, int in, int out){
    // construct word list and filename list
    // declar a null pointer to Node
    Node *head = NULL;
    // declar an array of MAXFILES strings of PATHLENGHTH character
    char *filenames[MAXFILES];
    // concat index and filenames with path
    int len = strlen(dirname);
    char indexPath[len + 7];
    char filenamesPath[len + 11];
    strncpy(indexPath, dirname, len + 1);
    strncat(indexPath, "/index", 7);
    strncpy(filenamesPath, dirname, len + 1);
    strncat(filenamesPath, "/filenames", 11);
    // load the word list and filenames list
    read_list(indexPath, filenamesPath, &head, filenames);
    // read word from fd in
    int n;
    char word[MAXWORD];
    while((n = read(in, word, MAXWORD - 1)) > 0) {
	word[n] = '\0';
	// loop through the word list to find the the word
        Node *cur = head;
        int found = 0;
        while (cur->next != NULL && (!found)) {
	    //printf("inside node loop the filename0 is %s\n", filenames[0]);
            if (strcmp(cur->word, word) == 0) {
                found = 1;
            } else {
                cur = cur->next;
            }
        }
	// if we found the word in word list,
	// loop through the frequency array
	// write a FreRecord to out for every non-zero freqency
	if (found) {
            int i;
            for (i = 0; i < MAXFILES; i ++){
                if (cur->freq[i] != 0) {
                    FreqRecord new;
                    // set the frequency and filename
                    new.freq = cur->freq[i];
                    // have to use strcpy because array type is not assignable
		    strncpy(new.filename, filenames[i], PATHLENGTH - 1);
		    // write to out
		    write(out, &new, sizeof(FreqRecord));
		}
	    }
	}
    	// always write a terminating FreqRecord to out
	FreqRecord end;
	end.freq = 0;
        write(out, &end, sizeof(FreqRecord));
    }
}
/*
 * insert freqinput i in to freqArray such that
 * freqArray = [...FreqRecord j, freaqread i, FreRecord k,...] where j > i > k
 * loop through every FreqRecord in the array
 * to find where to insert freqread
 * REQ: numRecored is the current number of FreqRecord in freArray
 * REQ: bound is the size of freArray
 */
void insertFreqInOrder(FreqRecord freqinput, FreqRecord *freqArray, int numRecorded, int bound)
{
    // if numRecorded == 0 then just add
    if (numRecorded == 0) {
	freqArray[0] = freqinput;
	return;
    }
    // deal with prepend
    if ((freqinput.freq > freqArray[0].freq) || (freqinput.freq == freqArray[0].freq)) {
	// shift everything by 1, i will be in the place of i + 1
	int i = 0;
	FreqRecord temp = freqArray[0];
	while((i < numRecorded) && (i < bound)) {
	    // save a pointer to i + 1
	    FreqRecord temp2 = freqArray [i + 1];
	    // set i + 1 to be i
	    freqArray[i + 1] = temp;
            // set the temp to be i + 1 for the next iteration
	    temp = temp2;
	    // increment i
	    i++;
	}
	// add the add element of array if its within bound
	if (numRecorded < bound) {
	    freqArray[i + 1] = temp;
	}
	// prepend
	freqArray[0] = freqinput;
	return;
	    
    }
    int cur = 0;
    // stop when freqinput > cur
    // or stop at the last FreqRecord of the list
    // or stop at the end of the list (when cur = MAXRECORDS -1)
    while((freqinput.freq < freqArray[cur].freq) && (cur < (MAXRECORDS - 1)) && (cur < numRecorded)) {
         cur++;
    }
    // if we manage to hit the end of the list,
    // then every FreRecord in freqArray is greater than freqinput
    if((cur + 1) == MAXRECORDS) {
        // replace the last FreRecord if
	// bigger freqinput is bigger then the last FreRecord of freqArray
        if ((freqinput.freq > freqArray[cur].freq) || (freqinput.freq == freqArray[cur].freq)) {
             freqArray[cur] = freqinput;
	}
    } else if ((cur + 1) == numRecorded) {
        /* we are at the last element of the list
         * if freqinput is greater then the last element,
         * it will go in to the place of the last element and the last element will go after it */
        if ((freqinput.freq > freqArray[cur].freq) || (freqinput.freq == freqArray[cur].freq)) {
            // save the last element
            FreqRecord temp = freqArray[cur];
            // set the last element to freqread
            freqArray[cur] = freqinput;
            // set the previously last element after it
            freqArray[cur + 1] = temp;
        } else {
	    // else insert freqinput after the last element
	    freqArray[cur + 1] = freqinput;
	}
    } else {
	/* we are some where in the middle of the list
	   shift cur and everything after cur by 1  */
	// shift everything by 1, i will be in the place of i + 1
        int i = cur;
        FreqRecord temp = freqArray[i];
        while((i < numRecorded) && (i < bound)) {
            // save a pointer to i + 1
            FreqRecord temp2 = freqArray [i + 1];
            // set i + 1 to be i
            freqArray[i + 1] = temp;
            // set the temp to be i + 1 for the next iteration
            temp = temp2;
            // increment i
            i++;
        }
        // add the previoulsy last element of array if its within bound
        if (i + 1 < bound) {
            freqArray[i + 1] = temp;
        }
	// insert freqinput at cur
	freqArray[cur] = freqinput;
    }
}
