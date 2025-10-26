# Debug library Makefile

CXX ?= g++
CXXFLAGS ?= -Wall -Wextra -Werror -std=c++98 -ggdb
INCLUDES = -I../../includes/

# Source files
SRCS = Debug.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

# Target library
TARGET = libdebug.a

# Default target
all: $(TARGET)

# Create static library
$(TARGET): $(OBJS)
	ar rcs $@ $^

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files
-include $(DEPS)

# Clean build files
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

fclean: clean

# Phony targets
.PHONY: all clean fclean

# Print variables (for debugging the Makefile)
print-%:
	@echo $* = $($*)
