EXES = a1jobs a1mon a1

all: $(EXES)

clean:
	rm -rf *.o $(EXES)

tar: a1mon.cpp a1jobs.cpp ProjectReport.pdf Makefile
	tar -cvf submit.tar a1mon.cpp a1jobs.cpp ProjectReport.pdf Makefile

a1mon: a1mon.o
	g++ a1mon.o -o a1mon

a1jobs: a1jobs.o
	g++ a1jobs.o -o a1jobs

a1mon.o: a1mon.cpp
	g++ a1mon.cpp -c 

a1jobs.o: a1jobs.cpp
	g++ a1jobs.cpp -c

a1: a1.o
	g++ -g a1.o Command.o JobSpawner.o CommandFactory.o -o a1

a1.o: a1.cpp JobSpawner.cpp Command.cpp
	g++ -g a1.cpp CommandFactory.cpp Command.cpp JobSpawner.cpp -c

CommandFactory.o: 
	g++ CommandFactory.cpp -c

Command.o:
	g++ Command.cpp -c

JobSpawner.o:
	g++ JobSpawner.cpp -c 

