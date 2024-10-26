CC=gcc
CFLAGS=-I.
#CFLAGS=

OBJECTS = daytimeudpcliv6 daytimeudpsrvv6 daytimeudpsrvv6_lossy daytimeudpcliv_options 

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
