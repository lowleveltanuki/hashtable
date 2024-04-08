#C
.PHONY:	all check clean debug profile

CC			:= gcc
BIN			:= hashtable

STANDARD	:= -std=c18
OPTIONS		:= -Wall -Wextra -Wpedantic -Waggregate-return -Wwrite-strings
OPTIONS		+= -Wvla -Wfloat-equal
DEBUG		:= -g3
PROFILE		:= -pg

SRC_DIR 	:= src
OBJ_DIR		:= obj
TST_DIR 	:= test

SRCS 		:= $(wildcard $(SRC_DIR)/*.c)
OBJS 		:= $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TSTS 		:= $(wildcard $(TST_DIR)/*.c)
TST_OBJS 	:= $(filter-out $(OBJ_DIR)/main.o, $(OBJS))
TST_OBJS 	+= $(patsubst $(TST_DIR)/%.c, $(OBJ_DIR)/%.o, $(TSTS))
TST_LIBS 	:= -lcheck -lm -lrt -lsubunit -pthread

CHECK		:= $(BIN)_test

all: $(BIN)

check: $(CHECK)

clean:
	@rm -rf $(BIN)* $(CHECK) $(OBJ_DIR)

debug: OPTIONS += $(DEBUG)
debug: $(BIN)

profile: OPTIONS += $(DEBUG) $(PROFILE)
profile: $(BIN)

$(OBJ_DIR):
	@mkdir -p $@

$(OBJS): | $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(STANDARD) $(OPTIONS) -g3 -c $< -o $@ $(LINKIN)

$(OBJ_DIR)/%.o: $(TST_DIR)/%.c
	$(CC) $(STANDARD) $(OPTIONS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(STANDARD) $(OPTIONS) $^ $(LDFLAGS) -o $@ $(LINKIN)

$(CHECK): $(TST_OBJS)
	$(CC) $(STANDARD) $(OPTIONS) $(DFLAGS) $^ $(LDFLAGS) -o $@ $(TST_LIBS)
	./$(CHECK)
