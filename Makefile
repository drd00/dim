CXX = g++

CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

NCURSES_FLAGS = -lncurses

SOURCES = $(wildcard *.cpp)

OBJECTS = $(SOURCES:.cpp=.o)

EXECUTABLE = dim

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXECUTABLE) $(NCURSES_FLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: all clean run
