EXES = a3sdn

all: a3sdn

clean:
	rm -rf *.o $(EXES)
	rm -rf .vscode

tar: 
	tar -cvf submit.tar a3sdn.cpp libraries.h constants.h packets.h packets.cpp ProjectReport.pdf Makefile

a3sdn: a3sdn.cpp packets.cpp
	g++ a3sdn.cpp packets.cpp -o a3sdn
