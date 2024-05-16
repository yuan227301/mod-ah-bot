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

#include "ObjectMgr.h"
#include "AuctionHouseMgr.h"
#include "AuctionHouseBot.h"
#include "Config.h"
#include "Player.h"
#include "WorldSession.h"
#include "GameTime.h"
#include "DatabaseEnv.h"
#include <vector>
#include <map>
#include "AHConfig.h"

using namespace std;
vector<uint32> npcItems;
vector<uint32> lootItems;

std::map<uint32, std::vector<uint32>> g_mapCategoryItems;

std::map<uint32, AHConfigCategory> g_configs;

AuctionHouseBot::AuctionHouseBot():AllianceConfig(2),HordeConfig(6),NeutralConfig(7)
{
    debug_Out = false;
    debug_Out_Filters = false;
    AHBSeller = false;
    AHBBuyer = false;

    //Begin Filters

    Vendor_Items = false;
    Loot_Items = false;

    No_Bind = false;
    Bind_When_Picked_Up = false;
    Bind_When_Equipped = false;
    Bind_When_Use = false;
    Bind_Quest_Item = false;

    DisableConjured = false;
    DisableMoneyLoot = false;

    DisableWarriorItems = false;
    DisablePaladinItems = false;
    DisableHunterItems = false;
    DisableRogueItems = false;
    DisablePriestItems = false;
    DisableDKItems = false;
    DisableShamanItems = false;
    DisableMageItems = false;
    DisableWarlockItems = false;
    DisableUnusedClassItems = false;
    DisableDruidItems = false;

    DisableItemsBelowLevel = 0;
    DisableItemsAboveLevel = 0;
    DisableTGsBelowLevel = 0;
    DisableTGsAboveLevel = 0;
    DisableItemsBelowGUID = 0;
    DisableItemsAboveGUID = 0;
    DisableTGsBelowGUID = 0;
    DisableTGsAboveGUID = 0;

    //End Filters

    _lastrun_a = time(NULL);
    _lastrun_h = time(NULL);
    _lastrun_n = time(NULL);

}

AuctionHouseBot::~AuctionHouseBot()
{
}

