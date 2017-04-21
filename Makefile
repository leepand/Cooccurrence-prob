all: itemfreq

SRC = src/main.cpp

LIBS = -lglog -lboost_iostreams

FLAGS = -std=c++11 -O3 -g

itemfreq:
	c++ -o $@.bin $(SRC) $(LIBS) $(FLAGS)

clean:
	rm -rf itemfreq.bin itemfreq.bin.*

