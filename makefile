all: my_bfm

my_fm: my_bfm.c 
	gcc -Wall -o my_bfm my_bfm.c

clean:
	$(RM) my_bfm