void AuctionHouseBot::addNewAuctions(Player *AHBplayer, AHBConfig *config)
{
    if (!AHBSeller)
    {
        LOG_TRACE("module", "AHSeller: Disabled");
        return;
    }

    if (m_nConfigCounts == 0)
    {
        LOG_DEBUG("module", "AuctionHouseBot: ConfigCounts is 0");
        return;
    }

    AuctionHouseEntry const* ahEntry =  sAuctionMgr->GetAuctionHouseEntry(config->GetAHFID());
    if (!ahEntry)
    {
        return;
    }
    AuctionHouseObject* auctionHouse =  sAuctionMgr->GetAuctionsMap(config->GetAHFID());
    if (!auctionHouse)
    {
        return;
    }

    uint32 auctions = auctionHouse->Getcount();
    if (auctions >= m_nConfigCounts)
    {
        LOG_TRACE("module", "AHSeller: Auctions above minimum");
        return;
    }

    uint32 items = 0;
    if ((m_nConfigCounts - auctions) >= ItemsPerCycle)
        items = ItemsPerCycle;
    else
        items = (m_nConfigCounts - auctions);

    LOG_DEBUG("module", "AHSeller: Start Adding {} Auctions, house id:", items, config->GetAHID());

    auto mapCount = config->GetItemCountsMap();

    // only insert a few at a time, so as not to peg the processor
    for (uint32 cnt = 1; cnt <= items; cnt++)
    {
        LOG_DEBUG("module", "AHSeller: {} count", cnt);

        uint32 itemID = 0;

        for (auto & pair : mapCount){

            if (pair.second >= g_configs[pair.first].Count) continue;

            const auto & items = g_mapCategoryItems[pair.first];
            if (items.size()==0) continue;

            itemID = items[urand(0, items.size() - 1)];

            pair.second++;
            break;
        }

        if (itemID == 0)
        {
            LOG_WARN("module", "AHSeller: ItemID is 0");
            return;
        }

        ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(itemID);
        if (prototype == NULL)
        {
            LOG_ERROR("module", "AHSeller: Huh?!?! prototype == NULL, item:{}", itemID);
            return;
        }

        Item* item = Item::CreateItem(itemID, 1, AHBplayer);
        if (item == NULL)
        {
            LOG_ERROR("module", "AHSeller: Item::CreateItem() returned NULL, item:{}", itemID);
            return;
        }
        item->AddToUpdateQueueOf(AHBplayer);

        uint32 randomPropertyId = Item::GenerateItemRandomPropertyId(itemID);
        if (randomPropertyId != 0)
            item->SetItemRandomProperties(randomPropertyId);

        // price 
        uint64 buyoutPrice = prototype->BuyPrice;
        uint64 bidPrice = 0;
        uint32 defaultPrice = 200 * prototype->Quality * prototype->ItemLevel;
        defaultPrice = defaultPrice > 0 ? defaultPrice : 10000;

        buyoutPrice = buyoutPrice > 0 ? buyoutPrice : defaultPrice;
        bidPrice = buyoutPrice * 80 / 100;

        buyoutPrice *= urand(config->GetMinPrice(prototype->Quality), config->GetMaxPrice(prototype->Quality));
        buyoutPrice /= 100;
        bidPrice = buyoutPrice * urand(70, 100);
        bidPrice /= 100;

        // stack count
        uint32 stackCount = 1;
        if (config->GetMaxStack(prototype->Quality) > 1 && item->GetMaxStackCount() > 1)
            stackCount = urand(1, minValue(item->GetMaxStackCount(), config->GetMaxStack(prototype->Quality)));
        else if (config->GetMaxStack(prototype->Quality) == 0 && item->GetMaxStackCount() > 1)
            stackCount = urand(1, item->GetMaxStackCount());

        uint32 etime = urand(0,2);
        etime = (43200<<etime);     //12 hours, 24 hours, 48 hours
        
        item->SetCount(stackCount);

        uint32 dep =  sAuctionMgr->GetAuctionDeposit(ahEntry, etime, item, stackCount);

        auto trans = CharacterDatabase.BeginTransaction();
        AuctionEntry* auctionEntry = new AuctionEntry();
        auctionEntry->Id = sObjectMgr->GenerateAuctionID();
        auctionEntry->houseId = config->GetAHID();
        auctionEntry->item_guid = item->GetGUID();
        auctionEntry->item_template = item->GetEntry();
        auctionEntry->itemCount = item->GetCount();
        auctionEntry->owner = AHBplayer->GetGUID();
        auctionEntry->startbid = bidPrice * stackCount;
        auctionEntry->buyout = buyoutPrice * stackCount;
        auctionEntry->bid = 0;
        auctionEntry->deposit = dep;
        auctionEntry->expire_time = (time_t) etime + time(NULL);
        auctionEntry->auctionHouseEntry = ahEntry;
        item->SaveToDB(trans);
        item->RemoveFromUpdateQueueOf(AHBplayer);
        sAuctionMgr->AddAItem(item);
        auctionHouse->AddAuction(auctionEntry);
        auctionEntry->SaveToDB(trans);
        CharacterDatabase.CommitTransaction(trans);

    }// for
}
void AuctionHouseBot::addNewAuctionBuyerBotBid(Player *AHBplayer, AHBConfig *config, WorldSession *session)
{
    if (!AHBBuyer)
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBuyer: Disabled");
        return;
    }

    QueryResult result = CharacterDatabase.Query("SELECT id FROM auctionhouse WHERE itemowner<>{} AND buyguid<>{}", AHBplayerGUID, AHBplayerGUID);

    if (!result)
        return;

    if (result->GetRowCount() == 0)
        return;

    // Fetches content of selected AH
    AuctionHouseObject* auctionHouse =  sAuctionMgr->GetAuctionsMap(config->GetAHFID());
    vector<uint32> possibleBids;

    do
    {
        uint32 tmpdata = result->Fetch()->Get<uint32>();
        possibleBids.push_back(tmpdata);
    }while (result->NextRow());

    for (uint32 count = 1; count <= config->GetBidsPerInterval(); ++count)
    {
        // Do we have anything to bid? If not, stop here.
        if (possibleBids.empty())
        {
            //if (debug_Out) sLog->outError( "AHBuyer: I have no items to bid on.");
            count = config->GetBidsPerInterval();
            continue;
        }

        // Choose random auction from possible auctions
        uint32 vectorPos = urand(0, possibleBids.size() - 1);
        vector<uint32>::iterator iter = possibleBids.begin();
        advance(iter, vectorPos);

        // from auctionhousehandler.cpp, creates auction pointer & player pointer
        AuctionEntry* auction = auctionHouse->GetAuction(*iter);

        // Erase the auction from the vector to prevent bidding on item in next iteration.
        possibleBids.erase(iter);

        if (!auction)
            continue;

        // get exact item information
		Item *pItem = sAuctionMgr->GetAItem(auction->item_guid);
        if (!pItem)
        {
			if (debug_Out)
                LOG_ERROR("module", "AHBuyer: Item {} doesn't exist, perhaps bought already?", auction->item_guid.ToString());
            continue;
        }

        // get item prototype
        ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(auction->item_template);

        // check which price we have to use, startbid or if it is bidded already
        uint32 currentprice;
        if (auction->bid)
            currentprice = auction->bid;
        else
            currentprice = auction->startbid;

        // Prepare portion from maximum bid
        double bidrate = static_cast<double>(urand(1, 100)) / 100;
        long double bidMax = 0;

        // check that bid has acceptable value and take bid based on vendorprice, stacksize and quality
        if (BuyMethod)
        {
            if (prototype->Quality <= AHB_MAX_QUALITY)
            {
                if (currentprice < prototype->SellPrice * pItem->GetCount() * config->GetBuyerPrice(prototype->Quality))
                    bidMax = prototype->SellPrice * pItem->GetCount() * config->GetBuyerPrice(prototype->Quality);
            }
            else
            {   // quality is something it shouldn't be, let's get out of here
                LOG_ERROR("module", "AHBuyer: Quality {} not Supported", prototype->Quality);
                continue;
            }
        }
        else
        {
            if (prototype->Quality <= AHB_MAX_QUALITY)
            {
                if (currentprice < prototype->BuyPrice * pItem->GetCount() * config->GetBuyerPrice(prototype->Quality))
                    bidMax = prototype->BuyPrice * pItem->GetCount() * config->GetBuyerPrice(prototype->Quality);
            }
            else
            {
                // quality is something it shouldn't be, let's get out of here
                LOG_ERROR("module", "AHBuyer: Quality {} not Supported", prototype->Quality);
                continue;
            }
        }

        // check some special items, and do recalculating to their prices
        switch (prototype->Class)
        {
            // ammo
        case 6:
            bidMax = 0;
            break;
        default:
            break;
        }

        if (bidMax == 0)
        {
            // quality check failed to get bidmax, let's get out of here
            continue;
        }

        // Calculate our bid
        long double bidvalue = currentprice + ((bidMax - currentprice) * bidrate);
        // Convert to uint32
        uint32 bidprice = static_cast<uint32>(bidvalue);

        // Check our bid is high enough to be valid. If not, correct it to minimum.
        if ((currentprice + auction->GetAuctionOutBid()) > bidprice)
            bidprice = currentprice + auction->GetAuctionOutBid();


        if (debug_Out)
        {
            LOG_INFO("module", "-------------------------------------------------");
            LOG_INFO("module", "AHBuyer: Info for Auction #{}:", auction->Id);
            LOG_INFO("module", "AHBuyer: AuctionHouse: {}", auction->GetHouseId());
            LOG_INFO("module", "AHBuyer: Owner: {}", auction->owner.ToString());
            LOG_INFO("module", "AHBuyer: Bidder: {}", auction->bidder.ToString());
            LOG_INFO("module", "AHBuyer: Starting Bid: {}", auction->startbid);
            LOG_INFO("module", "AHBuyer: Current Bid: {}", currentprice);
            LOG_INFO("module", "AHBuyer: Buyout: {}", auction->buyout);
            LOG_INFO("module", "AHBuyer: Deposit: {}", auction->deposit);
            LOG_INFO("module", "AHBuyer: Expire Time: {}", uint32(auction->expire_time));
            LOG_INFO("module", "AHBuyer: Bid Rate: {}", bidrate);
            LOG_INFO("module", "AHBuyer: Bid Max: {}", bidMax);
            LOG_INFO("module", "AHBuyer: Bid Value: {}", bidvalue);
            LOG_INFO("module", "AHBuyer: Bid Price: {}", bidprice);
            LOG_INFO("module", "AHBuyer: Item GUID: {}", auction->item_guid.ToString());
            LOG_INFO("module", "AHBuyer: Item Template: {}", auction->item_template);
            LOG_INFO("module", "AHBuyer: Item Info:");
            LOG_INFO("module", "AHBuyer: Item ID: {}", prototype->ItemId);
            LOG_INFO("module", "AHBuyer: Buy Price: {}", prototype->BuyPrice);
            LOG_INFO("module", "AHBuyer: Sell Price: {}", prototype->SellPrice);
            LOG_INFO("module", "AHBuyer: Bonding: {}", prototype->Bonding);
            LOG_INFO("module", "AHBuyer: Quality: {}", prototype->Quality);
            LOG_INFO("module", "AHBuyer: Item Level: {}", prototype->ItemLevel);
            LOG_INFO("module", "AHBuyer: Ammo Type: {}", prototype->AmmoType);
            LOG_INFO("module", "-------------------------------------------------");
        }

        // Check whether we do normal bid, or buyout
        if ((bidprice < auction->buyout) || (auction->buyout == 0))
        {
            if (auction->bidder)
            {
                if (auction->bidder == AHBplayer->GetGUID())
                {
                    //pl->ModifyMoney(-int32(price - auction->bid));
                }
                else
                {
                    // mail to last bidder and return money
                    auto trans = CharacterDatabase.BeginTransaction();
                    sAuctionMgr->SendAuctionOutbiddedMail(auction, bidprice, session->GetPlayer(), trans);
                    CharacterDatabase.CommitTransaction(trans);
                    //pl->ModifyMoney(-int32(price));
                }
           }

            auction->bidder = AHBplayer->GetGUID();
            auction->bid = bidprice;

            // Saving auction into database
            CharacterDatabase.Execute("UPDATE auctionhouse SET buyguid = '{}',lastbid = '{}' WHERE id = '{}'", auction->bidder.GetCounter(), auction->bid, auction->Id);
        }
        else
        {
            auto trans = CharacterDatabase.BeginTransaction();
            //buyout
            if ((auction->bidder) && (AHBplayer->GetGUID() != auction->bidder))
            {
                sAuctionMgr->SendAuctionOutbiddedMail(auction, auction->buyout, session->GetPlayer(), trans);
            }
            auction->bidder = AHBplayer->GetGUID();
            auction->bid = auction->buyout;

            // Send mails to buyer & seller
            //sAuctionMgr->SendAuctionSalePendingMail(auction, trans);
            sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
            sAuctionMgr->SendAuctionWonMail(auction, trans);
            auction->DeleteFromDB(trans);

			sAuctionMgr->RemoveAItem(auction->item_guid);
            auctionHouse->RemoveAuction(auction);
            CharacterDatabase.CommitTransaction(trans);
        }
    }
}

