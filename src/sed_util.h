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

u32 StringToInteger(u8 *str, u32 size)
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

b32 StringToBoolean(u8 *str)
{
    if(!strcmp(str, "true\0"))
    {
        return true;
    }
    else if(!strcmp(str, "false\0"))
    {
        return false;
    }
    
    return false;
}
#endif //SED_UTIL_H
