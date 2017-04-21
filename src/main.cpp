#include "item_freq.h"
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <climits>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <glog/logging.h>

using std::cerr; using std::endl;

enum RunType {
    BUILD, LOAD
};

typedef ItemFreqDB<std::string>   StringFreqDB;
std::unique_ptr<StringFreqDB>     g_pFreqDB;

static uint32_t      g_nMinCount = 0;
static uint32_t      g_nMaxVocab = 0;
static uint32_t      g_nWindowSize = 0;
static uint32_t      g_nTopK = UINT_MAX;
static float         g_fMemorySize = 0.0;
static const char    *g_cstrInputData = NULL;
static const char    *g_cstrOutputData = NULL;
static int           g_eRunType = BUILD;

static inline
void print_usage()
{
    using namespace std;

    cerr << "Usage: " << endl;
    cerr << "For building frequency table from data file:" << endl;
    cerr << "\t" << "./itemfreq.bin build -i input_data_file -min-count N "
         << "[-max-vocab N] [-window-size 15(default)] " << "-topk N(default all) "
         << "[-memory 4.0(default)] -o output_data_file" << endl; 
    cerr << "For loading frequency table file from previous built:" << endl;
    cerr << "\t" << "./itemfreq.bin load -i data_file" << endl;
}


namespace Test {
    void print_args()
    {
        using namespace std;
        cerr << "g_nMinCount = " << g_nMinCount << endl;
        cerr << "g_nMaxVocab = " << g_nMaxVocab << endl;
        cerr << "g_nWindowSize = " << g_nWindowSize << endl;
        cerr << "g_fMemorySize = " << g_fMemorySize << endl;
        cerr << "g_cstrInputData = " << (g_cstrInputData ? g_cstrInputData : "NULL") << endl;
        cerr << "g_cstrOutputData = " << (g_cstrOutputData ? g_cstrOutputData : "NULL") << endl;
        cerr << "g_eRunType = " << (g_eRunType == BUILD ? "BUILD" : "LOAD") << endl;
    }
} // namespace Test

static
void parse_args( int argc, char **argv )
{
    auto print_and_exit = [] {
        print_usage();
        exit(0);
    };

    if (argc < 4)
        print_and_exit();

    char    *parg = NULL;
    char    optc;
    int     i;

    if (strcmp(argv[1], "build") == 0) {
        g_eRunType = BUILD;
        for (i = 2; i < argc;) {
            parg = argv[i];
            if ( *parg++ != '-' )
                print_and_exit();
            optc = *parg;
            if (!optc)
                print_and_exit();
            if (optc == 'i') {
                if (++i >= argc)
                    print_and_exit();
                g_cstrInputData = argv[i];
            } else if (optc == 'o') {
                if (++i >= argc)
                    print_and_exit();
                g_cstrOutputData = argv[i];
            } else if (strcmp(parg, "min-count") == 0) {
                if (++i >= argc)
                    print_and_exit();
                if (sscanf(argv[i], "%u", &g_nMinCount) != 1)
                    print_and_exit();
            } else if (strcmp(parg, "max-vocab") == 0) {
                if (++i >= argc)
                    print_and_exit();
                if (sscanf(argv[i], "%u", &g_nMaxVocab) != 1)
                    print_and_exit();
            } else if (strcmp(parg, "window-size") == 0) {
                if (++i >= argc)
                    print_and_exit();
                if (sscanf(argv[i], "%u", &g_nWindowSize) != 1)
                    print_and_exit();
            } else if (strcmp(parg, "topk") == 0) {
                if (++i >= argc)
                    print_and_exit();
                if (sscanf(argv[i], "%u", &g_nTopK) != 1)
                    print_and_exit();
            } else if (strcmp(parg, "memory") == 0) {
                if (++i >= argc)
                    print_and_exit();
                if (sscanf(argv[i], "%f", &g_fMemorySize) != 1)
                    print_and_exit();
            } else {
                print_and_exit();
            } // if

            ++i;
        } // for

    } else if (strcmp(argv[1], "load") == 0) {
        g_eRunType = LOAD;
        for (i = 2; i < argc;) {
            parg = argv[i];
            if ( *parg++ != '-' )
                print_and_exit();
            optc = *parg;
            if (!optc)
                print_and_exit();
            if (optc == 'i') {
                if (++i >= argc)
                    print_and_exit();
                g_cstrInputData = argv[i];
            } else {
                print_and_exit();
            } // if

            ++i;
        } // for
    } else {
        print_and_exit();
    } // if
}

static
void check_args()
{
    using namespace std;

    auto err_exit = [] (const char *msg) {
        cerr << msg << endl;
        exit(-1);
    };

    if (g_eRunType == BUILD) {
        if (!g_cstrInputData)
            err_exit( "arg error: no input data file specified." );
        if (!g_nMinCount)
            err_exit( "arg error: -min-count must be specified." );
    } else if (g_eRunType == LOAD) {
        if (!g_cstrInputData)
            err_exit( "arg error: no input data file specified." );
    } // if
}

