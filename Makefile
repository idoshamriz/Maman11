all: binsem.a ut.a ph clean

FLAGS = -Wall -L./

ut.a:
	gcc $(FLAGS)  -c ut.c
	ar rcu libut.a ut.o
	ranlib libut.a 

binsem.a:
	gcc $(FLAGS) -c binsem.c
	ar rcu libbinsem.a binsem.o
	ranlib libbinsem.a 

ph: ph.c 
	gcc ${FLAGS} ph.c -lbinsem -lut -o ph

clean:
	rm -f *.o 
	rm -f a.out
	rm -f *~
	rm -f *a 
