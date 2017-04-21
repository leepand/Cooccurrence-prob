/*
 * c++ -o concur.bin main.cpp -lglog -lgflags -std=c++11 -pthread -O3 -Wall -g
 * ./concur.bin -id2word -in infile -out outfile
 * ./concur.bin -word2id -in infile -out outfile
 */
#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "concur_table.hpp"

DEFINE_bool(id2word, false, "convert id to word");
DEFINE_bool(word2id, false, "convert word to id");
DEFINE_string(in, "-", "input file");
DEFINE_string(out, "-", "output file");

// global vars
static ConcurTable      g_ConcurTable;

static
void do_id2word()
{
    g_ConcurTable.loadFromFileID(FLAGS_in);
    g_ConcurTable.dumpToFileWord(FLAGS_out);
}

static
void do_word2id()
{
    g_ConcurTable.loadFromFileWord(FLAGS_in);
    g_ConcurTable.dumpToFileID(FLAGS_out);
}


int main(int argc, char **argv)
try {
    using namespace std;

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_word2id)
        do_word2id();
    else if (FLAGS_id2word)
        do_id2word();
    else
        cerr << "You have to specify -word2id or -id2word" << endl;

    return 0;

} catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
}

