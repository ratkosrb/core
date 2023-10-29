/*
 * Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "DB2Meta.h"
#include "Log.h"
#include "Errors.h"

DB2MetaField::DB2MetaField(DB2Former type, uint8 arraySize, bool isSigned) : Type(type), ArraySize(arraySize), IsSigned(isSigned)
{
}

DB2Meta::DB2Meta(int32 indexField, uint32 fieldCount, uint32 layoutHash, DB2MetaField const* fields, int32 parentIndexField)
    : IndexField(indexField), ParentIndexField(parentIndexField), FieldCount(fieldCount), LayoutHash(layoutHash), Fields(fields)
{
}

bool DB2Meta::HasIndexFieldInData() const
{
    return IndexField != -1;
}

uint32 DB2Meta::GetIndexField() const
{
    return IndexField == -1 ? 0 : uint32(IndexField);
}

uint32 DB2Meta::GetRecordSize() const
{
    uint32 size = 0;
    for (uint32 i = 0; i < FieldCount; ++i)
    {
        for (uint8 j = 0; j < Fields[i].ArraySize; ++j)
        {
            if (i >= FieldCount && int32(i) == ParentIndexField)
            {
                size += 4;
                continue;
            }

            switch (Fields[i].Type)
            {
                case FT2_BYTE:
                    size += 1;
                    break;
                case FT2_SHORT:
                    size += 2;
                    break;
                case FT2_FLOAT:
                case FT2_INT:
                    size += 4;
                    break;
                case FT2_LONG:
                    size += 8;
                    break;
                case FT2_STRING:
                case FT2_STRING_NOT_LOCALIZED:
                    size += sizeof(char*);
                    break;
                default:
                    ASSERT_FORMAT(false, "Unsupported column type specified %c", Fields[i].Type);
                    break;
            }
        }
    }

    if (!HasIndexFieldInData())
        size += 4;

    return size;
}

int32 DB2Meta::GetParentIndexFieldOffset() const
{
    if (ParentIndexField == -1)
        return -1;

    uint32 offset = 0;
    if (!HasIndexFieldInData())
        offset += 4;

    for (int32 i = 0; i < ParentIndexField; ++i)
    {
        for (uint8 j = 0; j < Fields[i].ArraySize; ++j)
        {
            switch (Fields[i].Type)
            {
                case FT2_BYTE:
                    offset += 1;
                    break;
                case FT2_SHORT:
                    offset += 2;
                    break;
                case FT2_FLOAT:
                case FT2_INT:
                    offset += 4;
                    break;
                case FT2_LONG:
                    offset += 8;
                    break;
                case FT2_STRING:
                case FT2_STRING_NOT_LOCALIZED:
                    offset += sizeof(char*);
                    break;
                default:
                    ASSERT_FORMAT(false, "Unsupported column type specified %c", Fields[i].Type);
                    break;
            }
        }
    }

    return offset;
}

uint32 DB2Meta::GetDbIndexField() const
{
    if (IndexField == -1)
        return 0;

    uint32 index = 0;
    for (uint32 i = 0; i < FieldCount && i < uint32(IndexField); ++i)
        index += Fields[i].ArraySize;

    return index;
}

uint32 DB2Meta::GetDbFieldCount() const
{
    uint32 fields = 0;
    for (uint32 i = 0; i < FieldCount; ++i)
        fields += Fields[i].ArraySize;

    if (!HasIndexFieldInData())
        ++fields;

    return fields;
}

bool DB2Meta::IsSignedField(uint32 field) const
{
    switch (Fields[field].Type)
    {
        case FT2_STRING:
        case FT2_STRING_NOT_LOCALIZED:
        case FT2_FLOAT:
            return false;
        case FT2_INT:
        case FT2_BYTE:
        case FT2_SHORT:
        case FT2_LONG:
        default:
            break;
    }
    if (field == uint32(IndexField) || field == uint32(ParentIndexField))
        return false;

    return Fields[field].IsSigned;
}
