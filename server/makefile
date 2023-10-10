#################### Makefile #####################
 
################# Compiler Flags ##################
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g

############## Source & Executable ################
SRCS = *.c
EXEC = aesdsocket

##################### Targets #####################
default : $(EXEC)
all : $(EXEC)

$(EXEC): $(SRCS)
	$(CC) $(SRCS) $(CFLAGS) -o $(EXEC)

###################### Clean ######################
clean:
	-rm -rf *.o $(EXEC)
