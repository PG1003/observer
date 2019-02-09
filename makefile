CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O3
INCLUDES = -I "./src"
LDFLAGS =

EXAMPLEDIR = examples
TESTDIR = test
OBJDIR = obj
OUTDIR = out

TESTSOURCES = $(shell find $(TESTDIR) -type f -name '*.cpp')
TESTOBJECTS = $(patsubst $(TESTDIR)/%.cpp, $(OBJDIR)/%.o, $(TESTSOURCES))
TESTS = $(patsubst $(OBJDIR)/%.o, $(OUTDIR)/%, $(TESTOBJECTS))

EXAMPLESOURCES = $(shell find $(EXAMPLEDIR) -type f -name '*.cpp')
EXAMPLEOBJECTS = $(patsubst $(EXAMPLEDIR)/%.cpp, $(OBJDIR)/%.o, $(EXAMPLESOURCES))
EXAMPLES = $(patsubst $(OBJDIR)/%.o, $(OUTDIR)/%, $(EXAMPLEOBJECTS))

.phony: clean tests examples

$(OBJDIR):
	test ! -d $(OBJDIR) && mkdir $(OBJDIR)
	
$(OUTDIR):
	test ! -d $(OUTDIR) && mkdir $(OUTDIR)
	
all: tests examples

tests:$(OBJDIR) $(OUTDIR) $(TESTS) 

$(TESTS): $(TESTOBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(patsubst $(OUTDIR)%, $(OBJDIR)%.o, $@)

$(TESTOBJECTS): $(TESTSOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(patsubst $(OBJDIR)%.o, $(TESTDIR)%.cpp, $@) -o $@

examples:$(OBJDIR) $(OUTDIR) $(EXAMPLES) 

$(EXAMPLES): $(EXAMPLEOBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(patsubst $(OUTDIR)%, $(OBJDIR)%.o, $@)

$(EXAMPLEOBJECTS): $(EXAMPLESOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(patsubst $(OBJDIR)%.o, $(EXAMPLEDIR)%.cpp, $@) -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(OUTDIR)