void AuctionHouseBot::Update()
{
    time_t _newrun = time(NULL);
    if ((!AHBSeller) && (!AHBBuyer))
        return;
    if (!EnableAuctionsAlliance && !EnableAuctionsHorde && !EnableAuctionsNeutral)
        return;

    std::string accountName = "AuctionHouseBot" + std::to_string(AHBplayerAccount);

    WorldSession _session(AHBplayerAccount, std::move(accountName), nullptr, SEC_PLAYER, sWorld->getIntConfig(CONFIG_EXPANSION), 0, LOCALE_enUS, 0, false, false, 0);
    Player _AHBplayer(&_session);
    _AHBplayer.Initialize(AHBplayerGUID);
    ObjectAccessor::AddObject(&_AHBplayer);

    // Add New Bids
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        if (EnableAuctionsAlliance){
            addNewAuctions(&_AHBplayer, &AllianceConfig);
            if (((_newrun - _lastrun_a) >= (AllianceConfig.GetBiddingInterval() * MINUTE)) && (AllianceConfig.GetBidsPerInterval() > 0))
            {
                //if (debug_Out) sLog->outError( "AHBuyer: %u seconds have passed since last bid", (_newrun - _lastrun_a));
                //if (debug_Out) sLog->outError( "AHBuyer: Bidding on Alliance Auctions");
                addNewAuctionBuyerBotBid(&_AHBplayer, &AllianceConfig, &_session);
                _lastrun_a = _newrun;
            }
        }

        if (EnableAuctionsHorde)
        {   addNewAuctions(&_AHBplayer, &HordeConfig);
            if (((_newrun - _lastrun_h) >= (HordeConfig.GetBiddingInterval() * MINUTE)) && (HordeConfig.GetBidsPerInterval() > 0))
            {
                //if (debug_Out) sLog->outError( "AHBuyer: %u seconds have passed since last bid", (_newrun - _lastrun_h));
                //if (debug_Out) sLog->outError( "AHBuyer: Bidding on Horde Auctions");
                addNewAuctionBuyerBotBid(&_AHBplayer, &HordeConfig, &_session);
                _lastrun_h = _newrun;
            }
        }
    }

    if (EnableAuctionsNeutral){
        addNewAuctions(&_AHBplayer, &NeutralConfig);
        if (((_newrun - _lastrun_n) >= (NeutralConfig.GetBiddingInterval() * MINUTE)) && (NeutralConfig.GetBidsPerInterval() > 0))
        {
            //if (debug_Out) sLog->outError( "AHBuyer: %u seconds have passed since last bid", (_newrun - _lastrun_n));
            //if (debug_Out) sLog->outError( "AHBuyer: Bidding on Neutral Auctions");
            addNewAuctionBuyerBotBid(&_AHBplayer, &NeutralConfig, &_session);
            _lastrun_n = _newrun;
        }
    }
    
    ObjectAccessor::RemoveObject(&_AHBplayer);
}

