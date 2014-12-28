OS_VERSION:=$(shell uname)
ifneq (,$(findstring CYGWIN, $(OS_VERSION)))
 	CC = g++
else
	CC = mingw32-g++.exe
endif


TARGET = grasphost

MACHINE = $(shell $(CC) -dumpmachine)
# Windows
ifneq (,$(findstring mingw, $(MACHINE)))
	PLATFORM = WIN
	LIBS = -lm -lsetupapi -lws2_32
	RM = del
else 
ifneq (,$(findstring cygwin, $(MACHINE)))
	PLATFORM = POSIX
	LIBS = -lm
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
	$(CC) -O3 -Wall -c  -static  -static-libgcc -static-libstdc++  -mno-ms-bitfields -fmessage-length=0 -D_GLIBCXX_HAVE_BROKEN_VSWPRINTF -DPLATFORM_$(PLATFORM) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
    
%.o: %.cpp
	$(CC) -O3 -Wall -c -static  -static-libgcc -static-libstdc++ -mno-ms-bitfields -std=c++11 -D_GLIBCXX_HAVE_BROKEN_VSWPRINTF -fmessage-length=0 -DPLATFORM_$(PLATFORM) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"

$(TARGET): $(OBJS)
	@echo 'Building target: $@'
	$(CC) -o"$(TARGET)" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'

clean:
	-$(RM) $(OBJS) $(DEPS) $(TARGET)

.PHONY: all clean
