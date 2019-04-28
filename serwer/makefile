
CXX=g++
CXXFLAGS=-O2 -Wall
CXXFLAGS+=-MP -MMD -Wextra -pedantic -std=c++11
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
DEPS=$(SRC:.cpp=.d)
EXE=Serwer
all: $(SRC) $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(CXXFLAGS)  $(OBJ) -o $@ -lssl -lcrypto

.PHONY: clean cleanall all val run class

clean: 
	rm -f $(OBJ)

cleanall:
	rm -f $(OBJ) $(EXE) $(DEPS)

val:
	valgrind --leak-check=full ./$(EXE)

run:
	./$(EXE)

#using 'make class name=(yourname)' will create a new header file and a cpp file for a new 
#class you wish to create
class:
	echo '#ifndef _$(name)_H_\n#define _$(name)_H_\n\nclass $(name)\n{\n\n};\n\n#endif' > $(name).h
	echo '#include "$(name).h"\n' > $(name).cpp

-include $(DEPS)
