FLAGS = -std=c++20 -fPIC -O2 -Wall -Wextra -Wno-missing-field-initializers -Wno-switch 
CXX = g++

TARGET_NAME = libambient3d_shared.so

SRC  = $(shell find ./src -type f -name *.cpp)

RAYLIB_PATH = "../raylib/build/raylib_modified/"
RAYLIB_HEADERS = "../raylib/src"


LIBS = -L$(RAYLIB_PATH) -Wl,-rpath,$(RAYLIB_PATH) -lraylib_modified \
	   -lGL -lm -lpthread -ldl -lrt -lX11 -llz4 -lssl -lcrypto

OBJS = $(SRC:.cpp=.o)


all: $(TARGET_NAME)


%.o: %.cpp
	@$(CXX) $(FLAGS) \
		-I$(RAYLIB_HEADERS) \
		-c $< -o $@ && (echo -e "\033[32m[Compiled]\033[0m $<") || (echo -e "\033[31m[Failed]\033[0m $<"; exit 1) 

$(TARGET_NAME): $(OBJS)
	@echo -e "\033[90mLinking...\033[0m"
	@$(CXX) $(OBJS) -shared -o $@ -L.. $(LIBS) && (echo -e "\033[36mDone.\033[0m"; ls -lh $(TARGET_NAME))

#$(TARGET_NAME): $(OBJS)
#	@ar rcs $@ $^ && (echo -e "\033[32mDone\033[0m"; ls -alh $(TARGET_NAME))

clean:
	@rm -v $(OBJS) $(TARGET_NAME)

.PHONY: all clean

