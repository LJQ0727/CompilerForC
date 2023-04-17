

all: parser
run: parser
	./parser inp.txt
parser: parser.cpp scanner.cpp scanner.h parser.h
	g++ -g3 -o parser parser.cpp scanner.cpp

clean: 
	rm parser