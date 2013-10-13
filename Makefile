CC = g++

TARGET = scan_example

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
	$(CC) -O3 -Wall -c -fmessage-length=0 -DPLATFORM_$(PLATFORM) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"

$(TARGET): $(OBJS)
	@echo 'Building target: $@'
	$(CC) -o"$(TARGET)" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'

clean:
	-$(RM) $(OBJS) $(DEPS) $(TARGET)

.PHONY: all clean
