CC=g++
CFLAGS=-c -Wall -g -pg
LDFLAGS=-g -pg
SOURCES=logger.cc common.cc status.cc index.cc list_index.cc bst_index.cc fb_file_manager.cc memory_manager.cc data_manager.cc hashdb.cc
SOURCES_MAIN=test.cc
SOURCES_TEST=util_test.cc test_felixdb.cc test_memory_management.cc test_main.cc
SOURCES_BENCH=db_bench_tree_db.cc
LIBS_TEST=gtest/libgtest.a
LIBS_BENCH=-L/usr/local/lib -lkyotocabinet -lz -lstdc++ -lrt -lpthread -lm -lc
INCLUDES_TEST=-I .
INCLUDES_BENCH=-I/usr/local/include
OBJECTS=$(SOURCES:.cc=.o)
OBJECTS_MAIN=$(SOURCES_MAIN:.cc=.o)
EXECUTABLE=db
EXECUTABLE_TEST=testdb
EXECUTABLE_BENCH=kyoto_bench
LIBRARY=felixdb.a

all: $(SOURCES) $(EXECUTABLE) $(LIBRARY)

test: $(EXECUTABLE)
	$(CC) $(LIBS_TEST) $(INCLUDES_TEST) $(SOURCES_TEST) $(OBJECTS) -o $(EXECUTABLE_TEST)

bench:
	$(CC) $(LIBS_BENCH) $(INCLUDES_BENCH) $(SOURCES_BENCH) -o $(EXECUTABLE_BENCH)

$(EXECUTABLE): $(OBJECTS) $(OBJECTS_MAIN)
	$(CC) $(LDFLAGS) $(OBJECTS) $(OBJECTS_MAIN) -o $@

$(LIBRARY): $(OBJECTS)
	rm -f $@
	ar -rs $@ $(OBJECTS)

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *~ *.o $(EXECUTABLE)

