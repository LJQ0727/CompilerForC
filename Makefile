

all: parser
run: parser
	./parser test1.c1
parser: parser.cpp scanner.cpp scanner.h parser.h
	g++ -o parser parser.cpp scanner.cpp

clean: 
	rm parser