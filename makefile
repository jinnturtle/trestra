#CXX = g++
CC = gcc
LD = gcc
NAME = trestra
_OBJ = main.o
OBJ_D = o
OBJ = $(patsubst %, %/$(_OBJ), $(OBJ_D))
SRC_D = src
INC_D = inc
INC = -I $(INC_D) \
      -I lib/sqlite3
INC_O = lib/sqlite3/sqlite3.o
LIBS = -lpthread -ldl

all: $(OBJ_D) $(NAME)

$(NAME): $(OBJ)
	$(LD) -o $(NAME) $^ $(INC_O) $(LIBS)

$(OBJ_D):
	mkdir -p $@

$(OBJ_D)/%.o: $(SRC_D)/%.c
	$(CC) -o $@ -c $(INC) $<

#$(OBJ_D)/%.o: $(SRC_D)/%.cpp
#	$(CXX) -o $@ -c -I $(INC_D) $<

#-------------------------------------------------------------------------------
ctags:
	bash -c "ctags -R {src,inc}/* lib/*/*.{c,h}"

clean:
	rm -rfv $(OBJ_D)
	rm -vf $(NAME)