void AuctionHouseBot::Initialize()
{
    DisableItemStore.clear();
    QueryResult result = WorldDatabase.Query("SELECT item FROM mod_auctionhousebot_disabled_items");

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            DisableItemStore.insert(fields[0].Get<uint32>());
        } while (result->NextRow());
    }

    //End Filters
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        LoadValues(&AllianceConfig);
        LoadValues(&HordeConfig);
    }
    LoadValues(&NeutralConfig);

    //
    // check if the AHBot account/GUID in the config actually exists
    //

    if ((AHBplayerAccount != 0) || (AHBplayerGUID != 0))
    {
        QueryResult result = CharacterDatabase.Query("SELECT 1 FROM characters WHERE account = {} AND guid = {}", AHBplayerAccount, AHBplayerGUID);
        if (!result)
        {
           LOG_ERROR("module", "AuctionHouseBot: The account/GUID-information set for your AHBot is incorrect (account: {} guid: {})", AHBplayerAccount, AHBplayerGUID);
           return;
        }
    }

    if (AHBSeller)
    {
        m_nConfigCounts = 0;
        // init g_configs
        if (!LoadConfigFromDB()){
            LOG_ERROR("module", "AuctionHouseBot: Could not load configuration from database");
            return;
        }
        // init auction house config count.
        for (const auto & item : g_configs){
            AllianceConfig.InitItemCounts(item.first, 0);
            HordeConfig.InitItemCounts(item.first, 0);
            NeutralConfig.InitItemCounts(item.first, 0);
        }

        LoadNpcItemFromDB();

        LoadLootItemFromDB();

        ItemTemplateContainer const* its = sObjectMgr->GetItemTemplateStore();
        LOG_INFO("module", "GetItemTemplateStore: {}", its->size());
        for (ItemTemplateContainer::const_iterator itr = its->begin(); itr != its->end(); ++itr)
        {
            
            if (!FilterByBonding(itr->second.Bonding))
                continue;

            if (itr->second.Quality > 6)
                continue;

            // Disable items by Id
            if (DisableItemStore.find(itr->second.ItemId) != DisableItemStore.end())
            {
                LOG_DEBUG("module", "AuctionHouseBot: Item {} disabled (PTR/Beta/Unused Item)", itr->second.ItemId);
                continue;
            }

            if (!FilterByCharacterClass(itr->second.AllowableClass))
                continue;

            // Such items should not be present
            if (itr->second.Class == ITEM_CLASS_PERMANENT)
            {
                LOG_INFO("module", "AuctionHouseBot: Item {} disabled (Permanent Enchant Item)", itr->second.ItemId);
                continue;
            }

            // Disable conjured items
            if ((DisableConjured) && (itr->second.IsConjuredConsumable()))
            {
                continue;
            }

            // Disable moneyloot
            if ((DisableMoneyLoot) && (itr->second.MinMoneyLoot > 0))
            {
                LOG_TRACE("module", "AuctionHouseBot: Item {} disabled (MoneyLoot)", itr->second.ItemId);
                continue;
            }

            // Filter by item class
            uint32 index = GET_INDEX(itr->second.Class, itr->second.SubClass);
            auto it = g_configs.find(index);
            if (it != g_configs.end()){

                if (it->second.Count == 0) continue;
                m_nConfigCounts += it->second.Count;
                
                if (itr->second.Quality < it->second.QualityMin || it->second.QualityMax < itr->second.Quality)
                    continue;

                g_mapCategoryItems[index].push_back(itr->second.ItemId);
                continue;
            }

            if (!FilterByVendorItem(itr->second.ItemId))
                continue;

            if (!FilterByLootItem(itr->second.ItemId))
                continue;
            
             // Disable Items below level X
            if ((DisableItemsBelowLevel) && (itr->second.Class != ITEM_CLASS_TRADE_GOODS) && (itr->second.ItemLevel < DisableItemsBelowLevel))
            {
                if (debug_Out_Filters)
                    LOG_ERROR("module", "AuctionHouseBot: Item {} disabled (Item Level = {})", itr->second.ItemId, itr->second.ItemLevel);
                continue;
            }

            // Disable Items above level X
            if ((DisableItemsAboveLevel) && (itr->second.Class != ITEM_CLASS_TRADE_GOODS) && (itr->second.ItemLevel > DisableItemsAboveLevel))
            {
                if (debug_Out_Filters)
                    LOG_ERROR("module", "AuctionHouseBot: Item {} disabled (Item Level = {})", itr->second.ItemId, itr->second.ItemLevel);
                continue;
            }

           // Disable Trade Goods below level X
            if ((DisableTGsBelowLevel) && (itr->second.Class == ITEM_CLASS_TRADE_GOODS) && (itr->second.ItemLevel < DisableTGsBelowLevel))
            {
                if (debug_Out_Filters)
                    LOG_ERROR("module", "AuctionHouseBot: Trade Good {} disabled (Trade Good Level = {})", itr->second.ItemId, itr->second.ItemLevel);
                continue;
            }

           // Disable Trade Goods above level X
            if ((DisableTGsAboveLevel) && (itr->second.Class == ITEM_CLASS_TRADE_GOODS) && (itr->second.ItemLevel > DisableTGsAboveLevel))
            {
                if (debug_Out_Filters)
                    LOG_ERROR("module", "AuctionHouseBot: Trade Good {} disabled (Trade Good Level = {})", itr->second.ItemId, itr->second.ItemLevel);
                continue;
            }

            // Disable Items below GUID X
            if ((DisableItemsBelowGUID) && (itr->second.Class != ITEM_CLASS_TRADE_GOODS) && (itr->second.ItemId < DisableItemsBelowGUID))
            {
                if (debug_Out_Filters)
                    LOG_ERROR("module", "AuctionHouseBot: Item {} disabled (Item Level = {})", itr->second.ItemId, itr->second.ItemLevel);
                continue;
            }

            // Disable Items above GUID X
            if ((DisableItemsAboveGUID) && (itr->second.Class != ITEM_CLASS_TRADE_GOODS) && (itr->second.ItemId > DisableItemsAboveGUID))
            {
                if (debug_Out_Filters)
                    LOG_ERROR("module", "AuctionHouseBot: Item {} disabled (Item Level = {})", itr->second.ItemId, itr->second.ItemLevel);
                continue;
            }

            // Disable Trade Goods below GUID X
            if ((DisableTGsBelowGUID) && (itr->second.Class == ITEM_CLASS_TRADE_GOODS) && (itr->second.ItemId < DisableTGsBelowGUID))
            {
                if (debug_Out_Filters)
                    LOG_ERROR("module", "AuctionHouseBot: Item {} disabled (Trade Good Level = {})", itr->second.ItemId, itr->second.ItemLevel);
                continue;
            }

            // Disable Trade Goods above GUID X
            if ((DisableTGsAboveGUID) && (itr->second.Class == ITEM_CLASS_TRADE_GOODS) && (itr->second.ItemId > DisableTGsAboveGUID))
            {
                if (debug_Out_Filters)
                    LOG_ERROR("module", "AuctionHouseBot: Item {} disabled (Trade Good Level = {})", itr->second.ItemId, itr->second.ItemLevel);
                continue;
            }

        }

        LOG_INFO("module", "AuctionHouseBot:");
        LOG_INFO("module", "{} disabled items", uint32(DisableItemStore.size()));

        for (const auto& item : g_mapCategoryItems)
        {   
            std::string itemName = g_configs[item.first].Name;
            LOG_DEBUG("module", "loaded {} {} items", (uint32)item.second.size(), itemName);
        }

        LOG_INFO("module", "AuctionHouse total {} items", m_nConfigCounts);
        
    }

    LOG_INFO("module", "AuctionHouseBot and AuctionHouseBuyer have been loaded.");
}

