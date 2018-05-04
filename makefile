#CXX = g++
CC = gcc
CC_DBG_F = -ggdb
LD = gcc
NAME = trestra
_OBJ = main.o \
       utils.o \
       Task.o
OBJ_D = o
OBJ = $(patsubst %, $(OBJ_D)/%, $(_OBJ))
SRC_D = src
INC_D = inc
INC = -I $(INC_D) \
      -I lib/sqlite3
INC_O = lib/sqlite3/sqlite3.o
LIBS = -lncurses -lpthread -ldl

all: $(OBJ_D) $(NAME)

$(NAME): $(OBJ)
	echo "LD $@"
	$(LD) -o $@ $^ $(INC_O) $(LIBS)

$(OBJ_D):
	echo "MKDIR $@"
	mkdir -p $@

$(OBJ_D)/%.o: $(SRC_D)/%.c
	echo "CC $@"
	$(CC) -o $@ -c $(CC_DBG_F) $(INC) $<

#$(OBJ_D)/%.o: $(SRC_D)/%.cpp
#	echo "CXX $@"
#	$(CXX) -o $@ -c -I $(INC_D) $<

#-------------------------------------------------------------------------------
ctags:
	bash -c "ctags -R {src,inc}/* lib/*/*.{c,h}"

clean:
	rm -rfv $(OBJ_D)
	rm -vf $(NAME)
