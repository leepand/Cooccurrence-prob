#ifndef _CONCUR_TABLE_HPP_
#define _CONCUR_TABLE_HPP_

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

#define THROW_RUNTIME_ERROR_IF(cond, args) \
    do { \
        if (cond) THROW_RUNTIME_ERROR(args); \
    } while (0)

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <deque>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdio>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>

template< typename StreamType >
bool bad_stream( StreamType &stream )
{ return (stream.fail() || stream.bad()); }


class ConcurTable {
public:
    typedef unsigned long                           IdType;
    typedef std::shared_ptr<std::string>            StringPtr;

    struct Keyword {
        // Keyword() = default;
        Keyword(const StringPtr &_Word, std::size_t _Freq)
                : pWord(_Word), freq(_Freq) {}

        // bool operator < (const Keyword &rhs) const
        // { return *pWord < *(rhs.pWord); }
        
        StringPtr   pWord;
        std::size_t freq;
    };

    struct ConcurItem {
        boost::variant<StringPtr, IdType> item;
        std::size_t                       freq;
        double                            weight;
    };

    typedef std::vector<ConcurItem>               ConcurItemList;
    typedef std::pair<Keyword, ConcurItemList>    TableItemType;
    typedef std::deque<TableItemType>             TableType;

    TableType& data()
    { return m_queTable; }

#if 0
    ConcurItemList& lookup( const std::string &key )
    {
        // NOTE!!! lookup pointer in this way
        std::string &_key = const_cast<std::string&>(key);
        StringPtr pKey(&_key, [](std::string*){ /* DLOG(INFO) << "fake delete"; */ });
        auto it = m_queTable.find( pKey );
        if (it == m_queTable.end())
            return m_vecEmptyList;
        return it->second;
    }
#endif

public:
    void loadFromFileID( const std::string &filename );
    void loadFromFileWord( const std::string &filename );
    void dumpToFileWord(const std::string &filename);
    void dumpToFileID(const std::string &filename);

    std::size_t getIdx(const StringPtr &pWord) const;

    std::size_t size() const
    { return m_queTable.size(); }

private:
    TableType                       m_queTable;
    std::map<StringPtr, TableType::iterator>    m_mapKeyIdx;
    ConcurItemList                  m_vecEmptyList;
};

inline
std::size_t ConcurTable::getIdx(const StringPtr &pWord) const
{
    auto it = std::find_if(m_queTable.begin(), m_queTable.end(),
            [&](const TableItemType &tItem)->bool{
                return *(tItem.first.pWord) == *pWord;
            });

    if (it == m_queTable.end())
        return (std::size_t)-1;
    return std::distance(m_queTable.begin(), it);
}

inline
void ConcurTable::loadFromFileWord( const std::string &filename )
{
    using namespace std;

    LOG(INFO) << "Running ConcurTable::loadFromFileWord() ...";

    std::shared_ptr<std::istream> pStream;
    if (filename == "-")
        pStream.reset(&cin, [](std::istream*){});
    else
        pStream.reset(new ifstream(filename, ios::out));
    THROW_RUNTIME_ERROR_IF(!(*pStream), "ConcurTable::loadFromFileWord() cannot read file " << filename);

    m_queTable.clear();
    m_queTable.emplace_back(std::make_pair(Keyword(std::make_shared<string>(""), 0), ConcurItemList()));

    string line, strItem;
    size_t freq = 0, keyFreq = 0;
    double weight = 0.0;
    size_t lineno = 0;
    while (getline(*pStream, line)) {
        DLOG(INFO) << "Parsing line " << ++lineno;
        stringstream stream(line);
        auto pKey = std::make_shared<std::string>();
        stream >> *pKey;
        if (bad_stream(stream)) continue;
        auto colon = pKey->rfind(':');
        if (colon == string::npos || colon == 0 || colon == pKey->size()-1)
            continue;
        // DLOG(INFO) << pKey->substr(colon + 1);
        keyFreq = boost::lexical_cast<size_t>(pKey->substr(colon + 1));
        pKey->erase(colon); pKey->shrink_to_fit();
        m_queTable.emplace_back(std::make_pair(Keyword(pKey, keyFreq), ConcurItemList()));
        auto &lst = m_queTable.back().second;
        while (stream >> strItem) {
            auto colon2 = strItem.find_last_of(':');
            if (colon2 == string::npos || colon2 == 0 || colon2 == pKey->size()-1)
                continue;
            auto colon1 = strItem.find_last_of(':', colon2-1);
            if (colon1 == string::npos || colon1 == 0 || colon1 == pKey->size()-1)
                continue;
            auto pWord = std::make_shared<std::string>();
            *pWord = strItem.substr(0, colon1);
            // DLOG(INFO) << "colon1 = " << colon1 << "  colon2 = " << colon2;
            // DLOG(INFO) << "pWord = " << *pWord;
            // DLOG(INFO) << strItem.substr(colon1 + 1, colon2 - colon1 - 1);
            freq = boost::lexical_cast<size_t>(strItem.substr(colon1 + 1, colon2 - colon1 - 1));
            // DLOG(INFO) << strItem.substr(colon2 + 1);
            weight = boost::lexical_cast<double>(strItem.substr(colon2 + 1));
            pWord->erase(colon1); pWord->shrink_to_fit();
            lst.emplace_back(ConcurItem{pWord, freq, weight});
            // ::getchar();
        } // while
    } // while

    // DLOG(INFO) << "m_queTable.size() = " << m_queTable.size();

    lineno = 0;
    auto tableSize = m_queTable.size();
    LOG(INFO) << "Building index ...";
    // for (auto it = m_queTable.begin()+1; it != m_queTable.end(); ++it) {
// #pragma omp parallel for
    for (size_t i = 1; i < tableSize; ++i) {
        DLOG(INFO) << "Building idx for " << ++lineno;
        auto &lst = m_queTable[i].second;
        for (auto &v : lst) {
            auto pWord = boost::get<StringPtr>(v.item);
            // DLOG(INFO) << "Finding idx for " << *pWord;
            auto idx = getIdx(pWord);
            if (idx == (size_t)-1) {
                m_queTable.emplace_back(std::make_pair(Keyword(pWord, v.freq), ConcurItemList()));
                idx = m_queTable.size()-1;
            } // if
            v.item = idx;
        } // for v
    } // for it

    // DLOG(INFO) << "Building idx done!";
}

