# Bulk File Management


## Overview:

The program only accepts the command line arguments. 
User can optionally: 
1. Create a directory OR Create a file.
2. Delete a directory and its contents OR Delete a text file.
3. Rename a file or a directory
4. Append at most 50 bytes at the end of a text file, the text to add is given as a string on the command line (is ignored if a directory argument is given). And append 50 bytes of even numbers between [50,200] in a sequence for a binary file, where the starting number given by the user.
5. Log all operations performed/errors encountered in a log file


### How to use the code:

#### Compiling and Execution:

```Bash
        make
        ./my_bfm -c <Path> -l <Path> -a <TextFilePath> -e <starting number> OR -s <string to append> -r <OldPath> <NewPath> -d <Path>
``` 

For information on how to create a directory

_Important Note: Warnings will be raised while compiling the file because of the use of function pointers and implicit type conversion from `void *` to `const void *`. These can be ignored as they do not affect the correct functioning of the program._

###### Rename
```Bash
        ./my_bfm -r <OldPath> <NewPath>  # For either a file or a directory 
```
###### Create (directory or file)
```Bash
        ./my_bfm -c <NewFileName> # For a file
        ./my_bfm -c <NewDirectoryName> -f # For a Directory
```
###### Append
```Bash
        ./my_bfm -a <TextFile (Path or fileName)> -s "String To Append" # for a text file
        ./my_bfm -a <BinaryFile (Path or fileName)> -e <integer> # For a binary file
```
###### Delete
```Bash
        ./my_bfm  -d <Name to delete> # Will delete a file or a directory, can take a path as an input or relative path.  
```
###### Log
```Bash
        ./my_bfm <some other operations like create> -l <logfile> # Will log all the actions performed / error encountered during the execution of the process into the logfile.
```


### Behvaioural Choices and some explainations:
* We have chosen to write the program so that every operation can be performed on a different file. The file path for the operation is to be specified immediately after the flag specifying the operation. 
* As a result, while all five operations can be performed at the same time on either the same file (by typing the same address after every command) or on different files, the same command cannot be used again during one execution. If done so, only the file/directory specifed last with that flag will be operated on.

* For example, we can use the following commands:
```Bash
    ./my_bfm -c test.txt -a test.txt -s "Hello! World." -d test.txt -l log.txt

    #OR

    ./my_bfm -c test.txt -a test2.txt "Hello! World." #and so on

```
* Internally, since operation order is fixed, if all commands are used in the same line, one may encounter errors depending on whether it was renamed / deleted, etc.

* Append: We ask the user to give the input if the file is binary or not using the `-e` flag, as the assignment did not ask us to identify the type of the file. Any method to identify type of file will anyways be probabilistic and would involve use of magic database in linux system. One way to get around this issue is to use the built-in file command and grep the output.

* The use of stdlib and stdio are only for the rename system call. However, as the man page says, rename(2) is indeed a system call.

* For creating a file, we chose to give the user the option to specify if it is a file or directory using the `-f` flag. While this adds more responsibility on the user, it provides more functionality as it allows creation of files without any extensions. 

* For all other commands, we determine whether it is a file or directory automatically using system commands.

* All errors are logged into the logfile only if `-l` flag is given, otherwise the errors are returned as error codes.

* We have limited the path length to 1024 bytes. In most real world cases, this will not cause an issue.

* In case of errors with logging enabled, the error in any operation such as create or delete will be logged into the log file and the process will return any errors that may have been encountered during the logging operation itself. If logging is successful, the process will return 0 and user needs to read the log file to determine what went wrong. In case logging is not enabled, the process will return the error code directly. 
* `strerror()` was used to reduce unnecessary workload
* `strcat()` was also used extensively to avoid using `snprintf()`

