/*
 * Copyright (C) 2008-2010 Trinity <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef AUCTION_HOUSE_BOT_H
#define AUCTION_HOUSE_BOT_H

#include "Common.h"
#include "ObjectGuid.h"
#include <map>
#include "ItemTemplate.h"
#include "AHConfig.h"

struct AuctionEntry;
class Player;
class WorldSession;

enum class AHBotCommand : uint32
{
    ahexpire,
    minprice,
    maxprice,
    maxstack,
    buyerprice,
    bidinterval,
    bidsperinterval
};

class AuctionHouseBot
{
private:

    bool debug_Out;
    bool debug_Out_Filters;

    bool AHBSeller;
    bool AHBBuyer;
    bool BuyMethod;
    bool SellMethod;

    bool EnableAuctionsAlliance;
    bool EnableAuctionsHorde;
    bool EnableAuctionsNeutral;

    uint32 AHBplayerAccount;
    ObjectGuid::LowType AHBplayerGUID;
    uint32 ItemsPerCycle;

    //Begin Filters

    bool Vendor_Items;
    bool Loot_Items;

    bool No_Bind;
    bool Bind_When_Picked_Up;
    bool Bind_When_Equipped;
    bool Bind_When_Use;
    bool Bind_Quest_Item;

    bool DisableConjured;
    bool DisableMoneyLoot;

    bool DisableWarriorItems;
    bool DisablePaladinItems;
    bool DisableHunterItems;
    bool DisableRogueItems;
    bool DisablePriestItems;
    bool DisableDKItems;
    bool DisableShamanItems;
    bool DisableMageItems;
    bool DisableWarlockItems;
    bool DisableUnusedClassItems;
    bool DisableDruidItems;

    uint32 DisableItemsBelowLevel;
    uint32 DisableItemsAboveLevel;
    uint32 DisableTGsBelowLevel;
    uint32 DisableTGsAboveLevel;
    uint32 DisableItemsBelowGUID;
    uint32 DisableItemsAboveGUID;
    uint32 DisableTGsBelowGUID;
    uint32 DisableTGsAboveGUID;

    std::set<uint32> DisableItemStore;

    //End Filters

    AHBConfig AllianceConfig;
    AHBConfig HordeConfig;
    AHBConfig NeutralConfig;

    uint32 m_nConfigCounts;

    time_t _lastrun_a;
    time_t _lastrun_h;
    time_t _lastrun_n;

    inline uint32 minValue(uint32 a, uint32 b) { return a <= b ? a : b; };
    void addNewAuctions(Player *AHBplayer, AHBConfig *config);
    void addNewAuctionBuyerBotBid(Player *AHBplayer, AHBConfig *config, WorldSession *session);

//    friend class ACE_Singleton<AuctionHouseBot, ACE_Null_Mutex>;
    AuctionHouseBot();

public:
    static AuctionHouseBot* instance()
    {
        static AuctionHouseBot instance;
        return &instance;
    }

    ~AuctionHouseBot();

    void Update();
    void Initialize();
    void InitializeConfiguration();
    void LoadValues(AHBConfig *);
    bool LoadConfigFromDB();
    bool LoadNpcItemFromDB();
    bool LoadLootItemFromDB();
    void Commands(AHBotCommand, uint32, uint32, char*);
    ObjectGuid::LowType GetAHBplayerGUID() { return AHBplayerGUID; };

private:
    bool FilterByBonding(uint32 bonding);
    bool FilterByCharacterClass(uint32 Class);
    bool FilterByVendorItem(uint32 itemId);
    bool FilterByLootItem(uint32 itemId);
};

#define auctionbot AuctionHouseBot::instance()

#endif
