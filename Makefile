CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wno-deprecated-declarations -pthread -Iinclude
LDFLAGS = -lcrypto -lpthread

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
TEST_DIR = tests

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = mitmproxy

TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)
TEST_BINARIES = $(TEST_SOURCES:$(TEST_DIR)/%.cpp=%_test)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

test: $(TEST_BINARIES)
	@for test in $(TEST_BINARIES); do ./$$test; done

%_test: $(TEST_DIR)/%.cpp $(filter-out $(OBJ_DIR)/main.o, $(OBJECTS))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_BINARIES)

.PHONY: all test clean
