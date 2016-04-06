src = main.c

all:ntrack

ntrack: $(src)
	$(CC) $(src) -o $@

.PHONY: clean all

clean:
	-rm -f *.o
	-rm -f ntrack
	