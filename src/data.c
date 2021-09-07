#include "../headers/data.h"

//init DATA struct; size, start and end are initiliazed to 0 and file_indexes 
//is going to hold "files" indexes.
// - init files: how many files DATA should hold
DATA data_init(int files){
    DATA data = {0, 0, 0, 0, calloc(files, sizeof(int))};

    return data;
}

//finds the words in the texts and populate the dictionary increasing the word counter if already present
//or adding a new word if not present in the dictonary
// - DATA data: all the infos needed by the processes to analyze the files
// - dict_p dict: dictionary that keeps all the words found in the files
// - char* argv[]: list of pointers to files 
// - int file_counter: files to analyze
void word_counter(DATA data, dict_p dict, char* argv[]){//, int file_counter){
    //If size is 0 just return 
    if(data.size == 0) return;
    
    for(int i = 0; i < data.num_files; i++){
        int argv_index = data.files[i].index;
        int ch = 0, ch_count = 0;
        char *word = malloc(50);
        int file_size = 0, byte_read, starting_byte = data.start, ending_byte = data.end;
        char *path = filepath(argv[argv_index]);

        FILE* file = fopen(path, "r");
        
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);

        //We have multiple file to analyze
        if(i > 0)
            starting_byte = 0;
        if(i + 1 < data.num_files)
            ending_byte = file_size;

        fseek(file, starting_byte, SEEK_SET);
        byte_read = starting_byte;

        while(1){
            ch = fgetc(file);
            byte_read++;
            if(!is_word_terminator(ch)){
                word[ch_count] = ch;
                ch_count++;
            } else {
                //add the word to dict, free word and recreate it with size 1
                if(ch_count > 0){
                    word[ch_count] = '\0';
                    //if uppercase, add 32 to transform the last char to lowercase
                    if((word[0] >= 65) && (word[0] <= 90)){
                        word[0] += 32;
                    }
                    
                    dict_add(dict, word);
                    ch_count = 0;
                    free(word);
                    word = malloc(50);
                }
            }
            if(byte_read > ending_byte) break;
        }
        fclose(file);
        free(path);
    }
}

char* data_print(DATA data){
    printf("\tStart: %d\n\tend: %d\n\tsize: %d\n\t", data.start, data.end, data.size);
    printf("file indexes: ");
    for(int j = 0; j < data.num_files; j++){
        printf("%d ", data.files[j].index);
    }
    printf("\n");
}