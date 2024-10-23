# Variables
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra
LDFLAGS = -lexiv2
SRC = timestamp.cpp exif_file.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = timestamp

# Default rule to build the program
all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

# Rule to compile the source file into object file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
