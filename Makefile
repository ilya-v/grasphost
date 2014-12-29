OS_VERSION:=$(shell uname)
ifneq (,$(findstring CYGWIN, $(OS_VERSION)))
 	CC = g++
else
ifneq  (,$(findstring Darwin, $(OS_VERSION)))
	CC = gcc
else 
	CC = mingw32-g++.exe
endif
endif


TARGET = grasphost

COMP_FLAGS := 

MACHINE = $(shell $(CC) -dumpmachine)
# Windows
ifneq (,$(findstring mingw, $(MACHINE)))
	PLATFORM = WIN
	LIBS = -lm -lsetupapi -lws2_32
	RM = del
	COMP_FLAGS += -mno-ms-bitfields 
else 
ifneq (,$(findstring cygwin, $(MACHINE)))
	PLATFORM = POSIX
	LIBS = -lm
	COMP_FLAGS += -mno-ms-bitfields 
else
	PLATFORM = POSIX
	LIBS = -lm
endif
endif

SRCSC := $(wildcard *.c)
SRCSCPP := $(wildcard *.cpp)
OBJS := $(SRCSC:.c=.o) $(SRCSCPP:.cpp=.o) 
DEPS := $(SRCS:.c=.d)  $(SRCSCPP:.cpp=.d)   

all: $(TARGET)

%.o: %.c
	$(CC) $(COMP_FLAGS) -O3 -Wall -c  -static  -static-libgcc -static-libstdc++  -fmessage-length=0 -D_GLIBCXX_HAVE_BROKEN_VSWPRINTF -DPLATFORM_$(PLATFORM) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
    
%.o: %.cpp
	$(CC) $(COMP_FLAGS) -O3 -Wall -c -static  -static-libgcc -static-libstdc++ -mno-ms-bitfields -std=c++11 -D_GLIBCXX_HAVE_BROKEN_VSWPRINTF -fmessage-length=0 -DPLATFORM_$(PLATFORM) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"

$(TARGET): $(OBJS)
	@echo 'Building target: $@'
	$(CC) -o"$(TARGET)" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'

clean:
	-$(RM) $(OBJS) $(DEPS) $(TARGET)

.PHONY: all clean
