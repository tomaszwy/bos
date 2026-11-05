CXX      = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra
TARGET   = bos
SRC      = main.cpp socket.h client.h tick.h

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)

format:
	clang-format -i $(SRC)

.PHONY: all run clean format