static
void do_build_routine()
{
    using namespace std;

    typedef std::shared_ptr<std::string>    StringPtr;

    // defined in cooccur
    struct CREC {
        int       word1;
        int       word2;
        double    val;
    };

    const char *vocabOutFilename = "_vocab_count.txt";

    auto run_vocab_count = [&] {
        typedef boost::iostreams::stream< boost::iostreams::file_descriptor_source >
            FDStream;

        string vocabCmd, line;

        stringstream str;
        str << "./vocab_count.bin -verbose 0 ";
        str << "-min-count " << g_nMinCount;
        if (g_nMaxVocab)
            str << " -max-vocab " << g_nMaxVocab;
        str << " < " << g_cstrInputData << flush;
        vocabCmd = std::move(str.str());

        // LOG(INFO) << "vocabCmd = " << vocabCmd;
        ofstream vocabOutFile( vocabOutFilename, ios::out );
        if (!vocabOutFile)
            throw_runtime_error("Cannot open vocabOutFile for writting!");

        FILE *fp = ::popen(vocabCmd.c_str(), "r");
        if (!fp)
            throw_runtime_error("Launching vocab_count failed!");

        setlinebuf(fp);

        FDStream pipeStream( fileno(fp), boost::iostreams::never_close_handle );

        StringPtr    pWord;
        uint32_t     count;
        while (getline(pipeStream, line)) {
            // NOTE!! output on screen is stdout and stderr mixed
            // cerr << "line: " << line << endl;
            pWord.reset( new string );
            stringstream str(line);
            str >> *pWord >> count;
            g_pFreqDB->addItem( pWord, count );
            vocabOutFile << line << endl;
        } // while
        ::pclose(fp);
    };

    auto run_cooccur = [&] {
        string cooccurCmd;

        stringstream str;
        str << "./cooccur.bin -verbose 0 -noseq 1 -symmetric 0 -vocab-file "
                << vocabOutFilename;
        if (g_nWindowSize)
            str << " -window-size " << g_nWindowSize;
        if (g_fMemorySize >= 0.1)
            str << " -memory " << g_fMemorySize;
        str << " < " << g_cstrInputData << flush;

        cooccurCmd = std::move(str.str());
        // LOG(INFO) << "cooccurCmd = " << cooccurCmd;
        FILE *fp = ::popen(cooccurCmd.c_str(), "r");
        if (!fp)
            throw_runtime_error("Launching cooccur failed!");

        CREC rec;
        while ( fread(&rec, sizeof(CREC), 1, fp) == 1 ) {
            g_pFreqDB->addConcurItem(rec.word1, rec.word2, (uint32_t)(rec.val));
        } // while

        ::pclose(fp);

        // g_pFreqDB->checkConsistency();
    };

    auto sort_db = [&] {
        // TODO should use heap to keep top first
        // TODO openmp
        auto &items = g_pFreqDB->items();
        for (size_t i = 0; i < items.size(); ++i)
            sort_heap( items[i].concurItems.begin(), items[i].concurItems.end(), 
                    std::greater<StringFreqDB::ConcurItemInfo>() );
    };

    auto dump_db = [&]( ostream &os ) {
        for (uint32_t i = g_pFreqDB->minID(); i <= g_pFreqDB->maxID(); ++i) {
            const auto &item = g_pFreqDB->items()[i];
            // os << item.id << ":" << *(item.pItem) << ":" << item.count << "\t";
            os << *(item.pItem) << ":" << item.count << "\t";
            const auto &concurItems = item.concurItems;
            if (!concurItems.empty()) {
                for (uint32_t j = 0; j < concurItems.size(); ++j) {
                    os << concurItems[j].id << ":" 
                       << concurItems[j].condCount << ":"
                       << concurItems[j].condFreq << " ";
                } // for j
            } // if
            os << endl;
        } // for i
    };

    run_vocab_count();
    run_cooccur();
    g_pFreqDB->checkConsistency();
    sort_db();

    if (g_cstrOutputData) {
        ofstream ofs(g_cstrOutputData, ios::out);
        dump_db(ofs);
    } else {
        dump_db(cout);
    } // if
}

static
void do_load_routine()
{
}


int main( int argc, char **argv )
{
    using namespace std;

    parse_args( argc, argv );
    Test::print_args();
    check_args();

    try {
        google::InitGoogleLogging(argv[0]);

        g_pFreqDB.reset( new StringFreqDB(1, g_nTopK) );

        if (g_eRunType == BUILD)
            do_build_routine();
        else
            do_load_routine();

    } catch (const std::exception &ex) {
        cerr << "main caught exception: " << ex.what() << endl;
        exit(-1);
    } // try


    return 0;
}

