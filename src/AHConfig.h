#ifndef _MOD_AHCONFIG_CATEGORY_H
#define _MOD_AHCONFIG_CATEGORY_H

#include "Define.h"
#include <string>
#include <map>

#define GET_INDEX(x,y)      ((x<<8)+y)
#define GET_CLASS(x)        (x>>8)
#define GET_SUBCLASS(x)     (x&0xff)

// #define INDEX_GEM           GET_INDEX(ITEM_CLASS_GEM, 0)
// #define INDEX_GLYPH         GET_INDEX(ITEM_CLASS_GLYPH, 0) //((ITEM_CLASS_GLYPH<<16)+0)
// #define INDEX_ENCHANTMENT   GET_INDEX(ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_ITEM_ENHANCEMENT) //((ITEM_CLASS_CONSUMABLE<<16) + ITEM_SUBCLASS_ITEM_ENHANCEMENT)
// #define INDEX_WEAPON        GET_INDEX(ITEM_CLASS_WEAPON, 0) //((ITEM_CLASS_WEAPON<<16)+0)
// #define INDEX_ARMOR         GET_INDEX(ITEM_CLASS_ARMOR, 0) //((ITEM_CLASS_ARMOR<<16)+0)
// #define INDEX_ELEMENTAL     GET_INDEX(ITEM_CLASS_TRADE_GOODS, ITEM_SUBCLASS_ELEMENTAL) //((ITEM_CLASS_TRADE_GOODS<<16) + ITEM_SUBCLASS_ELEMENTAL)

#define AHB_GREY        0
#define AHB_WHITE       1
#define AHB_GREEN       2
#define AHB_BLUE        3
#define AHB_PURPLE      4
#define AHB_ORANGE      5
#define AHB_YELLOW      6
#define AHB_MAX_QUALITY 6
#define AHB_GREY_TG     0
#define AHB_WHITE_TG    1
#define AHB_GREEN_TG    2
#define AHB_BLUE_TG     3
#define AHB_PURPLE_TG   4
#define AHB_ORANGE_TG   5
#define AHB_YELLOW_TG   6
#define AHB_GREY_I      7
#define AHB_WHITE_I     8
#define AHB_GREEN_I     9
#define AHB_BLUE_I      10
#define AHB_PURPLE_I    11
#define AHB_ORANGE_I    12
#define AHB_YELLOW_I    13


struct AHConfigCategory
{
    uint32 ID;
    uint32 Class;                                           // id from ItemClass.dbc
    uint32 SubClass;                                        // id from ItemSubClass.dbc
    uint32 QualityMin;
    uint32 QualityMax;
    uint32 Count;
    std::string Name;
};


class AHBConfig
{
private:
    uint32 AHID;
    uint32 AHFID;

    uint32 minPriceGrey;
    uint32 maxPriceGrey;
    uint32 maxStackGrey;

    uint32 minPriceWhite;
    uint32 maxPriceWhite;
    uint32 maxStackWhite;

    uint32 minPriceGreen;
    uint32 maxPriceGreen;
    uint32 maxStackGreen;

    uint32 minPriceBlue;
    uint32 maxPriceBlue;
    uint32 maxStackBlue;

    uint32 minPricePurple;
    uint32 maxPricePurple;
    uint32 maxStackPurple;

    uint32 minPriceOrange;
    uint32 maxPriceOrange;
    uint32 maxStackOrange;

    uint32 minPriceYellow;
    uint32 maxPriceYellow;
    uint32 maxStackYellow;

    uint32 buyerPriceGrey;
    uint32 buyerPriceWhite;
    uint32 buyerPriceGreen;
    uint32 buyerPriceBlue;
    uint32 buyerPricePurple;
    uint32 buyerPriceOrange;
    uint32 buyerPriceYellow;
    
    uint32 buyerBiddingInterval;
    uint32 buyerBidsPerInterval;

    std::map<uint32, uint32> m_mItemCounts;

public:
    AHBConfig(uint32 ahid);

    void InitItemCounts(uint32 key, uint32 value);

    uint32 GetAHID(){ return AHID; }
    uint32 GetAHFID(){ return AHFID; }
    
    void SetMinPrice(uint32 color, uint32 value);
    uint32 GetMinPrice(uint32 color);
    void SetMaxPrice(uint32 color, uint32 value);
    uint32 GetMaxPrice(uint32 color);

    void SetMaxStack(uint32 color, uint32 value);
    uint32 GetMaxStack(uint32 color);

    void SetBuyerPrice(uint32 color, uint32 value);
    uint32 GetBuyerPrice(uint32 color);
    
    void SetBiddingInterval(uint32 value)
    {
        buyerBiddingInterval = value;
    }
    uint32 GetBiddingInterval()
    {
        return buyerBiddingInterval;
    }

    void DecItemCounts(uint32 Class, uint32 subClass, uint32 Quality);

    void IncItemCounts(uint32 Class, uint32 subClass, uint32 Quality)
    {
        AddItemCountsMap(Class, subClass);
    }

    void ResetItemCounts();

    void SetBidsPerInterval(uint32 value)
    {
        buyerBidsPerInterval = value;
    }
    uint32 GetBidsPerInterval()
    {
        return buyerBidsPerInterval;
    }

    std::map<uint32, uint32> GetItemCountsMap(){ return m_mItemCounts; }
    bool AddItemCountsMap(uint32 Class, uint32 subClass);

    ~AHBConfig(){}
};

#endif

