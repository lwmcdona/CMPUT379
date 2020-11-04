EXES = a4tasks

all: a4tasks

clean:
	rm -rf *.o $(EXES)
	rm -rf .vscode

tar: 
	tar -cvf submit.tar a4tasks.cpp libraries.h ProjectReport.pdf Makefile

a4tasks: a4tasks.cpp
	g++ a4tasks.cpp -lpthread -o a4tasks

checkfirst1: a4tasks ex1.txt
	valgrind --tool=drd --first-race-only=yes a4tasks ex1.txt 5000 5

checkfirst2: a4tasks ex2.txt
	valgrind --tool=drd --first-race-only=yes a4tasks ex2.txt 75 20

check1: a4tasks ex1.txt
	valgrind --tool=drd a4tasks ex1.txt 75 20

check2: a4tasks ex2.txt
	valgrind --tool=drd a4tasks ex2.txt 75 20