bool AuctionHouseBot::FilterByLootItem(uint32 itemID)
{
    if ((!Loot_Items))
    {
        for (unsigned int i = 0; (i < lootItems.size()); i++)
        {
            if (itemID == lootItems[i])
                return false;
        }
    }

    return true;
}

bool AuctionHouseBot::FilterByVendorItem(uint32 itemId)
{
    if ((!Vendor_Items))
    {
        for (unsigned int i = 0; (i < npcItems.size()); i++)
        {
            if (itemId == npcItems[i])
                return false;
        }
    }
    return true;
}

void AuctionHouseBot::InitializeConfiguration()
{
    debug_Out = sConfigMgr->GetOption<bool>("AuctionHouseBot.DEBUG", false);
    debug_Out_Filters = sConfigMgr->GetOption<bool>("AuctionHouseBot.DEBUG_FILTERS", false);

    AHBSeller = sConfigMgr->GetOption<bool>("AuctionHouseBot.EnableSeller", false);
    AHBBuyer = sConfigMgr->GetOption<bool>("AuctionHouseBot.EnableBuyer", false);
    SellMethod = sConfigMgr->GetOption<bool>("AuctionHouseBot.UseBuyPriceForSeller", false);
    BuyMethod = sConfigMgr->GetOption<bool>("AuctionHouseBot.UseBuyPriceForBuyer", false);

    AHBplayerAccount = sConfigMgr->GetOption<uint32>("AuctionHouseBot.Account", 0);
    AHBplayerGUID = sConfigMgr->GetOption<uint32>("AuctionHouseBot.GUID", 0);
    ItemsPerCycle = sConfigMgr->GetOption<uint32>("AuctionHouseBot.ItemsPerCycle", 200);

    EnableAuctionsAlliance = sConfigMgr->GetOption<bool>("AuctionHouseBot.Alliance", true);
    EnableAuctionsHorde = sConfigMgr->GetOption<bool>("AuctionHouseBot.Horde", true);
    EnableAuctionsNeutral = sConfigMgr->GetOption<bool>("AuctionHouseBot.Neutral", false);
    //Begin Filters

    Vendor_Items = sConfigMgr->GetOption<bool>("AuctionHouseBot.VendorItems", false);
    Loot_Items = sConfigMgr->GetOption<bool>("AuctionHouseBot.LootItems", true);

    No_Bind = sConfigMgr->GetOption<bool>("AuctionHouseBot.No_Bind", true);
    Bind_When_Picked_Up = sConfigMgr->GetOption<bool>("AuctionHouseBot.Bind_When_Picked_Up", false);
    Bind_When_Equipped = sConfigMgr->GetOption<bool>("AuctionHouseBot.Bind_When_Equipped", true);
    Bind_When_Use = sConfigMgr->GetOption<bool>("AuctionHouseBot.Bind_When_Use", true);
    Bind_Quest_Item = sConfigMgr->GetOption<bool>("AuctionHouseBot.Bind_Quest_Item", false);

    DisableConjured = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableConjured", false);
    DisableMoneyLoot = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableMoneyLoot", false);

    DisableWarriorItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableWarriorItems", false);
    DisablePaladinItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisablePaladinItems", false);
    DisableHunterItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableHunterItems", false);
    DisableRogueItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableRogueItems", false);
    DisablePriestItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisablePriestItems", false);
    DisableDKItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableDKItems", false);
    DisableShamanItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableShamanItems", false);
    DisableMageItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableMageItems", false);
    DisableWarlockItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableWarlockItems", false);
    DisableUnusedClassItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableUnusedClassItems", false);
    DisableDruidItems = sConfigMgr->GetOption<bool>("AuctionHouseBot.DisableDruidItems", false);

    DisableItemsBelowLevel = sConfigMgr->GetOption<uint32>("AuctionHouseBot.DisableItemsBelowLevel", 0);
    DisableItemsAboveLevel = sConfigMgr->GetOption<uint32>("AuctionHouseBot.DisableItemsAboveLevel", 0);
    DisableTGsBelowLevel = sConfigMgr->GetOption<uint32>("AuctionHouseBot.DisableTGsBelowLevel", 0);
    DisableTGsAboveLevel = sConfigMgr->GetOption<uint32>("AuctionHouseBot.DisableTGsAboveLevel", 0);
    DisableItemsBelowGUID = sConfigMgr->GetOption<uint32>("AuctionHouseBot.DisableItemsBelowGUID", 0);
    DisableItemsAboveGUID = sConfigMgr->GetOption<uint32>("AuctionHouseBot.DisableItemsAboveGUID", 0);
    DisableTGsBelowGUID = sConfigMgr->GetOption<uint32>("AuctionHouseBot.DisableTGsBelowGUID", 0);
    DisableTGsAboveGUID = sConfigMgr->GetOption<uint32>("AuctionHouseBot.DisableTGsAboveGUID", 0);
}

