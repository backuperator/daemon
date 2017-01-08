TARGET_EXEC ?= backuperator-daemon

BUILD_DIR ?= ./build
SRC_DIRS ?= helper src

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
LIBS := c++ cryptopp boost_system boost_filesystem glog protobuf

INC_DIRS := $(shell find $(SRC_DIRS) -type d) inc dependencies
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) -I/usr/local/include

LIB_DIRS := dependencies/cryptopp
LIB_FLAGS := $(addprefix -L,$(LIB_DIRS)) $(addprefix -l,$(LIBS))

CFLAGS ?= $(INC_FLAGS) -MMD -MP -msse4.2 -fno-omit-frame-pointer -g -fPIC
CPPFLAGS ?= $(CFLAGS) -std=c++11
LDFLAGS ?= -L/usr/local/lib -g -pthread $(LIB_FLAGS)


$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# C source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(CFLAGS) -c $< -o $@

# C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
