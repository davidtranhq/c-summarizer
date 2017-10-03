#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "hash.h"

#define HASHSIZE 101 /* initial ht size */

typedef enum
{
    ARG_ERR = 1,
    FILE_OPEN_ERR,
    FILE_READ_ERR,
    MEM_ALLOC_ERR,
} errors_t;

typedef struct sentence
{
    char *s;
    int score;
} sentence_t;

const char *program_name;

const char* const stop_words[] = {"a", "about", "above", "above", "across", "after", "afterwards", "again", "against", "all", "almost", "alone", "along", "already", "also","although","always","am","among", "amongst", "amoungst", "amount",  "an", "and", "another", "any","anyhow","anyone","anything","anyway", "anywhere", "are", "around", "as",  "at", "back","be","became", "because","become","becomes", "becoming", "been", "before", "beforehand", "behind", "being", "below", "beside", "besides", "between", "beyond", "bill", "both", "bottom","but", "by", "call", "can", "cannot", "cant", "co", "con", "could", "couldnt", "cry", "de", "describe", "detail", "do", "done", "down", "due", "during", "each", "eg", "eight", "either", "eleven","else", "elsewhere", "empty", "enough", "etc", "even", "ever", "every", "everyone", "everything", "everywhere", "except", "few", "fifteen", "fify", "fill", "find", "fire", "first", "five", "for", "former", "formerly", "forty", "found", "four", "from", "front", "full", "further", "get", "give", "go", "had", "has", "hasnt", "have", "he", "hence", "her", "here", "hereafter", "hereby", "herein", "hereupon", "hers", "herself", "him", "himself", "his", "how", "however", "hundred", "ie", "if", "in", "inc", "indeed", "interest", "into", "is", "it", "its", "itself", "keep", "last", "latter", "latterly", "least", "less", "ltd", "made", "many", "may", "me", "meanwhile", "might", "mill", "mine", "more", "moreover", "most", "mostly", "move", "much", "must", "my", "myself", "name", "namely", "neither", "never", "nevertheless", "next", "nine", "no", "nobody", "none", "noone", "nor", "not", "nothing", "now", "nowhere", "of", "off", "often", "on", "once", "one", "only", "onto", "or", "other", "others", "otherwise", "our", "ours", "ourselves", "out", "over", "own","part", "per", "perhaps", "please", "put", "rather", "re", "same", "see", "seem", "seemed", "seeming", "seems", "serious", "several", "she", "should", "show", "side", "since", "sincere", "six", "sixty", "so", "some", "somehow", "someone", "something", "sometime", "sometimes", "somewhere", "still", "such", "system", "take", "ten", "than", "that", "the", "their", "them", "themselves", "then", "thence", "there", "thereafter", "thereby", "therefore", "therein", "thereupon", "these", "they", "thickv", "thin", "third", "this", "those", "though", "three", "through", "throughout", "thru", "thus", "to", "together", "too", "top", "toward", "towards", "twelve", "twenty", "two", "un", "under", "until", "up", "upon", "us", "very", "via", "was", "we", "well", "were", "what", "whatever", "when", "whence", "whenever", "where", "whereafter", "whereas", "whereby", "wherein", "whereupon", "wherever", "whether", "which", "while", "whither", "who", "whoever", "whole", "whom", "whose", "why", "will", "with", "within", "without", "would", "yet", "you", "your", "yours", "yourself", "yourselves"}; /* english words that add no value (stop words) */

static void handle_error(errors_t e);
/*
    error: prints error message based on code e and exits
*/

size_t file_length(FILE *fp);
/*
    file_length: returns length of file in bytes
*/

char *copy_file(FILE *fp);
/*
    copy_file: allocates memory and copies file contents into heap, 
    returns char pointer to contents
*/

hashtable_t *count_words(char *s);
/*
    count_words: counts words in s, stores result in a hashtable and returns
    a pointer to it
*/

void index_word(char *w, hashtable_t *ht);
/*
    index_word: inserts w as a key into the ht if it is unique and not a
    stop word, otherwise increments it's value.
*/

int is_stop_word(char *w);
/*
    is_stop_word: returns 1 if found in list of stop_words, else returns 0
    used by index_word() to determine if a word is a stop word.
*/

void lower_str(char *s);
/*
    lower_str: lowers the case of all letters in a string.
*/

static void handle_error(errors_t e)
{
    fprintf(stderr, "%s: error %i:\n", program_name, e);
    switch (e)
    {
        case ARG_ERR:
            fprintf(stderr, "usage: %s [filename]", program_name);
            break;
        case FILE_OPEN_ERR:
            fputs("error opening file", stderr);
            break;
        case FILE_READ_ERR:
            fputs("error reading file", stderr);
            break;
        case MEM_ALLOC_ERR:
            fputs("error allocating memory (probably not enough memory)", stderr);
            break;
    }
    exit(e);
}

size_t file_length(FILE *fp)
{
    fseek(fp, 0L, SEEK_END);
    size_t bufsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return bufsize;
}

char *copy_file(FILE *fp)
{
    size_t len = file_length(fp);
    char *buf;
    /* try to alloc enough memory for file */
    if ((buf = malloc(len)) == NULL)
        handle_error(MEM_ALLOC_ERR);
    /* copy file into buf */
    size_t chread = fread(buf, 1, len, fp);
    if (ferror(fp))
        handle_error(FILE_READ_ERR);
    buf[chread] = '\0';
    return buf;
}

hashtable_t *count_words(char *s)
{
     /* copy text to be manipulated by strtok() */
    char *scpy;
    if ((scpy = strdup(s)) == NULL)
        handle_error(MEM_ALLOC_ERR);
    /* attempt to create hash table */
    hashtable_t *ht;
    if ((ht = ht_create(HASHSIZE, INTEGER)) == NULL)
        handle_error(MEM_ALLOC_ERR);
    /* loop through and index words */
    const char *delim = " ,.!?'\"()[]";
    char *w = strtok(scpy, delim);
    while (w != NULL)
    {
        lower_str(w);
        index_word(w, ht);
        w = strtok(NULL, delim);
    }
    return ht;
}

void index_word(char *w, hashtable_t *ht)
{
    int *count, init_val = 1;
    if ((count = ht_get(ht, w)) == NULL)
    {
        /* not found, insert */
        if (ht_insert(&ht, w, &init_val) < 0)
            handle_error(MEM_ALLOC_ERR);
    }
    else
        /* found, increment word's count */
        (*count)++;
}

int is_stop_word(char *w)
{
    /* get length of stop_words array */
    size_t n = sizeof(stop_words)/sizeof(stop_words[0]);
    /* binary search */
    size_t lo = 0, hi = n-1, mid;
    while (lo <= hi)
    {
        mid = (lo + hi)/2;
        if (strcmp(w, stop_words[mid]) > 0)
            hi = mid-1;
        else if (strcmp(w, stop_words[mid]) < 0)
            lo = mid+1;
        else
            return 1; /* found */
    }
    /* not found */
    return 0;
}

void lower_str(char *s)
{
    for ( ; *s; s++) *s = tolower(*s);
}

int main(int argc, char *argv[])
{
    program_name = argv[0];
    /* correct usage? */
    if (argc != 2)
        handle_error(ARG_ERR);
    /* attempt to open file */
    FILE *fp;
    if ((fp = fopen(argv[1], "r")) == NULL)
        handle_error(FILE_OPEN_ERR);
    /* read file */
    char *text;
    text = copy_file(fp);
    /* count word occurences */
    words = count_words(text);
    hashtable_t *words;
    
}