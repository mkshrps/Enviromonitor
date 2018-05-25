#define c compiler to use
CC = gcc
CXX = g++
#define the flags for the compiler
CFLAGS = -g -Wall 
#define any include directories other than /usr/include
INCLUDES = -I./  

# define library paths in addition to /usr/lib
#LFLAGS = -L../lib 

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
# LIBS = -lmylib -lm
LIBS = -lwiringPi -lcurl -lm

# define the C source files
SRCS = plantower.c  tempmon.c bme280.c bme280I2C.c bme280_selftest.c upload.c base64.c misc.c

# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

TARGET = emon

all: $(TARGET)
	@echo all files are done mjs

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(TARGET)



	