#include <stdio.h>      // Standard input/output library
#include <stdlib.h>     // Standard utility functions (e.g., exit())
#include <string.h>     // String manipulation functions (e.g., strlen())
#include <fcntl.h>      // File control options for system calls (e.g., open())
#include <unistd.h>     // POSIX API for system calls (e.g., write(), close())
#include <syslog.h>     // Syslog API for logging messages
#include <errno.h>      // Access to error numbers and error messages

// Enable or disable fprintf debugging messages
#define ENABLE_FPRINTF 0

int main(int argc, char *argv[]) {
    // Initialize syslog for logging.
    // NULL: Name of the log file, if using NULL, will use the application name (e.g "writer")
    // LOG_PID: Includes the process ID in the log messages.
    // LOG_CONS: Writes messages to the console if syslog fails.
    // LOG_USER: Specifies the log facility for user-level messages.
    openlog(NULL, LOG_PID | LOG_CONS, LOG_USER);

    // Check if the correct number of arguments is provided.
    // argc (argument count) must be 3: the program name, the file path, and the string to write.
    if (argc != 3) {
        // Log the error message to syslog with LOG_ERR level.
        syslog(LOG_ERR, "Error: Two arguments required: <file path> <string to write>");
#if ENABLE_FPRINTF
        // Print the error to stderr (standard error output) for user visibility.
        fprintf(stderr, "Error: Two arguments required: <file path> <string to write>\n");
#endif

        // Close the syslog connection before exiting the program.
        closelog();
        return 1; // Return an error code indicating failure.
    }

    // Extract the file path and string to write from the command-line arguments.
    const char *writefile = argv[1]; // First argument: file path
    const char *writestr = argv[2]; // Second argument: string to write

    // Open the file for writing using the open system call.
    // O_WRONLY: Open the file for write-only access.
    // O_CREAT: Create the file if it does not exist.
    // O_TRUNC: Truncate (overwrite) the file if it already exists.
    // Explanation of the S_I* flags:
    // - S_IRUSR: Read permission for the owner (user).
    // - S_IWUSR: Write permission for the owner (user).
    // - S_IRGRP: Read permission for the group.
    // - S_IROTH: Read permission for others (everyone else).
    // These flags ensure the file is created with permissions that allow the owner
    // to read and write, and others (group/others) can only read.
    int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    
    // Check if the open system call failed (returns -1 on failure).
    if (fd == -1) {
        // Log the error with LOG_ERR level and include the error message from strerror().
        syslog(LOG_ERR, "Error: Could not open file %s: %s", writefile, strerror(errno));     
#if ENABLE_FPRINTF
        // Print the error to stderr for user visibility.
        fprintf(stderr, "Error: Could not open file %s: %s\n", writefile, strerror(errno));
#endif        
        // Close the syslog connection before exiting.
        closelog();
        return 1; // Return an error code indicating failure.
    }

    // Write the string to the file using the write system call.
    // write(fd, buffer, size) writes `size` bytes from `buffer` to the file descriptor `fd`.
    ssize_t bytes_written = write(fd, writestr, strlen(writestr));
    
    // Check if the write system call failed (returns -1 on failure).
    if (bytes_written == -1) {
        // Log the error with LOG_ERR level and include the error message from strerror().
        syslog(LOG_ERR, "Error: Could not write to file %s: %s", writefile, strerror(errno));       
#if ENABLE_FPRINTF
        // Print the error to stderr for user visibility.
        fprintf(stderr, "Error: Could not write to file %s: %s\n", writefile, strerror(errno));
#endif        
        // Close the file descriptor before exiting.
        close(fd);
        
        // Close the syslog connection before exiting.
        closelog();
        return 1; // Return an error code indicating failure.
    }

    // Log a success message with LOG_DEBUG level indicating what was written and where.
    syslog(LOG_DEBUG, "Writing \"%s\" to \"%s\"", writestr, writefile);
#if ENABLE_FPRINTF
    fprintf(stdout, "Writing \"%s\" to \"%s\"\n", writestr, writefile);
#endif

    // Close the file descriptor using the close system call.
    // Always check the return value of close to ensure no errors occurred.
    if (close(fd) == -1) {
        // Log the error with LOG_ERR level if closing the file fails.
        syslog(LOG_ERR, "Error: Could not close file %s: %s", writefile, strerror(errno));
#if ENABLE_FPRINTF
        // Print the error to stderr for user visibility.
        fprintf(stderr, "Error: Could not close file %s: %s\n", writefile, strerror(errno));
#endif
        // Close the syslog connection before exiting.
        closelog();
        return 1; // Return an error code indicating failure.
    }

    // Close the syslog connection before exiting the program.
    closelog();
    
    // Return 0 to indicate success.
    return 0;
}
