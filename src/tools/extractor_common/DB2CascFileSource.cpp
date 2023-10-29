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

#include "DB2CascFileSource.h"
#include <CascLib.h>

DB2CascFileSource::DB2CascFileSource(const std::shared_ptr<CASC::Storage const>& storage, std::string fileName)
{
    _storageHandle = storage;
    _fileHandle.reset(storage->OpenFile(fileName.c_str(), CASC_LOCALE_NONE));
    _fileName = fileName;
}

bool DB2CascFileSource::IsOpen() const
{
    return _fileHandle != nullptr;
}

bool DB2CascFileSource::Read(void* buffer, std::size_t numBytes)
{
    uint32 bytesRead = 0;
    return _fileHandle->ReadFile(buffer, numBytes, &bytesRead) && numBytes == bytesRead;
}

std::size_t DB2CascFileSource::GetPosition() const
{
    return _fileHandle->GetPointer();
}

bool DB2CascFileSource::SetPosition(std::size_t position)
{
    return _fileHandle->SetPointer(position);
}

std::size_t DB2CascFileSource::GetFileSize() const
{
    return _fileHandle->GetSize();
}

char const* DB2CascFileSource::GetFileName() const
{
    return _fileName.c_str();
}
