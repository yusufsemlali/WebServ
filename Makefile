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
HTTP_DIR             := $(SRC_DIR)/http
EVENT_DIR            := $(SRC_DIR)/event
PARSER_DIR           := $(SRC_DIR)/parser
DEBUG_DIR            := $(SRC_DIR)/debug
BONUS_PARSER_DIR     := $(SRC_DIR)/bonus_parser


# Sources
SRCS                 := $(SRC_DIR)/main.cpp
CORE_SRCS            := $(wildcard $(CORE_DIR)/*.cpp)
HTTP_SRCS            := $(wildcard $(HTTP_DIR)/*.cpp)
EVENT_SRCS           := $(wildcard $(EVENT_DIR)/*.cpp)
PARSER_SRCS          := $(wildcard $(PARSER_DIR)/*.cpp)
DEBUG_SRCS           := $(wildcard $(DEBUG_DIR)/*.cpp)
BONUS_PARSER_SRCS    := $(wildcard $(BONUS_PARSER_DIR)/*.cpp)

# Objects and Dependencies
OBJS                 := $(SRCS:.cpp=.o)
DEPS                 := $(OBJS:.o=.d) 

# Libraries
CORE_LIB             := $(CORE_DIR)/libcore.a
HTTP_LIB             := $(HTTP_DIR)/libhttp.a
EVENT_LIB            := $(EVENT_DIR)/libevent.a
PARSER_LIB           := $(PARSER_DIR)/libparser.a
DEBUG_LIB            := $(DEBUG_DIR)/libdebug.a
BONUS_PARSER_LIB     := $(BONUS_PARSER_DIR)/libbonusparser.a

LIBS                 := $(PARSER_LIB) $(HTTP_LIB) $(EVENT_LIB) $(CORE_LIB) $(DEBUG_LIB)
BONUS_LIBS           := $(BONUS_PARSER_LIB) $(HTTP_LIB) $(EVENT_LIB) $(CORE_LIB) $(DEBUG_LIB)

# Library Linking
LIB_PATH             := -L$(PARSER_DIR) -L$(HTTP_DIR) -L$(EVENT_DIR) -L$(CORE_DIR) -L$(DEBUG_DIR) -L/usr/local/lib
LIB_FLAGS            := -lparser -lhttp -lcore -levent -ldebug
BONUS_LIB_FLAGS      := -lbonusparser -lhttp -lcore -levent -ldebug

# Targets
all: $(NAME)
#	 ./$(NAME)  &

debug: CXXFLAGS += -DDEBUG
debug: re
# 	./$(NAME) $(CONFIG_FILE)

verbose: CXXFLAGS += -DVERBOSE_LOGGING
verbose: re

lite-verbose: CXXFLAGS += -DLITE_VERBOSE_LOGGING
lite-verbose: re

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

$(HTTP_LIB): $(HTTP_SRCS)
	$(MAKE) -C $(HTTP_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"

$(EVENT_LIB): $(EVENT_SRCS)
	$(MAKE) -C $(EVENT_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"

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
	rm -f valgrind*.log
	$(MAKE) -C $(PARSER_DIR) clean
	$(MAKE) -C $(HTTP_DIR) clean
	$(MAKE) -C $(EVENT_DIR) clean
	$(MAKE) -C $(CORE_DIR) clean
	$(MAKE) -C $(DEBUG_DIR) clean
	# $(MAKE) -C $(BONUS_PARSER_DIR) clean

fclean: clean
	rm -rf $(NAME) $(BONUS_NAME)
	$(MAKE) -C $(PARSER_DIR) fclean
	$(MAKE) -C $(HTTP_DIR) fclean
	$(MAKE) -C $(EVENT_DIR) fclean
	$(MAKE) -C $(CORE_DIR) fclean
	$(MAKE) -C $(DEBUG_DIR) fclean
	# $(MAKE) -C $(BONUS_PARSER_DIR) fclean

re: fclean all

# Sanitizers & Debugging
sanitize: CXXFLAGS += -fsanitize=address -g
sanitize: re

valgrind: CXXFLAGS += -g
valgrind: all
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--track-fds=yes \
		--trace-children=yes \
		--log-file=valgrind.log \
		--suppressions=cgi.supp \
		./$(NAME) 

# More comprehensive valgrind check for webserv-specific issues
valgrind-full: CXXFLAGS += -g -O0
valgrind-full: all
	valgrind --tool=memcheck \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--track-fds=yes \
		--trace-children=yes \
		--show-reachable=yes \
		--malloc-fill=0x42 \
		--free-fill=0x69 \
		--verbose \
		--log-file=valgrind-full.log \
		--error-exitcode=1 \
		--suppressions=valgrind.supp \
		--suppressions=cgi.supp \
		--gen-suppressions=all \
		./$(NAME) 

# Helgrind for threading issues (if using threads)
valgrind-helgrind: CXXFLAGS += -g -O0
valgrind-helgrind: all
	valgrind --tool=helgrind \
		--track-lockorders=yes \
		--history-level=full \
		--conflict-cache-size=16777216 \
		--log-file=helgrind.log \
		./$(NAME) $(CONFIG_FILE)

# Analyze valgrind output
analyze-valgrind:
	@./analyze_valgrind.sh


.PHONY: all debug verbose bonus clean fclean re sanitize valgrind valgrind-full valgrind-helgrind analyze-valgrind
.SECONDARY: $(OBJS)

