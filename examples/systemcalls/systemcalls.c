#include "systemcalls.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int status = system(cmd);

    if(status == -1) {
        fprintf(stderr, "Error running command: %s", cmd);

        return false;
    } else if(WIFEXITED(status)) {
        // terminated normally with a call to _exit(int status);
        int exitCode = WEXITSTATUS(status);
        if (exitCode == 0) {
            printf("Command executed successfully.\n");
            return true;
        } else {
            printf("Command terminated with exit status=%d\n", exitCode);
            return false;
        }
    } else {
        // Other termination cases
        printf("Command terminated abnormally.\n");
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    bool pass = true;
    int status;
    pid_t pid;

    // creates a new process
    pid = fork();

    if(pid == -1)
        pass = false;
    else if (pid == 0) {
        // This is the child process (pid == 0)
        
        // Replace the current process image with a new process
        // the process will execute the command inserted
        char *fullPathOfFileToBeExecuted = command[0];
        execv(fullPathOfFileToBeExecuted, command);

        // execv should not return, otherwise it is an error
        fprintf(stderr, "child: execv failed");
        // we may not just 'return' from a child process, we should call exit
        exit(1);
    } else {
        // This is the parent process
        // wait for the child to finish
        if (waitpid(pid, &status, 0) == -1) {
            fprintf(stderr, "parent: waitpid failed");
            pass = false;
        } else if(WIFEXITED(status)) {
            // Child terminated normally with a call to _exit(int status);
            int exitCode = WEXITSTATUS(status);
            if (exitCode == 0) {
                printf("parent: Command executed successfully.\n");
            } else {
                printf("parent: Command terminated with exit status=%d\n", exitCode);
                pass = false;
            }
        } else {
            // Other termination cases
            printf("parent: Command terminated abnormally.\n");
            pass = false;
        }
    }

    va_end(args);
    return pass;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    bool pass = true;
    int status;
    pid_t pid;

    // creates a new process
    pid = fork();

    if(pid == -1)
        pass = false;
    else if (pid == 0) {
        // This is the child process (pid == 0)

        // Open the output file where we will redirect stdout
        // O_WRONLY: For write only.
        // O_CREAT: Create the file if it does not exist.
        // O_TRUNC: Trunc the file to zero length if it exists.
        // S_IRUSR | S_IWUSR: Allow write and read for the user
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            fprintf(stderr, "child: failed to open output file");
            exit(1);
        }

        // Redirect stdout to the file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            fprintf(stderr, "child: dup2 failed");
            close(fd);
            exit(1);
        }

        // Close the file descriptor (no longer needed after dup2)
        close(fd);
        
        // Replace the current process image with a new process
        // the process will execute the command inserted
        char *fullPathOfFileToBeExecuted = command[0];
        execv(fullPathOfFileToBeExecuted, command);

        // execv should not return, otherwise it is an error
        fprintf(stderr, "child: execv failed");
        // we may not just 'return' from a child process, we should call exit
        exit(1);
    } else {
        // This is the parent process
        // wait for the child to finish
        if (waitpid(pid, &status, 0) == -1) {
            fprintf(stderr, "parent: waitpid failed");
            pass = false;
        } else if(WIFEXITED(status)) {
            // Child terminated normally with a call to _exit(int status);
            int exitCode = WEXITSTATUS(status);
            if (exitCode == 0) {
                printf("parent: Command executed successfully.\n");
            } else {
                printf("parent: Command terminated with exit status=%d\n", exitCode);
                pass = false;
            }
        } else {
            // Other termination cases
            printf("parent: Command terminated abnormally.\n");
            pass = false;
        }
    }

    va_end(args);
    return pass;
}
