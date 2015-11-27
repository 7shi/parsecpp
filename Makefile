TARGET = cpp1    cpp2    cpp3    test \
         cpp1-03 cpp2-03 cpp3-03 \
         cpp1-11 cpp2-11 cpp3-11 \
         ref1    ref2    ref3    \
         parsec1 parsec2 parsec3

CXX11 = $(CXX) -std=c++11
HC    = ghc

all: $(TARGET)

cpp1: cpp1.cpp parsecpp.cpp
	$(CXX11) -o $@ $<
cpp2: cpp2.cpp parsecpp.cpp
	$(CXX11) -o $@ $<
cpp3: cpp3.cpp parsecpp.cpp
	$(CXX11) -o $@ $<
test: test.cpp parsecpp.cpp
	$(CXX11) -o $@ $<

cpp1-03: cpp1-03.cpp
	$(CXX) -o $@ $<
cpp2-03: cpp2-03.cpp
	$(CXX) -o $@ $<
cpp3-03: cpp3-03.cpp
	$(CXX) -o $@ $<

cpp1-11: cpp1-11.cpp
	$(CXX11) -o $@ $<
cpp2-11: cpp2-11.cpp
	$(CXX11) -o $@ $<
cpp3-11: cpp3-11.cpp
	$(CXX11) -o $@ $<

ref1: ref1.hs
	$(HC) -o $@ $<
ref2: ref2.hs
	$(HC) -o $@ $<
ref3: ref3.hs
	$(HC) -o $@ $<

parsec1: parsec1.hs
	$(HC) -o $@ $<
parsec2: parsec2.hs
	$(HC) -o $@ $<
parsec3: parsec3.hs
	$(HC) -o $@ $<

clean:
	rm -f $(TARGET) *.o *.hi *.exe
