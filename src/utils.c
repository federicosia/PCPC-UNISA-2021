#include "../headers/utils.h"

#define PATH "../texts/"

//check if ch is a word terminator
int is_word_terminator(int ch){
    if(ch == ' ' || ch == '\n' || ch == ':' || ch == '.' || ch == '\t' 
        || ch == ';' || ch == ',' || ch == '\'' || ch == '/' || ch == '-' || ch == EOF)
        return 1;
    else return 0;
}

/*Construct the string needed to access the directory texts were all texts are stored.
* - int index : index of the file to be opened, the index must be in range of 1 to argc - 1, otherwise undefined behaviour
* returns the string to access the text file
*/
char* filepath(char* filename){
    char *path_to_file = malloc(strlen(filename) + strlen(PATH) + 1);
    
    strcpy(path_to_file, PATH);
    strcat(path_to_file, filename);
    return path_to_file;
}