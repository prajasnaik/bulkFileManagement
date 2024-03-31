// @date: 13 February 2024
// Simple file management program

//Define Statements
#define     _GNU_SOURCE
#define     BUF_SIZE                1024
#define     N_BYTES                 50
#define     MAX_APPEND_SIZE         50
#define     MAX_BUF_SIZE            100
#define     E_OK                    0
#define     E_GENERAL               -1
#define     IS_FILE                 0
#define     IS_DIRECTORY            1
#define     ENABLE                  1
#define     DISABLE                 0

// Include Statements
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <unistd.h>
#include    <errno.h>
#include    <fcntl.h>
#include    <stdlib.h>
#include    <stdio.h> // used only for rename system call
#include    <string.h>
#include    <sys/syscall.h>
#include    <dirent.h>


// Global Variable for Error Code
int         ec          =           E_OK;

// Global Variable Flags for deciding which operation to perform
int         fCreate     =           DISABLE;
int         fDelete     =           DISABLE;
int         fRename     =           DISABLE;
int         fAppend     =           DISABLE;
int         fBinary     =           DISABLE;
int         fPath       =           DISABLE;
int         fDirectory  =           DISABLE;
int         fLog        =           DISABLE;

	// Buffers for storing paths for each function
char        *createPath;
char        *deletePath;
char        *oldPath;
char        *newPath;
char        *appendPath;
char        *writePath;
char        *appendBuffer;
char        *logFileName;

// Buffer for storing values to read and write
char        readBuffer              [MAX_APPEND_SIZE];
char        writeBuffer             [MAX_APPEND_SIZE];

// Function Declarations
int         ProcessCommandLine      (char **, int);
int         CheckDirectory          (char *);
int         NonBlockingOperation    (ssize_t (*) (int, void *, size_t), int, char *, void*, int, int );
int         AppendEvenNumbers       (int, char *);
int         AppendText              (char *, char*);
int         CreateFile              (char *);
int         CreateDirectory         (char *);
int         RemoveFile              (char *);
int         RemoveDirectory         (char *);
int         RenameDirectory         (char *, char *);
int         RenameFile              (char *, char *);
int         PerformOperations       ();
int         Help                    ();
int         BulkDeleteDirectory     (char *);
int         CreateLog               (char *);
char *      GetErrorMessage         (int);

struct 
linux_dirent64 {
    ino64_t        d_ino;    /* 64-bit inode number */
    off64_t        d_off;    /* 64-bit offset to next structure */
    unsigned short d_reclen; /* Length of this dirent */
    unsigned char  d_type;   /* File type */
    char           d_name[]; /* Filename (null-terminated) */
};


// Main function
int 
main(int argc, char *argv[])
{
    if (argc == 1)
            ec = Help();
    else
    {
        ec = ProcessCommandLine(argv, argc);
        ec = PerformOperations();
    }
    return ec;
}

// function: ProcessCommandLine
//      This function takes the command line arguments and appropriately sets 
//      flags for which operations need to be performed. It also extracts 
//      appropiate pointers
//  @param: commandLineArguments - Pointer to array containing command line 
//          arguments
//  @param: argCount - Integer count of total  number of command line arguments
//  @return: Integer error code
int 
ProcessCommandLine(char *commandLineArguments[], int argCount)
{
    int argno = 1;
    while(argno < argCount)
    {
        switch (commandLineArguments[argno][1])
        {
        case 'c':
            fCreate = ENABLE;
            if (argno + 1 == argCount)
                return E_GENERAL;
            createPath = commandLineArguments[argno + 1];
            argno += 2;
            break;
        case 'd':
            fDelete = ENABLE;
            if (argno + 1 == argCount)
                return E_GENERAL;
            deletePath = commandLineArguments[argno + 1];
            argno += 2;
            break;
        case 'r':
            fRename = ENABLE;
            if (argno + 2 == argCount)
                return E_GENERAL;
            oldPath = commandLineArguments[argno + 1];
            newPath = commandLineArguments[argno + 2];
            argno += 3;
            break;
        case 'a':
            fAppend = ENABLE;
            if (argno + 1 == argCount)
                return E_GENERAL;
            appendPath = commandLineArguments[argno + 1];
            argno += 2;
            break;
        case 'l':
            fLog = ENABLE;
            if (argno + 1 == argCount)
                return E_GENERAL;
            logFileName = commandLineArguments[argno + 1];
            argno += 2;
            break;
        case 'e':
            fBinary = ENABLE;
            if (argno + 1 == argCount)
                return E_GENERAL;
            appendBuffer = commandLineArguments[argno + 1];
            argno += 2;
            break;
        case 's':
            if (argno + 1 == argCount)
                return E_GENERAL;
            appendBuffer = commandLineArguments[argno + 1];
            argno += 2;
            break;
        case 'f':
            fDirectory = ENABLE;
            argno += 1;
            break;
        default:
            return E_OK;
            break;
        }
    } 
    return E_OK; 
}



