

all: parser

parser: parser.cpp scanner.cpp scanner.h parser.h semantic_routines.cpp semantic_routines.h
	g++ -o parser parser.cpp scanner.cpp semantic_routines.cpp

clean: 
	rm parser