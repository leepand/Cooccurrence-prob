#ifndef _ITEM_FREQ_H_
#define _ITEM_FREQ_H_

#include <memory>
#include <deque>
#include <iostream>
#include <algorithm>
#include "error.h"


template <typename ItemType>
class ItemFreqDB {
public:
    typedef typename std::shared_ptr<ItemType> ItemPtr;

    struct ConcurItemInfo {
        ConcurItemInfo() : id(0), condCount(0), condFreq(0.0) {}
        ConcurItemInfo( uint32_t _id, uint32_t _condCount, double _condFreq ) 
                : id(_id), condCount(_condCount), condFreq(_condFreq) {}

        bool operator < ( const ConcurItemInfo &rhs ) const
        { return (condFreq < rhs.condFreq); }
        bool operator > ( const ConcurItemInfo &rhs ) const
        { return (condFreq > rhs.condFreq); }

        uint32_t    id;
        uint32_t    condCount;
        double      condFreq;
    };

    struct ItemInfo {
        ItemInfo() : id(0), count(0) {}
        ItemInfo( const ItemPtr &_pItem, uint32_t _id, uint32_t _count ) 
                : pItem(_pItem), id(_id), count(_count) {}

        uint32_t                   id;
        ItemPtr                    pItem;
        uint32_t                   count;
        std::deque<ConcurItemInfo> concurItems;
    };

    typedef std::deque<ItemInfo>   ItemArray;

public:
    ItemFreqDB( uint32_t startID, uint32_t topK ) 
            : m_nStartID(startID)
            , m_nTopK(topK)
    { m_arrItems.resize(startID); }

    void addItem( const ItemPtr &pItem, uint32_t count )
    { 
        uint32_t id = size();
        m_arrItems.emplace_back(pItem, id, count); 
    }

    void addConcurItem( uint32_t mainId, uint32_t itemId, uint32_t condCount )
    {
        if (mainId < m_nStartID)
            return;

        if (mainId >= size())
            throw_runtime_error( std::stringstream() << "ItemFreqDB::addConcurItem() mainId "
                    << mainId << " out of range!" );

        ItemInfo &item = m_arrItems[mainId];
        double condFreq = (double)condCount / item.count;

        // smaller root heap
        auto &concurItems = item.concurItems;
        // concurItems.emplace_back( itemId, condCount, condFreq );
        if (concurItems.size() >= m_nTopK) {
            if (condFreq > concurItems.front().condFreq) {
                concurItems.front() = std::move( ConcurItemInfo(itemId, condCount, condFreq) );
                std::make_heap( concurItems.begin(), concurItems.end(), std::greater<ConcurItemInfo>() );
            } // if
        } else {
            concurItems.emplace_back( itemId, condCount, condFreq );
            std::push_heap( concurItems.begin(), concurItems.end(), std::greater<ConcurItemInfo>() );
        } // if
    }

    void checkConsistency() const
    {
        using namespace std;

        for (uint32_t i = m_nStartID; i < m_arrItems.size(); ++i) {
            if (m_arrItems[i].id != i)
                throw_runtime_error( stringstream() << "Element " << i << " id " << m_arrItems[i].id 
                        << " not same with its index!" );
        } // for
    }

    ItemArray& items()
    { return m_arrItems; }
    const ItemArray& items() const
    { return m_arrItems; }

    std::size_t size() const
    { return m_arrItems.size(); }

    const uint32_t minID() const
    { return m_nStartID; }
    const uint32_t maxID() const
    { return m_arrItems.back().id; }

private:
    const uint32_t m_nStartID;
    const uint32_t m_nTopK;
    ItemArray      m_arrItems;
};



#endif

