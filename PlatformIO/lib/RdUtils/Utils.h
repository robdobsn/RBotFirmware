// Utils
// Rob Dobson 2012-2017

#pragma once

#include <limits.h>

class Utils
{
  public:
    static bool isTimeout(unsigned long curTime, unsigned long lastTime, unsigned long maxDuration)
    {
        if (curTime >= lastTime)
        {
            return (curTime > lastTime + maxDuration);
        }
        return (ULONG_MAX - (lastTime - curTime) > maxDuration);
    }

    static int timeToTimeout(unsigned long curTime, unsigned long lastTime, unsigned long maxDuration)
    {
        if (curTime >= lastTime)
        {
            if (curTime > lastTime + maxDuration)
            {
                return 0;
            }
            return maxDuration - (curTime - lastTime);
        }
        if (ULONG_MAX - (lastTime - curTime) > maxDuration)
        {
            return 0;
        }
        return maxDuration - (ULONG_MAX - (lastTime - curTime));
    }

    static void setJsonBoolResult(String& resp, bool rslt, const char* otherJson = NULL)
    {
        String additionalJson = "";
        if (otherJson)
            additionalJson = otherJson + String(",");
        String retStr;
        if (rslt)
            resp = "{" + additionalJson + "\"rslt\": \"ok\"}";
        else
            resp = "{" + additionalJson + "\"rslt\": \"fail\"}";
    }

    // Following code from Unix sources
    static const unsigned long INADDR_NONE = ((unsigned long)0xffffffff);
    static unsigned long convIPStrToAddr(String &inStr)
    {
        char buf[30];
        char *cp = buf;

        inStr.toCharArray(cp, 29);
        unsigned long val, base, n;
        char c;
        unsigned long parts[4], *pp = parts;

        for (;;)
        {
            /*
             * Collect number up to ``.''.
             * Values are specified as for C:
             * 0x=hex, 0=octal, other=decimal.
             */
            val = 0;
            base = 10;
            if (*cp == '0')
            {
                if ((*++cp == 'x') || (*cp == 'X'))
                {
                    base = 16, cp++;
                }
                else
                {
                    base = 8;
                }
            }
            while ((c = *cp) != '\0')
            {
                if (isascii(c) && isdigit(c))
                {
                    val = (val * base) + (c - '0');
                    cp++;
                    continue;
                }
                if ((base == 16) && isascii(c) && isxdigit(c))
                {
                    val = (val << 4) +
                          (c + 10 - (islower(c) ? 'a' : 'A'));
                    cp++;
                    continue;
                }
                break;
            }
            if (*cp == '.')
            {
                /*
                 * Internet format:
                 *  a.b.c.d
                 *  a.b.c (with c treated as 16-bits)
                 *  a.b (with b treated as 24 bits)
                 */
                if ((pp >= parts + 3) || (val > 0xff))
                {
                    return (INADDR_NONE);
                }
                *pp++ = val, cp++;
            }
            else
            {
                break;
            }
        }

        /*
         * Check for trailing characters.
         */
        if (*cp && (!isascii(*cp) || !isspace(*cp)))
        {
            return (INADDR_NONE);
        }

        /*
         * Concoct the address according to
         * the number of parts specified.
         */
        n = pp - parts + 1;
        switch (n)
        {
        case 1: /* a -- 32 bits */
            break;

        case 2: /* a.b -- 8.24 bits */
            if (val > 0xffffff)
            {
                return (INADDR_NONE);
            }
            val |= parts[0] << 24;
            break;

        case 3: /* a.b.c -- 8.8.16 bits */
            if (val > 0xffff)
            {
                return (INADDR_NONE);
            }
            val |= (parts[0] << 24) | (parts[1] << 16);
            break;

        case 4: /* a.b.c.d -- 8.8.8.8 bits */
            if (val > 0xff)
            {
                return (INADDR_NONE);
            }
            val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
            break;
        }
        return (val);
    }
};