void AuctionHouseBot::IncrementItemCounts(AuctionEntry* ah)
{
    // from auctionhousehandler.cpp, creates auction pointer & player pointer

    // get exact item information
    Item *pItem =  sAuctionMgr->GetAItem(ah->item_guid);
    if (!pItem)
    {
		if (debug_Out)
            LOG_ERROR("module", "AHBot: Item {} doesn't exist, perhaps bought already?", ah->item_guid.ToString());
        return;
    }

    // get item prototype
    ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(ah->item_template);

    AHBConfig *config;

    AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(ah->GetHouseId());
    if (!ahEntry)
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBot: {} returned as House Faction. Neutral", ah->GetHouseId());
        config = &NeutralConfig;
    }
    else if (ahEntry->houseId == AUCTIONHOUSE_ALLIANCE)
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBot: {} returned as House Faction. Alliance", ah->GetHouseId());
        config = &AllianceConfig;
    }
    else if (ahEntry->houseId == AUCTIONHOUSE_HORDE)
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBot: {} returned as House Faction. Horde", ah->GetHouseId());
        config = &HordeConfig;
    }
    else
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBot: {} returned as House Faction. Neutral", ah->GetHouseId());
        config = &NeutralConfig;
    }

    config->IncItemCounts(prototype->Class, prototype->SubClass, prototype->Quality);
}

void AuctionHouseBot::DecrementItemCounts(AuctionEntry* ah, uint32 itemEntry)
{
    // get item prototype
    ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(itemEntry);

    AHBConfig *config;

    AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(ah->GetHouseId());
    if (!ahEntry)
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBot: {} returned as House Faction. Neutral", ah->GetHouseId());
        config = &NeutralConfig;
    }
    else if (ahEntry->houseId == AUCTIONHOUSE_ALLIANCE)
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBot: {} returned as House Faction. Alliance", ah->GetHouseId());
        config = &AllianceConfig;
    }
    else if (ahEntry->houseId == AUCTIONHOUSE_HORDE)
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBot: {} returned as House Faction. Horde", ah->GetHouseId());
        config = &HordeConfig;
    }
    else
    {
        if (debug_Out)
            LOG_ERROR("module", "AHBot: {} returned as House Faction. Neutral", ah->GetHouseId());
        config = &NeutralConfig;
    }

    config->DecItemCounts(prototype->Class, prototype->SubClass, prototype->Quality);
}

void AuctionHouseBot::Commands(AHBotCommand command, uint32 ahMapID, uint32 col, char* args)
{
    AHBConfig *config = NULL;
    switch (ahMapID)
    {
    case 2:
        config = &AllianceConfig;
        break;
    case 6:
        config = &HordeConfig;
        break;
    case 7:
        config = &NeutralConfig;
        break;
    }
    std::string color;
    switch (col)
    {
    case AHB_GREY:
        color = "grey";
        break;
    case AHB_WHITE:
        color = "white";
        break;
    case AHB_GREEN:
        color = "green";
        break;
    case AHB_BLUE:
        color = "blue";
        break;
    case AHB_PURPLE:
        color = "purple";
        break;
    case AHB_ORANGE:
        color = "orange";
        break;
    case AHB_YELLOW:
        color = "yellow";
        break;
    default:
        break;
    }
    switch (command)
    {
    case AHBotCommand::ahexpire:
        {
            AuctionHouseObject* auctionHouse =  sAuctionMgr->GetAuctionsMap(config->GetAHFID());

            AuctionHouseObject::AuctionEntryMap::iterator itr;
            itr = auctionHouse->GetAuctionsBegin();

            while (itr != auctionHouse->GetAuctionsEnd())
            {
                if (itr->second->owner.GetCounter() == AHBplayerGUID)
                {
                    itr->second->expire_time = GameTime::GetGameTime().count();
                    uint32 id = itr->second->Id;
                    uint32 expire_time = itr->second->expire_time;
                    CharacterDatabase.Execute("UPDATE auctionhouse SET time = '{}' WHERE id = '{}'", expire_time, id);
                }
                ++itr;
            }
        }
        break;
    case AHBotCommand::minprice:
        {
            char * param1 = strtok(args, " ");
            uint32 minPrice = (uint32) strtoul(param1, NULL, 0);
			WorldDatabase.Execute("UPDATE mod_auctionhousebot SET minprice{} = '{}' WHERE auctionhouse = '{}'", color, minPrice, ahMapID);
            config->SetMinPrice(col, minPrice);
        }
        break;
    case AHBotCommand::maxprice:
        {
            char * param1 = strtok(args, " ");
            uint32 maxPrice = (uint32) strtoul(param1, NULL, 0);
			WorldDatabase.Execute("UPDATE mod_auctionhousebot SET maxprice{} = '{}' WHERE auctionhouse = '{}'", color, maxPrice, ahMapID);
            config->SetMaxPrice(col, maxPrice);
        }
        break;
    case AHBotCommand::maxstack:
        {
            char * param1 = strtok(args, " ");
            uint32 maxStack = (uint32) strtoul(param1, NULL, 0);
			WorldDatabase.Execute("UPDATE mod_auctionhousebot SET maxstack{} = '{}' WHERE auctionhouse = '{}'", color, maxStack, ahMapID);
            config->SetMaxStack(col, maxStack);
        }
        break;
    case AHBotCommand::buyerprice:
        {
            char * param1 = strtok(args, " ");
            uint32 buyerPrice = (uint32) strtoul(param1, NULL, 0);
			WorldDatabase.Execute("UPDATE mod_auctionhousebot SET buyerprice{} = '{}' WHERE auctionhouse = '{}'", color, buyerPrice, ahMapID);
            config->SetBuyerPrice(col, buyerPrice);
        }
        break;
    case AHBotCommand::bidinterval:
        {
            char * param1 = strtok(args, " ");
            uint32 bidInterval = (uint32) strtoul(param1, NULL, 0);
			WorldDatabase.Execute("UPDATE mod_auctionhousebot SET buyerbiddinginterval = '{}' WHERE auctionhouse = '{}'", bidInterval, ahMapID);
            config->SetBiddingInterval(bidInterval);
        }
        break;
    case AHBotCommand::bidsperinterval:
        {
            char * param1 = strtok(args, " ");
            uint32 bidsPerInterval = (uint32) strtoul(param1, NULL, 0);
			WorldDatabase.Execute("UPDATE mod_auctionhousebot SET buyerbidsperinterval = '{}' WHERE auctionhouse = '{}'", bidsPerInterval, ahMapID);
            config->SetBidsPerInterval(bidsPerInterval);
        }
        break;
    default:
        break;
    }
}

