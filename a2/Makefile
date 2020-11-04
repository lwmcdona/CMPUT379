EXES = a2sdn

all: a2sdn

clean:
	rm -rf *.o $(EXES)

tar: a2sdn.cpp ProjectReport.pdf Makefile
	tar -cvf submit.tar a2sdn.cpp ProjectReport.pdf Makefile

a2sdn: a2sdn.cpp
	g++ a2sdn.cpp -o a2sdn