// function: Help
//      Provides a basic message on how to use the program.
//  @param: None
//  @return: Integer Error Code
int 
Help()
{
    char *helpMessage = "\tUsage:\n\t./my_bfm -c <Path> -a <TextFilePath> -s <string to append> OR -e <number to append> -r <OldPath> <NewPath> -d <Path> -l <log file>\n\tFor more info, please refer to README file\n";
    int length = strlen(helpMessage);
    int error = write(STDOUT_FILENO, helpMessage, length);
    if (error == E_GENERAL)
    {
        char *errorMessage = GetErrorMessage(errno);
        int errorCode = errno;
        char buffer[BUF_SIZE] = "\nFailed to print Help Message: ";
        errorMessage = strcat(buffer, errorMessage);
        if (fLog)
            error = CreateLog(errorMessage);
        return error;
    }

    else 
    {
        error = E_OK;
        if (fLog)
            error = CreateLog("\nPrinted Help Message");
        return error;
    }
}


//  function: PerformOperations
//      This function calls and executes appropriate functions based on
//      which flags were set after processing command line.
//  @param: None
//  @return: Integer Error Code
int 
PerformOperations()
{
    int status = E_OK;
    
    if (fCreate)
    {
        if(fDirectory)
        {
            status = CreateDirectory(createPath);
            if (status != E_OK)
                return status;
            fDirectory = DISABLE;
        }
        else 
        {
            status = CreateFile(createPath);
            if(status != E_OK)
                return status;
        }
    }
    if (fRename)
    {
        status = CheckDirectory(oldPath);   
        if (status != E_OK)
            return status;
        if (fDirectory)
        {
            status = RenameDirectory(oldPath, newPath);
            if (status != E_OK)
                return status;
            fDirectory = DISABLE;
        }
        else
        {
            status = RenameFile(oldPath, newPath);
            if (status != E_OK)
                return status;
        }
    }

    if (fAppend)
    {
        status = CheckDirectory(appendPath);
        if (status != E_OK)
            return status;

        if (!fDirectory)
        {
            if (fBinary)
            {
                int startNumber = strtol(appendBuffer, NULL, 0);    // Convert start number to int base 10
                AppendEvenNumbers(startNumber, appendPath);
                if (status != E_OK)
                    return status;
            }
            else
            {
                status = AppendText(appendBuffer, appendPath);
                if (status != E_OK)
                    return status;
            }
        }
        else
            fDirectory = DISABLE;
    }

    if (fDelete)
    {
        status = CheckDirectory(deletePath);
        if (status != E_OK)
            return status;

        if (fDirectory)
        {
            status = RemoveDirectory(deletePath);
            if (status != E_OK)
                return status;
            fDirectory = DISABLE;
        }
        else
        {
            status = RemoveFile(deletePath);
            if (status != E_OK)
                return status;
        }
    }
    return status;
}


//  function: CheckDirectory
//      This function checks if a given path is a file or directory and 
//      sets the fDirectory flag appropriately
//  @param: char pointer to the file path we want to check
//  @return: Integer error code
int 
CheckDirectory(char *filePath)
{
    struct stat fileInfo;
    int status = stat(filePath, &fileInfo);
    if (status == E_GENERAL)
    {
        if (fLog)
        {
            char *errorMessage = GetErrorMessage(errno);
            char buffer[BUF_SIZE] = "\nFailed to check directory: ";
            status = CreateLog(strcat(buffer, errorMessage));
            return status;
        }
        else return errno;
    }
    else 
    {
        if (fLog)
        {
            char buffer[1024] = "\nChecked if path is file or directory: ";
            char *message = strcat(buffer, filePath);
            status = CreateLog(message);
            if (status != E_OK)
            {
                return status;
            }
        }
        if(S_ISDIR(fileInfo.st_mode))
        {
            fDirectory = ENABLE;
            return E_OK;
        }
        else 
            return E_OK;
    }

}

