
#define E_OK 0


// @authors: Prajas Naik and Kalash Shah
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h> // used only for rename system call
#include <string.h>
#include <sys/syscall.h>
#include <dirent.h>


// Global Variable for Error Code
int         ec          =           E_OK;

// Global Variable Flags for deciding which operation to perform
int         fCreate     =           DISABLE;
int         fWrite      =           DISABLE;
int         fDelete     =           DISABLE;
int         fRename     =           DISABLE;
int         fAppend     =           DISABLE;
int         fBinary     =           DISABLE;
int         fPath       =           DISABLE;
int         fDirectory  =           DISABLE;

// Buffers for storing paths for each function
char        *createPath;
char        *deletePath;
char        *oldPath;
char        *newPath;
char        *appendPath;
char        *writePath;
char        *appendBuffer;

// Buffer for storing values to read and write
char        readBuffer              [MAX_APPEND_SIZE];
char        writeBuffer             [MAX_APPEND_SIZE];

// Function Declarations
int         ProcessCommandLine      (char **, int);
int         CheckDirectory          (char *);
int         NonBlockingOperation    (ssize_t (*) (int, void *, size_t), int, char *, void*, int );
int         AppendOddNumbers        (int, char *);
int         AppendText              (char *, char*);
int         CreateFile              (char *);
int         CreateDirectory         (char *);
int         RemoveFile              (char *);
int         RemoveDirectory         (char *);
int         RenameDirectory         (char *, char *);
int         RenameFile              (char *, char *);
int         PerformOperations       ();
int         PrintFirstNBytes        (char *);
int         Help                    ();
int         BulkDeleteDirectory     (char *);

struct linux_dirent {
    unsigned long  d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    char           d_name[];
};

