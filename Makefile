CXX := g++
CXXFLAGS_DEBUG := -std=c++20 -Wall -Wextra -g
CXXFLAGS_RELEASE := -std=c++20 -Wall -Wextra -O2
SRC_DIR := src
BUILD_DIR := build
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
TARGET := $(BUILD_DIR)/simulator

all: CXXFLAGS := $(CXXFLAGS_DEBUG)
all: $(TARGET)

release: CXXFLAGS := $(CXXFLAGS_RELEASE)
release: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all release clean
