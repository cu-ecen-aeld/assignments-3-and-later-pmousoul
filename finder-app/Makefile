EXEFILE=writer
# CROSS_COMPILE=
CROSS_COMPILE=aarch64-none-linux-gnu-
CC=$(CROSS_COMPILE)gcc

all:
	$(CC) $(EXEFILE).c -o $(EXEFILE) 

clean:
	rm -f $(EXEFILE)
	rm -f *.o