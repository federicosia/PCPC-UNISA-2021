# MPI Parallel word count
## Introduction

This software performs word counting in parallel, thanks to MPI, over a large number of files.

- The **master** node (in this case the process with rank 0) reads the file list and distributes the files to all words. Once a process has received its list of files, it starts to processes it counting all the words, keeping track of the frequency of each word found. The structure holding the words and frequencies is called *local histogram*.

- In the second phase each process sends the local histogram to the master node.

- Once the **master** node received all local histograms it will merge them in a single structure called *global histogram*. After the global histogram is created, the master node will create a csv file with the global histogram saved.

## Implementation

Splitting the files between processes is bad choice because files could have different sizes, so the work won't be divided equally. A good solution is sharing the bytes of the list between processes, this way the workload will be the same. The master node will create a struct called ```files_info``` for each process, that has 5 parameters: 

1. ```size```
    - How many bytes the process must analyze in the given file list.
2. ```start```
    - First byte to be analyzed in the first file. A file can be splitted between more processes, this prevent that the process analyze the same data more than a single time.
3. ```end```
    - Last byte to be analyzed in the last file. A file can also be partially analyzed by a process.
4. ```num_files```
    - Number of files present in the list of files sent by the master node.
5. ```files```
    - List of file's indexes

To populate this structure this piece of code is used: 

```
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
```
The first if is used when the number of bytes that have to be analyzed by the process *i* is bigger than the file size with index *file_counter*, the size is decremented ```supp_size - file_list_size[file_counter]```, the index is stored in ```data[i].files```. If ```supp_size > 0``` we continue otherwise we stop, in the second case it means that we stored all the informations needed for the process *i* to start working. The case when the first if is not true means that ```supp_size < file_list_size[file_counter]```, so only a piece of this file will be analyzed, ```data[i].end``` will be equal to the remaining bytes to be analyzed ```supp_size```.

When a process receives, from the **master**, the structure ```files_info``` it will create an additional structure called ```dict```. When a process finds a new word it will be stored in the dictionary, if the entry is already present than a counter is increased, otherwise a new entry will be created with the counter set to 1. This structure is composed by the following parameters:

1. ```num_entries```
    - The number of entries stored in the dictionary.
2. ```size```
    - Entries that can be stored in the dictionary, if num_entries equals size than will be reallocated extra memory to allow more entries to be stored.
3. ```dict_entries```
    - List of entries, a entry is a structure that holds 2 parameters, a word and the occurrences of that word present in the file list.

When a process has done its work it sends the dictionary (the **local histogram**) to the master. When the master receives all the dictionaries, it sorts and merges all the entries creating the **global histogram**, storing it in a file ```.csv```.

The MPI features used are:
- ```MPI_Gather```
- ```MPI_Gatherv```
- ```MPI_Send```
- ```MPI_Recv```  

To send the custom structures towards other processes 3 *derived type* were created using:
- ```MPI_Type_contiguous```
- ```MPI_Type_create_struct```