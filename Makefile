CXX := g++
CXXFLAGS := -std=gnu++17 -O3 -g -fno-omit-frame-pointer -Wall -Wextra -Werror
LDFLAGS :=

SRCDIR := src
INCDIR := include
BINDIR := bin
OBJDIR := obj

SOURCES := $(SRCDIR)/main.cpp $(SRCDIR)/executor.cpp
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

TARGET := $(BINDIR)/fuzzer

all: release

release: CXXFLAGS +=
release: LDFLAGS +=
release: dirs $(TARGET)

debug: CXXFLAGS += -O0 -g -fsanitize=address,undefined
debug: dirs $(TARGET)

dirs:
	@mkdir -p $(BINDIR) $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

re: clean
	make all

format:
	clang-format -i $(SOURCES)

.PHONY: all release debug clean dirs