inline
void ConcurTable::loadFromFileID( const std::string &filename )
{
    using namespace std;

    LOG(INFO) << "Running ConcurTable::loadFromFileID() ...";

    m_queTable.clear();
    m_queTable.emplace_back(std::make_pair(Keyword(std::make_shared<string>(""), 0), ConcurItemList()));

    std::shared_ptr<std::istream> pStream;
    if (filename == "-")
        pStream.reset(&cin, [](std::istream*){});
    else
        pStream.reset(new ifstream(filename, ios::out));
    THROW_RUNTIME_ERROR_IF(!(*pStream), "ConcurTable::loadFromFileID() cannot read file " << filename);

    string line, strItem;
    IdType id = 0;
    size_t freq = 0, keyFreq = 0;
    double weight = 0.0;
    while (getline(*pStream, line)) {
        stringstream stream(line);
        auto pKey = std::make_shared<std::string>();
        stream >> *pKey;
        auto colon = pKey->rfind(':');
        if (colon == string::npos || colon == 0 || colon == pKey->size()-1)
            continue;
        keyFreq = boost::lexical_cast<size_t>(pKey->substr(colon + 1));
        pKey->erase(colon);
        pKey->shrink_to_fit();
        // DLOG(INFO) << "pKey = " << *pKey;

        ConcurItemList cList;
        while (stream >> strItem) {
            if (sscanf(strItem.c_str(), "%lu:%lu:%lf", &id, &freq, &weight) != 3)
                continue;
            cList.emplace_back( ConcurItem{id, freq, weight} );
        } // while

        // assert( !cList.empty() );

        m_queTable.emplace_back(std::make_pair(Keyword(pKey, keyFreq), ConcurItemList()));
        m_queTable.back().second.swap(cList);
    } // while

    for (auto it = m_queTable.begin()+1; it != m_queTable.end(); ++it) {
        // DLOG(INFO) << "keyword = " << *(kv.first);
        for (auto &v : it->second) {
            auto idx = boost::get<IdType>(v.item);
            // DLOG(INFO) << "idx = " << idx;
            assert( idx != 0 && idx < m_queTable.size() );
            v.item = m_queTable[idx].first.pWord;
        } // for
    } // for

    // DEBUG
#if 0
    for (auto &kv : m_queTable) {
        // cout << *(kv.first) << "\t";
        DLOG(INFO) << "keyword = " << *(kv.first);
        auto &cList = kv.second;
        if (cList.size() > 1) {
            for (auto it = cList.begin(); it != cList.end()-1; ++it)
                if (it->weight < (it+1)->weight)
                { DLOG(INFO) << "found inconsistent record!"; break; }
        } // if
        // for (auto &v : kv.second)
            // cout << *(boost::get<StringPtr>(v.item)) << ":" << v.weight << " ";
        // cout << endl;
    } // for
#endif
}


inline
void ConcurTable::dumpToFileWord( const std::string &filename )
{
    using namespace std;

    LOG(INFO) << "Running ConcurTable::dumpToFileWord() ...";

    std::shared_ptr<std::ostream> pStream;
    if (filename == "-")
        pStream.reset(&cout, [](std::ostream*){});
    else
        pStream.reset(new ofstream(filename, ios::out));

    THROW_RUNTIME_ERROR_IF(!(*pStream), "ConcurTable::dumpToFile() cannot write file " << filename);

    for (auto it = m_queTable.begin()+1; it != m_queTable.end(); ++it) {
        // DLOG(INFO) << "keyword = " << *(it->first.pWord) << ":" << it->first.freq;
        *pStream << *(it->first.pWord) << ":" << it->first.freq << "\t";
        for (auto &v : it->second)
            *pStream << *(boost::get<StringPtr>(v.item)) << ":" << v.freq << ":" << v.weight << " ";
        *pStream << endl;
    } // for
}


inline
void ConcurTable::dumpToFileID( const std::string &filename )
{
    using namespace std;

    LOG(INFO) << "Running ConcurTable::dumpToFileID() ...";

    std::shared_ptr<std::ostream> pStream;
    if (filename == "-")
        pStream.reset(&cout, [](std::ostream*){});
    else
        pStream.reset(new ofstream(filename, ios::out));

    THROW_RUNTIME_ERROR_IF(!(*pStream), "ConcurTable::dumpToFileID() cannot write file " << filename);

    for (auto it = m_queTable.begin()+1; it != m_queTable.end(); ++it) {
        // DLOG(INFO) << "keyword = " << *(it->first.pWord) << ":" << it->first.freq;
        *pStream << *(it->first.pWord) << ":" << it->first.freq << "\t";
        for (auto &v : it->second)
            *pStream << boost::get<IdType>(v.item) << ":" << v.freq << ":" << v.weight << " ";
        *pStream << endl;
    } // for
}


#endif