//  function: NonBlockingOperation
//      This is somewhat of wrapper function used to call appropriate read/write system call
//      using function pointers. 
//  @param: Pointer to a function with the same declaration as read/write system call
//  @param: Integer flag to pass to system call
//  @param: pointer to file path
//  @param: buffer to use for writing/reading. Can be any data type.
//  @param: number of bytes to write/read.
//  @return: integer error code
int 
NonBlockingOperation(ssize_t (*operation) (int, void *, size_t), int flag, char *filePath, void* buffer, int noOfBytes, int permissions) //will cause warning with write function as argument, but this is not an issue
{
    int fd;
    if (strcmp(filePath, "stdout") == 0)
        fd = STDOUT_FILENO;
    else
    {
        if (permissions == -1)
            fd = open(filePath, flag | O_NONBLOCK);
        else
            fd = open(filePath, flag | O_NONBLOCK, permissions);

    }
    int status = E_OK;
    if (fd == E_GENERAL)
    {
        return errno;
    }
    else
    {
        status = (*operation)(fd, buffer, noOfBytes);
        if (status == E_GENERAL)
            status = errno;
        while (status == EAGAIN) // Repeat till call succeeds, can add extra code if we want to do something else while the operation is completing.
        {
            status = (*operation)(fd, buffer, noOfBytes);
            if (status == E_GENERAL)
            {
                status = errno;
                if (status != EAGAIN)
                {
                    if (fd <= STDERR_FILENO)
                        return status;
                    int error = close(fd);
                    if (error == E_GENERAL)
                        status = errno;
                    return status;
                }
            }
        }
        if (fd <= STDERR_FILENO)
            return E_OK;
        status = close(fd);
        if (status == E_GENERAL)
            status = errno;
        return status;
    }
}

// function: ApppendOddNumbers
//      Uses the non-blocking operation function to write the first 50 even numbers
//      starting from number specified by user to the command line to a file.
//  @param: Integer number to start from
//  @param: Pointer to file path
//  @return: Integer Error Code
int 
AppendEvenNumbers(int startNumber, char *filePath)
{
    short int evenNumbers[25];
    int bytesToWrite = 50;
    if (startNumber < 50) // To ensure at most 50 bytes get written
        return E_OK;
    else if (startNumber % 2 == 1)
        startNumber ++;
    for (int i = 0; i < 25; i ++)
    {
        if (startNumber > 199)
            {
                bytesToWrite = i * 2; // In case number exceeds 199, we stop writing
                break;
            }
        evenNumbers[i] = startNumber;
        startNumber += 2;
    }
    
    int status = NonBlockingOperation(&write, O_WRONLY | O_APPEND, filePath, evenNumbers, bytesToWrite, -1);
    if (status == E_OK)
    {
        if (fLog)
        {
            char buffer[BUF_SIZE] = "\nAppended even numbers to ";
            status = CreateLog(strcat(buffer, filePath));
        }
        
    }
    else if (fLog)
    {
        char *errorMessage = GetErrorMessage(errno);
        char buffer[BUF_SIZE] = "\nCould not append even numbers: ";
        status = CreateLog(strcat(buffer, errorMessage));
    }   
    return status;
}

// function: ApppendOddNumbers
//      Uses the non-blocking operation function to write the atmost 50 bytes given
//      by user to the command line to a file.
//  @param: Pointer to text to write
//  @param: Pointer to file path
//  @return: Integer Error Code
int 
AppendText(char *text, char* filePath) //Wrapper function
{
    int bytesToWrite = strlen(text);
    if (bytesToWrite > N_BYTES)
        bytesToWrite = N_BYTES; //To ensure at most 50 bytes are written
    int status = NonBlockingOperation(&write, O_WRONLY | O_APPEND, filePath, text, bytesToWrite, -1);
    if (status == E_OK)
    {
        if (fLog)
        {
            char buffer[BUF_SIZE] = "\nAppended text to ";
            status = CreateLog(strcat(buffer, filePath));
        }
    }
    else if (fLog)
    {
        char *errorMessage = GetErrorMessage(errno);
        char buffer[BUF_SIZE] = "\nCould not append text to ";
        status = CreateLog(strcat(strcat(strcat(buffer, filePath), ": "), errorMessage));
    }   
    return status;
}

