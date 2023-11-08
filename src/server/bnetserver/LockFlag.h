#ifndef _LOCKFLAG_H
#define _LOCKFLAG_H

enum LockFlag
{
    NONE            = 0x00,
    IP_LOCK         = 0x01,
    FIXED_PIN       = 0x02,
    TOTP            = 0x04,
    ALWAYS_ENFORCE  = 0x08,
    GEO_COUNTRY     = 0x10,
    GEO_CITY        = 0x20
};

#endif