void AuctionHouseBot::LoadValues(AHBConfig *config)
{
    if (debug_Out)
        LOG_ERROR("module", "Start Settings for {} Auctionhouses:", WorldDatabase.Query("SELECT name FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<std::string_view>());

    if (AHBSeller)
    {
        //load min and max prices
		config->SetMinPrice(AHB_GREY, WorldDatabase.Query("SELECT minpricegrey FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxPrice(AHB_GREY, WorldDatabase.Query("SELECT maxpricegrey FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMinPrice(AHB_WHITE, WorldDatabase.Query("SELECT minpricewhite FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxPrice(AHB_WHITE, WorldDatabase.Query("SELECT maxpricewhite FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMinPrice(AHB_GREEN, WorldDatabase.Query("SELECT minpricegreen FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxPrice(AHB_GREEN, WorldDatabase.Query("SELECT maxpricegreen FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMinPrice(AHB_BLUE, WorldDatabase.Query("SELECT minpriceblue FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxPrice(AHB_BLUE, WorldDatabase.Query("SELECT maxpriceblue FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMinPrice(AHB_PURPLE, WorldDatabase.Query("SELECT minpricepurple FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxPrice(AHB_PURPLE, WorldDatabase.Query("SELECT maxpricepurple FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMinPrice(AHB_ORANGE, WorldDatabase.Query("SELECT minpriceorange FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxPrice(AHB_ORANGE, WorldDatabase.Query("SELECT maxpriceorange FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMinPrice(AHB_YELLOW, WorldDatabase.Query("SELECT minpriceyellow FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxPrice(AHB_YELLOW, WorldDatabase.Query("SELECT maxpriceyellow FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
        //load max stacks
		config->SetMaxStack(AHB_GREY, WorldDatabase.Query("SELECT maxstackgrey FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxStack(AHB_WHITE, WorldDatabase.Query("SELECT maxstackwhite FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxStack(AHB_GREEN, WorldDatabase.Query("SELECT maxstackgreen FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxStack(AHB_BLUE, WorldDatabase.Query("SELECT maxstackblue FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxStack(AHB_PURPLE, WorldDatabase.Query("SELECT maxstackpurple FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxStack(AHB_ORANGE, WorldDatabase.Query("SELECT maxstackorange FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetMaxStack(AHB_YELLOW, WorldDatabase.Query("SELECT maxstackyellow FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
        if (debug_Out)
        {
            LOG_ERROR("module", "minPriceGrey            = {}", config->GetMinPrice(AHB_GREY));
            LOG_ERROR("module", "maxPriceGrey            = {}", config->GetMaxPrice(AHB_GREY));
            LOG_ERROR("module", "minPriceWhite           = {}", config->GetMinPrice(AHB_WHITE));
            LOG_ERROR("module", "maxPriceWhite           = {}", config->GetMaxPrice(AHB_WHITE));
            LOG_ERROR("module", "minPriceGreen           = {}", config->GetMinPrice(AHB_GREEN));
            LOG_ERROR("module", "maxPriceGreen           = {}", config->GetMaxPrice(AHB_GREEN));
            LOG_ERROR("module", "minPriceBlue            = {}", config->GetMinPrice(AHB_BLUE));
            LOG_ERROR("module", "maxPriceBlue            = {}", config->GetMaxPrice(AHB_BLUE));
            LOG_ERROR("module", "minPricePurple          = {}", config->GetMinPrice(AHB_PURPLE));
            LOG_ERROR("module", "maxPricePurple          = {}", config->GetMaxPrice(AHB_PURPLE));
            LOG_ERROR("module", "minPriceOrange          = {}", config->GetMinPrice(AHB_ORANGE));
            LOG_ERROR("module", "maxPriceOrange          = {}", config->GetMaxPrice(AHB_ORANGE));
            LOG_ERROR("module", "minPriceYellow          = {}", config->GetMinPrice(AHB_YELLOW));
            LOG_ERROR("module", "maxPriceYellow          = {}", config->GetMaxPrice(AHB_YELLOW));

            LOG_ERROR("module", "maxStackGrey            = {}", config->GetMaxStack(AHB_GREY));
            LOG_ERROR("module", "maxStackWhite           = {}", config->GetMaxStack(AHB_WHITE));
            LOG_ERROR("module", "maxStackGreen           = {}", config->GetMaxStack(AHB_GREEN));
            LOG_ERROR("module", "maxStackBlue            = {}", config->GetMaxStack(AHB_BLUE));
            LOG_ERROR("module", "maxStackPurple          = {}", config->GetMaxStack(AHB_PURPLE));
            LOG_ERROR("module", "maxStackOrange          = {}", config->GetMaxStack(AHB_ORANGE));
            LOG_ERROR("module", "maxStackYellow          = {}", config->GetMaxStack(AHB_YELLOW));
        }

        //AuctionHouseEntry const* ahEntry =  sAuctionMgr->GetAuctionHouseEntry(config->GetAHFID());
        AuctionHouseObject* auctionHouse =  sAuctionMgr->GetAuctionsMap(config->GetAHFID());

        config->ResetItemCounts();
        uint32 auctions = auctionHouse->Getcount();

        if (auctions)
        {
            for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
            {
                AuctionEntry *Aentry = itr->second;
				Item *item = sAuctionMgr->GetAItem(Aentry->item_guid);
                if (item)
                {
                    ItemTemplate const *prototype = item->GetTemplate();
                    if (prototype)
                    {
                        if (config->AddItemCountsMap(prototype->Class, prototype->SubClass)) 
                            continue;
                    }
                }
            }
        }

        if (debug_Out)
        {
			LOG_ERROR("module", "Current Settings for {} Auctionhouses:", WorldDatabase.Query("SELECT name FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<std::string>());
        }
    }
    if (AHBBuyer)
    {
        //load buyer bid prices
		config->SetBuyerPrice(AHB_GREY, WorldDatabase.Query("SELECT buyerpricegrey FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetBuyerPrice(AHB_WHITE, WorldDatabase.Query("SELECT buyerpricewhite FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetBuyerPrice(AHB_GREEN, WorldDatabase.Query("SELECT buyerpricegreen FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetBuyerPrice(AHB_BLUE, WorldDatabase.Query("SELECT buyerpriceblue FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetBuyerPrice(AHB_PURPLE, WorldDatabase.Query("SELECT buyerpricepurple FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetBuyerPrice(AHB_ORANGE, WorldDatabase.Query("SELECT buyerpriceorange FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
		config->SetBuyerPrice(AHB_YELLOW, WorldDatabase.Query("SELECT buyerpriceyellow FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
        //load bidding interval
		config->SetBiddingInterval(WorldDatabase.Query("SELECT buyerbiddinginterval FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
        //load bids per interval
		config->SetBidsPerInterval(WorldDatabase.Query("SELECT buyerbidsperinterval FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<uint32>());
        if (debug_Out)
        {
            LOG_INFO("module", "buyerPriceGrey          = {}", config->GetBuyerPrice(AHB_GREY));
            LOG_INFO("module", "buyerPriceWhite         = {}", config->GetBuyerPrice(AHB_WHITE));
            LOG_INFO("module", "buyerPriceGreen         = {}", config->GetBuyerPrice(AHB_GREEN));
            LOG_INFO("module", "buyerPriceBlue          = {}", config->GetBuyerPrice(AHB_BLUE));
            LOG_INFO("module", "buyerPricePurple        = {}", config->GetBuyerPrice(AHB_PURPLE));
            LOG_INFO("module", "buyerPriceOrange        = {}", config->GetBuyerPrice(AHB_ORANGE));
            LOG_INFO("module", "buyerPriceYellow        = {}", config->GetBuyerPrice(AHB_YELLOW));
            LOG_INFO("module", "buyerBiddingInterval    = {}", config->GetBiddingInterval());
            LOG_INFO("module", "buyerBidsPerInterval    = {}", config->GetBidsPerInterval());
        }
    }

    if (debug_Out)
        LOG_ERROR("module", "End Settings for {} Auctionhouses:", WorldDatabase.Query("SELECT name FROM mod_auctionhousebot WHERE auctionhouse = {}", config->GetAHID())->Fetch()->Get<std::string>());
}

bool AuctionHouseBot::LoadConfigFromDB(){

    g_configs.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT Class, SubClass, `Name`, `Count`, QualityMin, QualityMax FROM `mod_ah_config_category`");

    if (!result)
    {
        LOG_WARN("module", ">> Loaded 0 Seller Config. DB table `mod_ah_config_category` is empty.");
        return false;
    }

    do
    {
        Field* fields = result->Fetch();
        uint8 Class = fields[0].Get<uint8>();
        uint8 SubClass = fields[1].Get<uint8>();

        auto & config = g_configs[GET_INDEX(Class, SubClass)];

        config.Name = fields[2].Get<std::string>();
        config.Count = fields[3].Get<uint32>();
        config.QualityMin = fields[4].Get<uint8>();
        config.QualityMax = fields[5].Get<uint8>();

    } while (result->NextRow());

    return true;
}

bool AuctionHouseBot::LoadNpcItemFromDB(){

    char npcQuery[] = "SELECT distinct item FROM npc_vendor";
    QueryResult results = WorldDatabase.Query(npcQuery);
    if (!results)
    {
        LOG_ERROR("module", "AuctionHouseBot: \"{}\" failed", npcQuery);
        return false;
    }

    do
    {
        Field* fields = results->Fetch();
        npcItems.push_back(fields[0].Get<int32>());

    } while (results->NextRow());

    return true;
}

bool AuctionHouseBot::LoadLootItemFromDB(){

    char lootQuery[] = "SELECT item FROM creature_loot_template UNION "
        "SELECT item FROM reference_loot_template UNION "
        "SELECT item FROM disenchant_loot_template UNION "
        "SELECT item FROM fishing_loot_template UNION "
        "SELECT item FROM gameobject_loot_template UNION "
        "SELECT item FROM item_loot_template UNION "
        "SELECT item FROM milling_loot_template UNION "
        "SELECT item FROM pickpocketing_loot_template UNION "
        "SELECT item FROM prospecting_loot_template UNION "
        "SELECT item FROM skinning_loot_template";

    QueryResult results = WorldDatabase.Query(lootQuery);
    if (!results)
    {
        LOG_ERROR("module", "AuctionHouseBot: \"{}\" failed", lootQuery);
        return false;
    }

    do
    {
        Field* fields = results->Fetch();
        lootItems.push_back(fields[0].Get<uint32>());

    } while (results->NextRow());
    
    return true;
}

bool AuctionHouseBot::FilterByBonding(uint32 bonding){

    if (bonding == NO_BIND) return No_Bind;
    if (bonding == BIND_WHEN_PICKED_UP) return Bind_When_Picked_Up;
    if (bonding == BIND_WHEN_EQUIPED) return Bind_When_Equipped;
    if (bonding == BIND_WHEN_USE) return Bind_When_Use;
    if (bonding == BIND_QUEST_ITEM) return Bind_Quest_Item;

    return false;
}

bool AuctionHouseBot::FilterByCharacterClass(uint32 Class){
    // Disable items specifically for Warrior
    if (Class == 1){
        return !DisableWarriorItems;
    }
    // Disable items specifically for Paladin
    if (Class == 2){
        return !DisablePaladinItems;
    }
    // Disable items specifically for Hunter
    if (Class == 4){
        return !DisableHunterItems;
    }
    // Disable items specifically for Rogue
    if (Class == 8){
        return !DisableRogueItems;
    }
    // Disable items specifically for Priest
    if (Class == 16){
        return !DisablePriestItems;
    }
    // Disable items specifically for DK
    if (Class == 32){
        return !DisableDKItems;
    }
    // Disable items specifically for Shaman
    if (Class == 64){
        return !DisableShamanItems;
    }
    // Disable items specifically for Mage
    if (Class == 128){
        return !DisableMageItems;
    }
    // Disable items specifically for Warlock
    if (Class == 256){
        return !DisableWarlockItems;
    }
    // Disable items specifically for Unused Class
    if (Class == 512){
        return !DisableUnusedClassItems;
    }
    // Disable items specifically for Druid
    if (Class == 1024){
        return !DisableDruidItems;
    }

    return true;
}
