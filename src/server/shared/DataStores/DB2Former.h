#ifndef _DB2FORMER_H
#define _DB2FORMER_H

enum DB2Former
{
    FT2_STRING = 's',                                        // LocalizedString*
    FT2_STRING_NOT_LOCALIZED = 'S',                          // char*
    FT2_FLOAT = 'f',                                         // float
    FT2_INT = 'i',                                           // uint32
    FT2_BYTE = 'b',                                          // uint8
    FT2_SHORT = 'h',                                         // uint16
    FT2_LONG = 'l'                                           // uint64
};

#endif