both: a1mon a1jobs

a1mon: shared_functions.cpp a1mon.cpp
	g++ shared_functions.cpp a1mon.cpp -o a1mon.o

a1jobs: linked_list.cpp shared_functions.cpp a1jobs.cpp
	g++ linked_list.cpp shared_functions.cpp a1jobs.cpp -o a1jobs.o

clean:
	rm -rf *.o; rm -rf .vscode

tar:
	tar -cvf submit.tar ./