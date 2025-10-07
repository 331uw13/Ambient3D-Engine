FLAGS = -std=c++20 -O2 -Wall -Wextra -Wno-missing-field-initializers -Wno-switch 
CXX = g++

TARGET_NAME = out

SRC  = $(shell find ./src -type f -name *.cpp)


# Temporary stuff... Probably better idea to install header files to the system?

RAYLIB_PATH = "../raylib/build/raylib_modified/"
RAYLIB_HEADERS = "../raylib/src"
AMBIENT3D_SHARED = "../shared"


LIBS = -L$(RAYLIB_PATH) -Wl,-rpath,$(RAYLIB_PATH) -lraylib_modified \
	   -lGL -lm -lpthread -ldl -lrt -lX11 -llz4 -lambient3d

OBJS = $(SRC:.cpp=.o)


all: $(TARGET_NAME)


%.o: %.cpp
	@$(CXX) $(FLAGS) \
		-I$(AMBIENT3D_SHARED) \
		-I$(RAYLIB_HEADERS) \
		-c $< -o $@ && (echo -e "\033[32m[Compiled]\033[0m $<") || (echo -e "\033[31m[Failed]\033[0m $<"; exit 1) 

$(TARGET_NAME): $(OBJS)
	@echo -e "\033[90mLinking...\033[0m"
	@$(CXX) $(OBJS) -o $@ -L.. $(LIBS) && (echo -e "\033[36mDone.\033[0m"; ls -lh $(TARGET_NAME))

#$(TARGET_NAME): $(OBJS)
#	@ar rcs $@ $^ && (echo -e "\033[32mDone\033[0m"; ls -alh $(TARGET_NAME))

clean:
	@rm -v $(OBJS) $(TARGET_NAME)

.PHONY: all clean

