CC = gcc
CFLAGS =  -Wall -O1 -g

OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o
OBJS2 = mm.o memlib.o fcyc.o clock.o ftimer.o test_driver.o


mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

test_driver: $(OBJS2)
	$(CC) $(CFLAGS) -o test_driver $(OBJS2)

mm.o: mm.c mm.h memlib.h

test_driver.o: mm.c mm.h memlib.h test_driver.c 

clean:
	rm -f *~ mm.o mdriver test_driver.o test_driver


