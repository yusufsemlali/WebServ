# Project Names
NAME         := webserv
BONUS_NAME   := webserv_bonus
CONFIG_FILE := configs/test.conf
# Compiler Settings
CXX          := g++
CXXFLAGS     := -Wall -Wextra -Werror -std=c++98 -ggdb
DFLAGS       := -MMD -MP
INCLUDES     := -I./includes -I/usr/local/include

# Directories
SRC_DIR              := src
CORE_DIR             := $(SRC_DIR)/core
PARSER_DIR           := $(SRC_DIR)/parser
DEBUG_DIR            := $(SRC_DIR)/debug
BONUS_PARSER_DIR     := $(SRC_DIR)/bonus_parser


# Sources
SRCS                 := $(SRC_DIR)/main.cpp
CORE_SRCS            := $(wildcard $(CORE_DIR)/*.cpp)
PARSER_SRCS          := $(wildcard $(PARSER_DIR)/*.cpp)
DEBUG_SRCS           := $(wildcard $(DEBUG_DIR)/*.cpp)
BONUS_PARSER_SRCS    := $(wildcard $(BONUS_PARSER_DIR)/*.cpp)

# Objects and Dependencies
OBJS                 := $(SRCS:.cpp=.o)
DEPS                 := $(OBJS:.o=.d) 

# Libraries
CORE_LIB						:= $(CORE_DIR)/libcore.a
PARSER_LIB           := $(PARSER_DIR)/libparser.a
DEBUG_LIB            := $(DEBUG_DIR)/libdebug.a
BONUS_PARSER_LIB     := $(BONUS_PARSER_DIR)/libbonusparser.a

LIBS                 := $(PARSER_LIB) $(CORE_LIB) $(DEBUG_LIB)
BONUS_LIBS           := $(BONUS_PARSER_LIB) $(CORE_LIB) $(DEBUG_LIB)

# Library Linking
LIB_PATH             := -L$(PARSER_DIR) -L$(CORE_DIR) -L$(DEBUG_DIR) -L/usr/local/lib
LIB_FLAGS            := -lparser -lcore -ldebug
BONUS_LIB_FLAGS      := -lbonusparser -lcore -ldebug

# Targets
all: $(NAME)
	./$(NAME) $(CONFIG_FILE)

debug: CXXFLAGS += -DDEBUG
debug: $(NAME)
	./$(NAME) $(CONFIG_FILE)

bonus: CXXFLAGS += -D BONUS
bonus: $(BONUS_NAME)

$(NAME): $(LIBS) $(OBJS) 
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIB_PATH) $(LIB_FLAGS) -o $@

$(BONUS_NAME): $(BONUS_LIBS) $(OBJS) 
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIB_PATH) $(BONUS_LIB_FLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DFLAGS) -c $< -o $@

# Sub-library builds
$(PARSER_LIB): $(PARSER_SRCS)
	$(MAKE) -C $(PARSER_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"

$(CORE_LIB): $(CORE_SRCS)
	$(MAKE) -C $(CORE_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"

$(DEBUG_LIB): $(DEBUG_SRCS)
	$(MAKE) -C $(DEBUG_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"

$(BONUS_PARSER_LIB): $(BONUS_PARSER_SRCS)
	$(MAKE) -C $(BONUS_PARSER_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"
-include $(DEPS)

# Clean Targets
clean:
	find . -name "*.o" -type f -delete
	find . -name "*.d" -type f -delete
	$(MAKE) -C $(PARSER_DIR) clean
	$(MAKE) -C $(CORE_DIR) clean
	$(MAKE) -C $(DEBUG_DIR) clean
	# $(MAKE) -C $(BONUS_PARSER_DIR) clean

fclean: clean
	rm -rf $(NAME) $(BONUS_NAME)
	$(MAKE) -C $(PARSER_DIR) fclean
	$(MAKE) -C $(CORE_DIR) fclean
	$(MAKE) -C $(DEBUG_DIR) fclean
	# $(MAKE) -C $(BONUS_PARSER_DIR) fclean

re: fclean all

# Sanitizers & Debugging
sanitize: CXXFLAGS += -fsanitize=address -g
sanitize: re

valgrind: CXXFLAGS += -g
valgrind: re
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(NAME)  $(CONFIG_FILE)


.PHONY: all debug bonus clean fclean re sanitize valgrind
.SECONDARY: $(OBJS)

