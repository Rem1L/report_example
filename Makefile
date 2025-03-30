CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11
TARGET = ril_ipc_test
SRCS = ril_ipc_test.cpp util_ril.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
