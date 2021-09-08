#include <stdio.h>
#include <stdlib.h>
#include "../headers/utils.h"
#include "../headers/dict.h"

typedef struct{
    int index;
} file;

typedef struct{
    int size;
    int start;
    int end;
    int num_files;
    file *files;
} files_info;

files_info data_init(int files);
void word_counter(files_info data, dict_p dict, char* argv[]);//, int file_counter);
char* data_print(files_info data);