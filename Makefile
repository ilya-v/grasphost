CC = mingw32-g++.exe -static 

TARGET = grasphost

MACHINE = $(shell $(CC) -dumpmachine)
# Windows
ifneq (,$(or $(findstring mingw, $(MACHINE)), $(findstring cygwin, $(MACHINE))))
	PLATFORM = WIN
	LIBS = -lm -lsetupapi 
	RM = del
# POSIX
else
	PLATFORM = POSIX
	LIBS = -lm
endif

SRCSC := $(wildcard *.c)
SRCSCPP := $(wildcard *.cpp)
OBJS := $(SRCSC:.c=.o) $(SRCSCPP:.cpp=.o) 
DEPS := $(SRCS:.c=.d)  $(SRCSCPP:.cpp=.d)   

all: $(TARGET)

%.o: %.c
	$(CC) -O3 -Wall -c  -static  -static-libgcc -static-libstdc++  -mno-ms-bitfields -fmessage-length=0 -DPLATFORM_$(PLATFORM) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
    
%.o: %.cpp
	$(CC) -O3 -Wall -c -static  -static-libgcc -static-libstdc++ -mno-ms-bitfields -std=c++11   -fmessage-length=0 -DPLATFORM_$(PLATFORM) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"

$(TARGET): $(OBJS)
	@echo 'Building target: $@'
	$(CC) -o"$(TARGET)" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'

clean:
	-$(RM) $(OBJS) $(DEPS) $(TARGET)

.PHONY: all clean
