#################### Makefile #####################
 
################# Compiler Flags ##################
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g
EXEC = writer

##################### Targets #####################
writer: writer.o
	$(CC) $(CFLAGS) -o $(EXEC) writer.o

writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c

###################### Clean ######################
clean:
	-rm *.o $(EXEC)