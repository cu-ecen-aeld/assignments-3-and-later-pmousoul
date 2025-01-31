/*
  A C version of the writer.sh script found in this folder.

  Two (2) arguments must be provided.
  Please provide a full path to a file (including filename) and
  a text string which will be written within this file as the second argument.

  Usage:
  ./writer <path_to_file> <string>

  Author: Panagiotis Mousouliotis
*/

#include <stdio.h>      // for printf()

#include <sys/types.h>  // for open()
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>     // for write()
#include <string.h>     // for strlen()

#include <syslog.h>

void initialize_logging(const char *app_name) {
  openlog(app_name, LOG_PID | LOG_CONS, LOG_USER);
  syslog(LOG_INFO, "Logging initialized for %s", app_name);
}

void log_message(int priority, const char *message) {
    syslog(priority, "%s", message);
}

int main(int argc, char**argv)
{
    
  /* expect 3 arguments (1st being the executable name) */
  if (argc != 3) {
    printf("Wrong argument number.\n");
    return 1;
  }

  /* save args to variables to make code more readable */
  const char* app_name = argv[0];
  const char* filename = argv[1];
  const char* content = argv[2];

  /* initialize syslog logging */
  initialize_logging(app_name);

  /* assume that target directory is created by the caller */

  /* create the file and write the 'content' string */
  int fd;
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    log_message(LOG_ERR, "File could not be created.");
    return 1;
  }

  /* write the 'content' string to the file */
  ssize_t nr;
  nr = write (fd, content, strlen(content));
  if (nr == -1) {
    log_message(LOG_ERR, "Content could not be written.");
    return 1;
  }
  else {
    char message[500];
    sprintf(message, "Writing %s to %s", content, filename);
    log_message(LOG_DEBUG, message);
  }

  /* closing the logging system */
  closelog();

  return 0;
}