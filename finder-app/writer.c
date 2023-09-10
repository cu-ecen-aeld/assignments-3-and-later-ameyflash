#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    char * writefile;
    char * writestr;
    int file_open_ret;
    ssize_t write_file_value;

    openlog(NULL,0,LOG_USER);

    if(argc != 3)
    {
        syslog(LOG_ERR,"Invalid number of arguments : %d\n",argc);
        syslog(LOG_ERR,"Usage: $writer <writefile> <writestring>\n");
        return 1;
    }

    writefile = argv[1];
    writestr = argv[2];

    file_open_ret = open(writefile, (O_RDWR | O_CREAT | O_TRUNC),
           (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH ));
    if (file_open_ret == -1)
    {
        syslog(LOG_ERR,"Cannot open file : %s\n",writefile);
        return 1;
    }

    write_file_value = write(file_open_ret, writestr, strlen (writestr));
    if (write_file_value == -1)
    {
        syslog(LOG_ERR,"Write failed!!!");
        return 1;
    }

    syslog(LOG_DEBUG,"Writing %s to %s",writestr,writefile);

    return 0;
}