//  function: CreateFile
//      Creates a file with specified file name
//  @param: pointer to file path you want to create
//  @return: Integer Error Code
int 
CreateFile(char *pathName)
{
    int fd = creat(pathName, S_IRWXU); // User has read, write, and execute access, can be made input based in the future
    int status = E_OK;
    if (fd == E_GENERAL)
    {
        if (fLog)
        {
            char *errorMessage = GetErrorMessage(errno);
            char buffer[BUF_SIZE] = "\nCould not create file ";
            status = CreateLog(strcat(strcat(strcat(buffer, pathName), ": "), errorMessage));   
            return status;
        }
        return errno;
    }
    else 
    {
        if (fLog)
        {
            char buffer[BUF_SIZE] = "\nSuccessfully created file: ";
            status = CreateLog(strcat(buffer, pathName));   
            if (status != E_OK)
            {
                int imStatus = close(fd);
                if (imStatus == E_GENERAL)
                    return errno;
                return status;
            }
        }
        status = close(fd);
        if (status == E_GENERAL)
            return errno;
        return status;
    }
    
}

//  function: RenameFile
//      Renames file using link and unlink
//  @param: Pointer to old file name / path
//  @param: Pointer to new file name / path
//  @return: Integer error code
int 
RenameFile(char *oldFilePath, char *newFilePath)
{
    int status = E_OK;
    if (link(oldFilePath, newFilePath) == E_GENERAL) // Done with link and unlink for learning purposes, can be done with rename system call
    {
        if (fLog)
        {
            char* errorMessage = GetErrorMessage(errno);
            char buffer[BUF_SIZE] = "\nCould not rename the file ";
            status = CreateLog(strcat(strcat(strcat(buffer, oldFilePath), ": "), errorMessage));
            return status;
        }
        return errno;
        
    }
    else 
    {
        if (unlink(oldFilePath) == E_GENERAL)
        {
            if (fLog)
            {
                char* errorMessage = GetErrorMessage(errno);
                char buffer[BUF_SIZE] = "\nCould not rename the file ";
                status = CreateLog(strcat(strcat(strcat(buffer, oldFilePath), ": "), errorMessage));
                return status;
            }
            return errno;   
        }
        else if (fLog)
        {
            char buffer[BUF_SIZE] = "\nSuccessfully renamed ";
            status = CreateLog(strcat(strcat(strcat(buffer, oldFilePath), " to "), newFilePath));
            return status;
        }
        else return E_OK;
    }
}

//  function: RenameDirectory
//      Renames directory using rename system call
//  @param: Pointer to old directory name / path
//  @param: Pointer to new directory name / path
//  @return: Integer error code
int 
RenameDirectory(char *oldDirPath, char *newDirPath)
{
    if (rename(oldDirPath, newDirPath) == E_GENERAL)
    {
        if (fLog)
        {
            char *errorMessage = GetErrorMessage(errno);
            char buffer[BUF_SIZE] = "\nCould not rename the directory ";
            int status = CreateLog(strcat(strcat(strcat(buffer, oldDirPath), ": "), errorMessage));
            return status;           
        }
        return errno;
    }
    else if (fLog)
    {
        char buffer[BUF_SIZE] = "\nSuccessfully rename the directory: ";
        int status = CreateLog(strcat(strcat(strcat(buffer, oldDirPath), " to "), newDirPath));
        return status;
    }
    else return E_OK;
}

//  function: CreateDirectory
//      Creates a new directory
//  @param: Pointer to pathname
//  @return: Integer error code
int 
CreateDirectory(char *pathName)
{
    int status = mkdir(pathName, S_IRWXU); // User has read, write, and execute access, can be made input based in the future
    if (status == E_GENERAL)
    {
        if (fLog)
        {
            char *errorMessage = GetErrorMessage(errno);
            char buffer[BUF_SIZE] = "\nCould not create the directory ";
            status = CreateLog(strcat(strcat(strcat(buffer, pathName), ": "), errorMessage));
            return status;
        }
        return errno;
    }
    else if (fLog)
    {
        char buffer[BUF_SIZE] = "\nSuccessfully created directory: ";
        status = CreateLog(strcat(buffer, pathName));
    }
    return status;
}   

