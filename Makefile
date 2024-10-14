
all : fakerm

fakerm.o : fakerm.c
	$(CC) -c $^ -o $@

fakerm : fakerm.o
	$(CC) $^ -o $@
