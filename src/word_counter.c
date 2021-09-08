#include "../headers/word_counter.h"

dict_p word_counting(int argc, char* argv[])
{
    double start, result;
    int rank, num_proc;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

    if(argc < 2){
        printf("Please pass atleast one text file...\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
        exit(-1);
    }

    start = MPI_Wtime();

    long total_size = 0;
    long* file_list_size = calloc(argc - 1, sizeof(long));
    //dict used to merge all dicts
    dict_p global_dict = dict_create();
    //dict used by the process to calculate intermediate results
    dict_p proc_dict = dict_create();

    //needed for gatherv
    int displs[num_proc];
    //buffer entries
    char *buffer_entries;
    //buffers used to store sizes and entries of dicts
    int *global_buffer_dicts_sizes = calloc(num_proc, sizeof(int));
    int *global_buffer_dicts_entries = calloc(num_proc, sizeof(int));

    files_info data[num_proc], proc_data = data_init(argc - 1);
    //init data structs
    for(int i = 0; i < num_proc; i++)
        data[i] = data_init(argc - 1);
    
    //--------files_info CUSTOM TYPES--------//
    //type used for the list of file indexes
    MPI_Datatype mpi_files_info_files_index;
    MPI_Type_contiguous(argc - 1, MPI_INT, &mpi_files_info_files_index);
    MPI_Type_commit(&mpi_files_info_files_index);

    //type used to store infos about data structs
    MPI_Datatype mpi_files_info_infos;
    MPI_Type_contiguous(4, MPI_INT, &mpi_files_info_infos);
    MPI_Type_commit(&mpi_files_info_infos);
    //---------------------------------//

    //--------TYPE_CREATE_STRUCT DICT_ENTRY--------//
    //we have to create a struct type, otherwise we should different copies and memory and pack everything
    const int dict_fields = 2;
    MPI_Aint dict_offsets[dict_fields];
    MPI_Aint extent_dict = (char*)(&proc_dict->entry[1]) - (char*)(&proc_dict->entry[0]);
    int dict_block_length[] = {STRLEN, 1};
    MPI_Datatype dict_types[] = {MPI_CHAR, MPI_INT};
    MPI_Datatype mpi_dict_entry_struct_tmp, mpi_dict_entry_struct;

    //calcualte offsets
    dict_offsets[0] = offsetof(dict_entry, name);
    dict_offsets[1] = offsetof(dict_entry, occurrences);

    //create mpi struct
    MPI_Type_create_struct(dict_fields, dict_block_length, dict_offsets, dict_types, &mpi_dict_entry_struct_tmp);
    MPI_Type_create_resized(mpi_dict_entry_struct_tmp, 0, extent_dict, &mpi_dict_entry_struct);
    MPI_Type_commit(&mpi_dict_entry_struct);
    //---------------------------------------------//

    //Master
    if(rank == 0){
        //find the size in bytes of all files, divide the size with the number of workers
        for(int i = 1; i < argc; i++){
            char *path = filepath(argv[i]);
            FILE* file = fopen(path, "r");

            if(file == NULL){
                printf("File %s does not exists!\n", path);
                exit(-1);
            }
            //end of the file
            fseek(file, 0, SEEK_END);
            file_list_size[i - 1] = ftell(file);
            //printf("Dimensione del %d° file: %d\n", i, file_list_size[i - 1]);
            total_size += ftell(file);
            fclose(file);
            free(path);
        }
        //printf("Files given -> %d, total size -> %ld bytes\n", argc - 1, total_size);
        //Divide the total size by the number of slaves
        int slave_size = total_size / num_proc;
        int remainder = total_size % num_proc;
        //position in the buffer_all_data to store data
        int position = 0;
        //Add the remainder to the slaves
        for(int i = 0; i < num_proc; i++){
            data[i].size = slave_size;
            if(remainder > 0 && i < remainder)
                data[i].size += 1;
        }
        //copy file_list_size, it will be used later...
        long* file_sizes = calloc(argc - 1, sizeof(long));
        memcpy(file_sizes, file_list_size, (argc - 1) * sizeof(long));
        //supp_size is needed for keep track of much bytes the i process must analyze
        int index = 0, supp_size = 0;
        //start from 0 and it's value is always + 1 in data because it's used to reference files in argv
        int file_counter = 0;
        //last file index for data structure i
        int file_index = 0;

        for(int i = 0; i < num_proc; i++){
            //if the proc has no bytes to analyze just pass to the next one
            if(data[i].size == 0) continue;
            //init file indexes
            index = 0;
            //file_counter + 1 because the first one is the name of executable, we dont need it.
            data[i].files[index].index = file_counter + 1;
            //each process must start from were the last process stopped, but could happen that a process
            //did nothing, so we must find the last process which worked and take it's final byte analyzed as start
            if(i > 0){
                int j = i - 1;
                while(data[j].size <= 0) j--;
                //if the end is equal to the size of curren file, start from 0.
                data[i].start = data[j].end % file_sizes[file_counter];
            }
            supp_size = data[i].size;
            //this piece of code is needed to determinate the end of a process and the files
            //that must be analyzed by the process i.
            while(supp_size > 0){
                if(supp_size > file_list_size[file_counter]){
                    supp_size -= file_list_size[file_counter];
                    //this if is needed if the process must analyze only a whole document, otherwise overflow.
                    if(supp_size == 0)  
                        data[i].end = file_list_size[file_counter];

                    file_list_size[file_counter] = 0;
                    
                    if(argc - 1 > file_counter){
                        file_counter++;
                        if(supp_size > 0){
                            index++;
                            data[i].files[index].index = file_counter + 1;
                        }
                    }
                } else {
                    //here means that the process scans this last file, so we save the last bytes to be analyzed
                    file_list_size[file_counter] -= supp_size;
                    if(index == 0)
                        data[i].end = data[i].start + supp_size;
                    else 
                        data[i].end = supp_size;

                    if(file_list_size[file_counter] == 0)
                        file_counter++;

                    supp_size = 0;
                }
            }
            //Updating number of files for each process
            for(int l = 0; l < argc - 1; l++){
                if (data[i].files[l].index > 0)
                    data[i].num_files++;
                else break;
            }

            //each proc should check is his last char is a word terminator, otherwise must do some right padding
            //the right padding is made by removing, for each char needed, one byte until terminator symbol is found
            //until a word terminator is reached (terminator symbols are \t, \n, ' ', comma, dot, ;, :, ').
            //the last process doesn't need these operations
            if(i + 1 < num_proc){
                unsigned int file_size = 0;
                int file_index = 0;
                //obtain the last element of the file list != from 0
                for(int j = 0; j < argc - 1; j++){
                    if(data[i].files[j].index == 0){
                        file_index = data[i].files[j - 1].index; 
                        file_size = file_sizes[j];
                        break;
                    }
                }
                //this is needed if a process must analyze all files
                //the index must be in range of 0 to argc - 1. So argc - 2 is the last index.
                if(file_size == 0){
                    file_index = argc - 2;
                    file_size = file_sizes[file_index];
                    //file index is 0 when we pass only 1 file, so we have to analyze file 1
                    if(file_index == 0) file_index++;
                }

                //last_byte_last_file tells me the point were to do seek and check if the last char
                // to be analyzed is /n, /t or ' '. If not, increase the size until one of the previous chars
                //and decrease the size of the next process by x amount of right padding did in the current process.
                char *path = filepath(argv[file_index]);
                FILE* file = fopen(path, "r");
                fseek(file, data[i].end - 1, SEEK_SET);
                //check if the last byte is alphabet char or one of the following symbols
                int ch = fgetc(file);
                if(!is_word_terminator(ch) && data[i].end > 0){
                    //this counter is used to determinate which process we are going to take bytes
                    //to allow right padding at the current process
                    int proc_counter = i + 1;
                    while(1){
                        ch = fgetc(file);
                        if(is_word_terminator(ch)) break;
                        //Check if process proc_counter can give a byte to current process, otherwise
                        //pass to next process
                        int old_size = data[i].size;
                        while(data[i].size == old_size){
                            if(data[proc_counter].size > 0){
                                data[i].size++;
                                data[i].end++;
                                data[proc_counter].size--;
                                file_list_size[file_counter]--;
                            }
                            //exit from the inner while
                            else if(proc_counter < num_proc) proc_counter++;
                            else break;
                        }
                        //exit from the outer while, it should never happen
                        if(data[i].size == old_size) break;
                    }
                    if(file_list_size[file_counter] == 0) file_counter++;
                }
                fclose(file);
                free(path);
            }
        }

        //Send data to other processes
        for(int i = 1; i < num_proc; i++){
            int data_infos[] = {data[i].size, data[i].start, data[i].end, data[i].num_files};

            MPI_Send(data_infos, 1, mpi_files_info_infos, i, 1, MPI_COMM_WORLD);
            MPI_Send(&data[i].files[0], 1, mpi_files_info_files_index, i, 1, MPI_COMM_WORLD);
        }

        proc_data = data[0];
        word_counter(proc_data, proc_dict, argv);
    }
        //master doing is piece of work
        /*proc_data = data[0];

        //data_print(proc_data);

        word_counter(proc_data, proc_dict, argv);//, file_counter);
        //dict_print(proc_dict);
    
        //--------GATHER ALL DICTS--------//
        MPI_Gather(&proc_dict->num_entries, 1, MPI_INT, global_buffer_dicts_entries, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gather(&proc_dict->size, 1, MPI_INT, global_buffer_dicts_sizes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        //Update size and num entries of global dict, calculate displs to find the displacement ->
        //-> relative to the global_dict entries of each process
        int displs[num_proc];
        displs[0] = 0;
        for(int k = 0; k < num_proc; k++){
            if(k > 0)
                displs[k] = displs[k - 1] + global_buffer_dicts_entries[k - 1];
            global_dict->size += global_buffer_dicts_sizes[k];
            global_dict->num_entries += global_buffer_dicts_entries[k];
        }

        //Allocate the needed memory to hold all the entries
        dict_increase_size(global_dict);

        MPI_Gatherv(proc_dict->entry, proc_dict->num_entries, mpi_dict_entry_struct, global_dict->entry, 
                global_buffer_dicts_entries, displs, mpi_dict_entry_struct, 0, MPI_COMM_WORLD);
            
        global_dict = merge_dict(global_dict);*/
    //} 
    //Slaves
    else {
        int data_infos[4];

        MPI_Recv(data_infos, 1, mpi_files_info_infos, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&proc_data.files[0], 1, mpi_files_info_files_index, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        proc_data.size = data_infos[0];
        proc_data.start = data_infos[1];
        proc_data.end = data_infos[2];
        proc_data.num_files = data_infos[3];

        //data_print(proc_data);

        word_counter(proc_data, proc_dict, argv);
        //dict_print(proc_dict);

        //--------SLAVES SEND DICT files_info TO MASTER--------//
        /*MPI_Gather(&proc_dict->num_entries, 1, MPI_INT, global_buffer_dicts_entries, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gather(&proc_dict->size, 1, MPI_INT, global_buffer_dicts_sizes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gatherv(proc_dict->entry, proc_dict->num_entries, mpi_dict_entry_struct, NULL, 
                NULL, NULL, NULL, 0, MPI_COMM_WORLD);*/
    }

    //--------GATHER ALL DICTS--------//
    MPI_Gather(&proc_dict->num_entries, 1, MPI_INT, global_buffer_dicts_entries, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&proc_dict->size, 1, MPI_INT, global_buffer_dicts_sizes, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if(rank == 0){
        //Update size and num entries of global dict, calculate displs to find the displacement
        //relative to the global_dict entries of each process
        displs[0] = 0;
        for(int k = 0; k < num_proc; k++){
            if(k > 0)
                displs[k] = displs[k - 1] + global_buffer_dicts_entries[k - 1];
            global_dict->size += global_buffer_dicts_sizes[k];
            global_dict->num_entries += global_buffer_dicts_entries[k];
        }
        dict_increase_size(global_dict);
    }
    
    MPI_Gatherv(proc_dict->entry, proc_dict->num_entries, mpi_dict_entry_struct, global_dict->entry, 
                global_buffer_dicts_entries, displs, mpi_dict_entry_struct, 0, MPI_COMM_WORLD);

    if(rank == 0){
        global_dict = merge_dict(global_dict);
        result = MPI_Wtime() - start;
        printf("Time elapsed: %0.3f sec(s).\n", result);
    }

    MPI_Type_free(&mpi_files_info_infos);
    MPI_Type_free(&mpi_dict_entry_struct);
    MPI_Type_free(&mpi_dict_entry_struct_tmp);
    MPI_Type_free(&mpi_files_info_files_index);
    MPI_Finalize();

    dict_free(proc_dict);
    free(file_list_size);
    free(global_buffer_dicts_entries);
    free(global_buffer_dicts_sizes);

    if(rank == 0)
        return global_dict;

    exit(0);
}