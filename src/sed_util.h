#ifndef SED_UTIL_H
#define SED_UTIL_H

u32 DigitCount(u32 n)
{
    u32 count = 0;
    
    while(n)
    {
        n = n / 10;
        count++;
    }
    
    return count;
}

u32 Power(u32 x, u32 power)
{
    u32 result = 1;
    
    for(u32 n = 0; n < power; n++)
    {
        result *= x;
    }
    
    return result;
}

u32 StringToInteger(const u8 *str, u32 size)
{
    u32 num = 0;
    u32 power = size - 1;
    
    for(u32 n = 0; n < size; n++)
    {
        num += (str[n] - '0') * Power(10, power);
        power--;
    }
    
    return num;
}

#endif //SED_UTIL_H