// Main function
int main(int argc, char *argv[])
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
int ProcessCommandLine(char *commandLineArguments[], int argCount)
{
    int argno = 1;
    while(argno < argCount)
    {
        switch (commandLineArguments[argno][1])
        {
        case 'c':
            fCreate = ENABLE;
            createPath = commandLineArguments[argno + 1];
            argno += 2;
            break;
        case 'w':
            fWrite = ENABLE;
            writePath = commandLineArguments[argno + 1]; 
            argno += 2;
            break;
        case 'd':
            fDelete = ENABLE;
            deletePath = commandLineArguments[argno + 1];
            argno += 2;
            break;
        case 'r':
            fRename = ENABLE;
            oldPath = commandLineArguments[argno + 1];
            newPath = commandLineArguments[argno + 2];
            argno += 3;
            break;
        case 'a':
            fAppend = ENABLE;
            appendPath = commandLineArguments[argno + 1];
            appendBuffer = commandLineArguments[argno + 2]; 
            argno += 3;
            break;
        case 'b':
            fBinary = ENABLE;
            argno += 1;
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
int Help()
{
    char *helpMessage = "\tUsage:\n\t./my_fm -c <Path> -w <Path> -a <TextFilePath> <string to append> -r <OldPath> <NewPath> -d <Path>\n\tFor more info, please refer to README file\n";
    int length = strlen(helpMessage);
    int error = write(STDOUT_FILENO, helpMessage, length);
    if (error == E_GENERAL)
        return errno;
    else return E_OK;
}


//  function: PerformOperations
//      This function calls and executes appropriate functions based on
//      which flags were set after processing command line.
//  @param: None
//  @return: Integer Error Code
int PerformOperations()
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
                AppendOddNumbers(startNumber, appendPath);
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

    if (fWrite)
    {
        status = CheckDirectory(writePath);
        if (status != E_OK)
            return status;
        if (!fDirectory)
        {
            status = PrintFirstNBytes(writePath);
            if (status != E_OK)
                return status;
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
int CheckDirectory(char *filePath)
{
    struct stat fileInfo;
    int error = stat(filePath, &fileInfo);
    if (error == E_GENERAL)
    {
        return errno;
    }
    else 
    {
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
int NonBlockingOperation(ssize_t (*operation) (int, void *, size_t), int flag, char *filePath, void* buffer, int noOfBytes) //will cause warning with write function as argument, but this is not an issue
{
    int fd;
    if (strcmp(filePath, "stdout") == 0)
        fd = STDOUT_FILENO;
    else
        fd = open(filePath, flag | O_NONBLOCK);
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
//      Uses the non-blocking operation function to write the first 50 odd numbers
//      starting from number specified by user to the command line to a file.
//  @param: Integer number to start from
//  @param: Pointer to file path
//  @return: Integer Error Code
int AppendOddNumbers(int startNumber, char *filePath)
{
    short int oddNumbers[25];
    int bytesToWrite = 50;
    if (startNumber < 50) // To ensure at most 50 bytes get written
        return E_OK;
    else if (startNumber % 2 == 0)
        startNumber ++;
    for (int i = 0; i < 25; i ++)
    {
        if (startNumber > 200)
            {
                bytesToWrite = i * 2; // In case number exceeds 200, we stop writing
                break;
            }
        oddNumbers[i] = startNumber;
        startNumber += 2;
    }
    int status = NonBlockingOperation(&write, O_WRONLY | O_APPEND, filePath, oddNumbers, bytesToWrite);
    return status;
}

// function: ApppendOddNumbers
//      Uses the non-blocking operation function to write the atmost 50 bytes given
//      by user to the command line to a file.
//  @param: Pointer to text to write
//  @param: Pointer to file path
//  @return: Integer Error Code
int AppendText(char *text, char* filePath) //Wrapper function
{
    int bytesToWrite = strlen(text);
    if (bytesToWrite > N_BYTES)
        bytesToWrite = N_BYTES; //To ensure at most 50 bytes are written
    int status = NonBlockingOperation(&write, O_WRONLY | O_APPEND, filePath, text, bytesToWrite);
    return status;
}

//  function: PrintFirstNBytes
//      Prints the first 50 bytes from a file to the command line using non-blocking
//      read function
//  @param: pointer to file name
//  @return: Integer Error Code
int PrintFirstNBytes(char *fileName)
{
    char buffer[MAX_APPEND_SIZE];
    int status = NonBlockingOperation(&read, O_RDONLY, fileName, buffer, N_BYTES);
    if (status != E_OK)
    {
        return status;
    }
    else
    {
        status = NonBlockingOperation(&write, 0, "stdout", buffer, N_BYTES);
        return status;
    }
}

//  function: CreateFile
//      Creates a file with specified file name
//  @param: pointer to file path you want to create
//  @return: Integer Error Code
int CreateFile(char *pathName)
{
    int fd = creat(pathName, S_IRWXU); // User has read, write, and execute access, can be made input based in the future
    if (fd == E_GENERAL)
    {
        return errno;
    }
    else 
    {
        int status = close(fd);
        if (status == E_GENERAL)
            return errno;
        return E_OK;
    }
    
}

//  function: RenameFile
//      Renames file using link and unlink
//  @param: Pointer to old file name / path
//  @param: Pointer to new file name / path
//  @return: Integer error code
int RenameFile(char *oldFilePath, char *newFilePath)
{
    if (link(oldFilePath, newFilePath) == E_GENERAL) // Done with link and unlink for learning purposes, can be done with rename system call
    {
        return errno;
    }
    else 
    {
        if (unlink(oldFilePath) == E_GENERAL)
        {
            return errno;
        }
        else return E_OK;
    }
}

//  function: RenameDirectory
//      Renames directory using rename system call
//  @param: Pointer to old directory name / path
//  @param: Pointer to new directory name / path
//  @return: Integer error code
int RenameDirectory(char *oldDirPath, char *newDirPath)
{
    if (rename(oldDirPath, newDirPath) == E_GENERAL)
        return errno;
    else return E_OK;
}

//  function: CreateDirectory
//      Creates a new directory
//  @param: Pointer to pathname
//  @return: Integer error code
int CreateDirectory(char *pathName)
{
    int status = mkdir(pathName, S_IRWXU); // User has read, write, and execute access, can be made input based in the future
    if (status == E_GENERAL)
    {
        return errno;
    }
    return E_OK;
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
        return errno;
    }
    else 
        return E_OK;
}

//  function: RemoveDirectory
//      Deletes specified directory if it is empty
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
        else return errno; 
    }
    else
        return E_OK;
    notEmpty:
        status = BulkDeleteDirectory(path);
        if (status == 0)
            return RemoveDirectory(path);
}

int
BulkDeleteDirectory(char *path)
{
    int fd;
    long nread;
    char buf[BUF_SIZE];
    struct linux_dirent *d;
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
            d = (struct linux_dirent *) (buf + bpos);
            d_type = *(buf + bpos + d->d_reclen - 1);
            if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            {
                goto skip;
            }
            strcpy(buffer, path);
            strcat(buffer, "/");
            strcat(buffer, d->d_name);
            if (d_type == DT_DIR)
            {
                
                int error = RemoveDirectory(buffer);
                if(error != 0)
                    return error;

            }
            else
            {
                RemoveFile(buffer);
            }
            skip:
                bpos += d->d_reclen;
        }
    }
}

