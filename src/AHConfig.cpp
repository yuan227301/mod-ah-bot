#include "AHConfig.h"

AHBConfig::AHBConfig(uint32 ahid)
{
    AHID = ahid;
    switch(ahid)
    {
    case 2:
        AHFID = 55;
        break;
    case 6:
        AHFID = 29;
        break;
    case 7:
        AHFID = 120;
        break;
    default:
        AHFID = 120;
        break;
    }

}

void AHBConfig::SetMinPrice(uint32 color, uint32 value)
{
    switch(color)
    {
    case AHB_GREY:
        minPriceGrey = value;
        break;
    case AHB_WHITE:
        minPriceWhite = value;
        break;
    case AHB_GREEN:
        minPriceGreen = value;
        break;
    case AHB_BLUE:
        minPriceBlue = value;
        break;
    case AHB_PURPLE:
        minPricePurple = value;
        break;
    case AHB_ORANGE:
        minPriceOrange = value;
        break;
    case AHB_YELLOW:
        minPriceYellow = value;
        break;
    default:
        break;
    }
}
uint32 AHBConfig::GetMinPrice(uint32 color)
{
    switch(color)
    {
    case AHB_GREY:
        {
            if (minPriceGrey == 0)
                return 100;
            else if (minPriceGrey > maxPriceGrey)
                return maxPriceGrey;
            else
                return minPriceGrey;
            break;
        }
    case AHB_WHITE:
        {
            if (minPriceWhite == 0)
                return 150;
            else if (minPriceWhite > maxPriceWhite)
                return maxPriceWhite;
            else
                return minPriceWhite;
            break;
        }
    case AHB_GREEN:
        {
            if (minPriceGreen == 0)
                return 200;
            else if (minPriceGreen > maxPriceGreen)
                return maxPriceGreen;
            else
                return minPriceGreen;
            break;
        }
    case AHB_BLUE:
        {
            if (minPriceBlue == 0)
                return 250;
            else if (minPriceBlue > maxPriceBlue)
                return maxPriceBlue;
            else
                return minPriceBlue;
            break;
        }
    case AHB_PURPLE:
        {
            if (minPricePurple == 0)
                return 300;
            else if (minPricePurple > maxPricePurple)
                return maxPricePurple;
            else
                return minPricePurple;
            break;
        }
    case AHB_ORANGE:
        {
            if (minPriceOrange == 0)
                return 400;
            else if (minPriceOrange > maxPriceOrange)
                return maxPriceOrange;
            else
                return minPriceOrange;
            break;
        }
    case AHB_YELLOW:
        {
            if (minPriceYellow == 0)
                return 500;
            else if (minPriceYellow > maxPriceYellow)
                return maxPriceYellow;
            else
                return minPriceYellow;
            break;
        }
    default:
        {
            return 0;
            break;
        }
    }
}
void AHBConfig::SetMaxPrice(uint32 color, uint32 value)
{
    switch(color)
    {
    case AHB_GREY:
        maxPriceGrey = value;
        break;
    case AHB_WHITE:
        maxPriceWhite = value;
        break;
    case AHB_GREEN:
        maxPriceGreen = value;
        break;
    case AHB_BLUE:
        maxPriceBlue = value;
        break;
    case AHB_PURPLE:
        maxPricePurple = value;
        break;
    case AHB_ORANGE:
        maxPriceOrange = value;
        break;
    case AHB_YELLOW:
        maxPriceYellow = value;
        break;
    default:
        break;
    }
}
uint32 AHBConfig::GetMaxPrice(uint32 color)
{
    switch(color)
    {
    case AHB_GREY:
        {
            if (maxPriceGrey == 0)
                return 150;
            else
                return maxPriceGrey;
            break;
        }
    case AHB_WHITE:
        {
            if (maxPriceWhite == 0)
                return 250;
            else
                return maxPriceWhite;
            break;
        }
    case AHB_GREEN:
        {
            if (maxPriceGreen == 0)
                return 300;
            else
                return maxPriceGreen;
            break;
        }
    case AHB_BLUE:
        {
            if (maxPriceBlue == 0)
                return 350;
            else
                return maxPriceBlue;
            break;
        }
    case AHB_PURPLE:
        {
            if (maxPricePurple == 0)
                return 450;
            else
                return maxPricePurple;
            break;
        }
    case AHB_ORANGE:
        {
            if (maxPriceOrange == 0)
                return 550;
            else
                return maxPriceOrange;
            break;
        }
    case AHB_YELLOW:
        {
            if (maxPriceYellow == 0)
                return 650;
            else
                return maxPriceYellow;
            break;
        }
    default:
        {
            return 0;
            break;
        }
    }
}

void AHBConfig::SetMaxStack(uint32 color, uint32 value)
{
    switch(color)
    {
    case AHB_GREY:
        maxStackGrey = value;
        break;
    case AHB_WHITE:
        maxStackWhite = value;
        break;
    case AHB_GREEN:
        maxStackGreen = value;
        break;
    case AHB_BLUE:
        maxStackBlue = value;
        break;
    case AHB_PURPLE:
        maxStackPurple = value;
        break;
    case AHB_ORANGE:
        maxStackOrange = value;
        break;
    case AHB_YELLOW:
        maxStackYellow = value;
        break;
    default:
        break;
    }
}
uint32 AHBConfig::GetMaxStack(uint32 color)
{
    switch(color)
    {
    case AHB_GREY:
        {
            return maxStackGrey;
            break;
        }
    case AHB_WHITE:
        {
            return maxStackWhite;
            break;
        }
    case AHB_GREEN:
        {
            return maxStackGreen;
            break;
        }
    case AHB_BLUE:
        {
            return maxStackBlue;
            break;
        }
    case AHB_PURPLE:
        {
            return maxStackPurple;
            break;
        }
    case AHB_ORANGE:
        {
            return maxStackOrange;
            break;
        }
    case AHB_YELLOW:
        {
            return maxStackYellow;
            break;
        }
    default:
        {
            return 0;
            break;
        }
    }
}
void AHBConfig::SetBuyerPrice(uint32 color, uint32 value)
{
    switch(color)
    {
    case AHB_GREY:
        buyerPriceGrey = value;
        break;
    case AHB_WHITE:
        buyerPriceWhite = value;
        break;
    case AHB_GREEN:
        buyerPriceGreen = value;
        break;
    case AHB_BLUE:
        buyerPriceBlue = value;
        break;
    case AHB_PURPLE:
        buyerPricePurple = value;
        break;
    case AHB_ORANGE:
        buyerPriceOrange = value;
        break;
    case AHB_YELLOW:
        buyerPriceYellow = value;
        break;
    default:
        break;
    }
}
uint32 AHBConfig::GetBuyerPrice(uint32 color)
{
    switch(color)
    {
    case AHB_GREY:
        return buyerPriceGrey;
        break;
    case AHB_WHITE:
        return buyerPriceWhite;
        break;
    case AHB_GREEN:
        return buyerPriceGreen;
        break;
    case AHB_BLUE:
        return buyerPriceBlue;
        break;
    case AHB_PURPLE:
        return buyerPricePurple;
        break;
    case AHB_ORANGE:
        return buyerPriceOrange;
        break;
    case AHB_YELLOW:
        return buyerPriceYellow;
        break;
    default:
        return 0;
        break;
    }
}