//  function: RemoveFile
//      Deletes specified file if no other links exist, else unlinks
//  @param: pointer to file path to delete / unlink
//  @return: Integer error code
int 
RemoveFile(char *filePath)
{
    int status = unlink(filePath);
    if (status == E_GENERAL)
    {
        if (fLog)
        {
            char *errorMessage = GetErrorMessage(errno);
            char buffer[BUF_SIZE] = "\nCould not remove the file ";
            status = CreateLog(strcat(strcat(strcat(buffer, filePath), ": "), errorMessage));    
            return status;
        }
        return errno;
    }
    else if (fLog)
    {
        char buffer[BUF_SIZE] = "\nSuccessfully removed file: ";
        status = CreateLog(strcat(buffer, filePath));
    }
    return status;
}

//  function: RemoveDirectory
//      Deletes specified directory
//  @param: pointer to firectory path to delete
//  @return: Integer error code
int
RemoveDirectory(char *path)
{
    int status = rmdir(path);
    if (status == E_GENERAL)
    {
        if (errno == ENOTEMPTY)
        {
            goto notEmpty;
        }
        else 
        {
            if (fLog)
            {
                char *errorMessage = GetErrorMessage(errno);
                char buffer[BUF_SIZE] = "\nCould not remove the directory ";
                status = CreateLog(strcat(strcat(strcat(buffer, path), ": "), errorMessage)); 
                return status;
            } 
            return errno; 
        }

    }
    else
    {
        if (fLog)
        {
            char buffer[BUF_SIZE] = "\nSuccessfully removed directory and its contents: ";
            status = CreateLog(strcat(buffer, path));
        }
        return status;
    }
    notEmpty:
        status = BulkDeleteDirectory(path);
        if (status == E_OK)
            return RemoveDirectory(path); //To delete calling directory once it is empty
        else return status;
}



//  function: BulkDeleteDirectory
//      Walks through a directory and deletes all files and directories within it
//  @param: pointer to directory path to delete
//  @return: Integer error code
int
BulkDeleteDirectory(char *path)
{
    int fd;
    long nread;
    char buf[BUF_SIZE];
    struct linux_dirent64 *d;
    char d_type;
    struct stat fileInfo;
    char buffer[1024];
    fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd == -1)
        return errno;
    for (;;) 
    {
        nread = getdents64(fd, buf, BUF_SIZE);
        if (nread == -1)
            return errno;
        if (nread == 0)
            break;

        for (long bpos = 0; bpos < nread;) {
            d = (struct linux_dirent64 *) (buf + bpos);
            if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            {
                goto skip;
            }
            strcpy(buffer, path);
            strcat(buffer, "/");
            strcat(buffer, d->d_name);
            if (d->d_type == DT_DIR)
            {
                
                int error = RemoveDirectory(buffer);  //Repeating the process for child directory
                if(error != 0)
                    return error;

            }
            else
            {
                int status = RemoveFile(buffer); // Deleting any file in the directory
                if (status == E_GENERAL)
                    return errno;
            }
            skip:
                bpos += d->d_reclen;
        }
    }
}

//  function: CreateLog
//      Logs specified message to a log file
//  @param: pointer to message
//  @return: Integer error code
int
CreateLog(char *message)
{
    int status = NonBlockingOperation(&write, O_APPEND | O_WRONLY | O_CREAT, logFileName, message, strlen(message), S_IRWXU);
        return status;
}

//  function: GetErrorMessage
//      Returns Appropriate Error Message based on error code
//  @param: Integer error code defined in errno
//  @return: Pointer to message
char *GetErrorMessage(int errorCode) {
    switch (errorCode) {
        case 0: return "Success";
        case 1: return "Operation not permitted";
        case 2: return "No such file or directory";
        case 3: return "No such process";
        case 4: return "Interrupted system call";
        case 5: return "I/O error";
        case 6: return "No such device or address";
        case 7: return "Argument list too long";
        case 8: return "Exec format error";
        case 9: return "Bad file number";
        case 10: return "No child processes";
        case 11: return "Try again";
        case 12: return "Out of memory";
        case 13: return "Permission denied";
        case 14: return "Bad address";
        case 15: return "Block device required";
        case 16: return "Device or resource busy";
        case 17: return "File exists";
        case 18: return "Cross-device link";
        case 19: return "No such device";
        case 20: return "Not a directory";
        case 21: return "Is a directory";
        case 22: return "Invalid argument";
        case 23: return "File table overflow";
        case 24: return "Too many open files";
        case 25: return "Not a typewriter";
        case 26: return "Text file busy";
        case 27: return "File too large";
        case 28: return "No space left on device";
        case 29: return "Illegal seek";
        case 30: return "Read-only file system";
        case 31: return "Too many links";
        case 32: return "Broken pipe";
        case 33: return "Math argument out of domain of func";
        case 34: return "Math result not representable";
        case 35: return "Resource deadlock would occur";
        case 36: return "File name too long";
        case 37: return "No record locks available";
        case 38: return "Invalid system call number";
        case 39: return "Directory not empty";
        case 40: return "Too many symbolic links encountered";
        case 41: return "Operation would block";
        case 42: return "No message of desired type";
        case 43: return "Identifier removed";
        case 44: return "Channel number out of range";
        case 45: return "Level 2 not synchronized";
        case 46: return "Level 3 halted";
        case 47: return "Level 3 reset";
        case 48: return "Link number out of range";
        case 49: return "Protocol driver not attached";
        case 50: return "No CSI structure available";
        case 51: return "Level 2 halted";
        case 52: return "Invalid exchange";
        case 53: return "Invalid request descriptor";
        case 54: return "Exchange full";
        case 55: return "No anode";
        case 56: return "Invalid request code";
        case 57: return "Invalid slot";
        case 58: return GetErrorMessage(EDEADLK);
        case 59: return "Bad font file format";
        case 60: return "Device not a stream";
        case 61: return "No data available";
        case 62: return "Timer expired";
        case 63: return "Out of streams resources";
        case 64: return "Machine is not on the network";
        case 65: return "Package not installed";
        case 66: return "Object is remote";
        case 67: return "Link has been severed";
        case 68: return "Advertise error";
        case 69: return "Srmount error";
        case 70: return "Communication error on send";
        case 71: return "Protocol error";
        case 72: return "Multihop attempted";
        case 73: return "RFS specific error";
        case 74: return "Not a data message";
        case 75: return "Value too large for defined data type";
        case 76: return "Name not unique on network";
        case 77: return "File descriptor in bad state";
        case 78: return "Remote address changed";
        case 79: return "Can not access a needed shared library";
        case 80: return "Accessing a corrupted shared library";
        case 81: return ".lib section in a.out corrupted";
        case 82: return "Attempting to link in too many shared libraries";
        case 83: return "Cannot exec a shared library directly";
        case 84: return "Illegal byte sequence ";
        case 85: return "Interrupted system call should be restarted";
        case 86: return "Streams pipe error";
        case 87: return "Too many users";
        case 88: return "Socket operation on non-socket";
        case 89: return "Destination address required";
        case 90: return "Message too long";
        case 91: return "Protocol wrong type for socket";
        case 92: return "Protocol not available";
        case 93: return "Protocol not supported";
        case 94: return "Socket type not supported";
        case 95: return "Operation not supported on transport endpoint";
        case 96: return "Protocol family not supported";
        case 97: return "Address family not supported by protocol";
        case 98: return "Address already in use";
        case 99: return "Cannot assign requested address";
        case 100: return "Network is down";
        case 101: return "Network is unreachable";
        case 102: return "Network dropped connection because of reset";
        case 103: return "Software caused connection abort";
        case 104: return "Connection reset by peer";
        case 105: return "No buffer space available";
        case 106: return "Transport endpoint is already connected";
        case 107: return "Transport endpoint is not connected";
        case 108: return "Cannot send after transport endpoint shutdown";
        case 109: return "Too many references: cannot splice";
        case 110: return "Connection timed out";
        case 111: return "Connection refused";
        case 112: return "Host is down";
        case 113: return "No route to host";
        case 114: return "Operation already in progress";
        case 115: return "Operation now in progress";
        case 116: return "Stale file handle";
        case 117: return "Structure needs cleaning";
        case 118: return "Not a XENIX named type file";
        case 119: return "No XENIX semaphores available";
        case 120: return "Is a named type file";
        case 121: return "Remote I/O error";
        case 122: return "Quota exceeded";
        case 123: return "No medium found";
        case 124: return "Wrong medium type";
        case 125: return "Operation Canceled";
        case 126: return "Required key not available";
        case 127: return "Key has expired";
        case 128: return "Key has been revoked";
        case 129: return "Key was rejected by service";
        case 130: return "Owner died";
        case 131: return "State not recoverable";
        case 132: return "Operation not possible due to RF-kill";
        case 133: return "Memory page has hardware error";
        default:
            return "Invalid error code";
    }
}
