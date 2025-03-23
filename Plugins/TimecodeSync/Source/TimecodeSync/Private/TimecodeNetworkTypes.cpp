#include "TimecodeNetworkTypes.h"

TArray<uint8> FTimecodeNetworkMessage::Serialize() const
{
    TArray<uint8> Result;

    // Reserve adequate space for the data
    Result.Reserve(256);

    // Serialize message type
    uint8 TypeValue = static_cast<uint8>(MessageType);
    Result.Add(TypeValue);

    // Serialize timecode
    FTCHARToUTF8 TimecodeUtf8(*Timecode);
    uint16 TimecodeLength = static_cast<uint16>(TimecodeUtf8.Length());
    Result.Add(TimecodeLength & 0xFF);
    Result.Add((TimecodeLength >> 8) & 0xFF);
    Result.Append((uint8*)TimecodeUtf8.Get(), TimecodeUtf8.Length());
    // Add null terminator
    Result.Add(0);

    // Serialize data
    FTCHARToUTF8 DataUtf8(*Data);
    uint16 DataLength = static_cast<uint16>(DataUtf8.Length());
    Result.Add(DataLength & 0xFF);
    Result.Add((DataLength >> 8) & 0xFF);
    Result.Append((uint8*)DataUtf8.Get(), DataUtf8.Length());
    // Add null terminator
    Result.Add(0);

    // Serialize timestamp (network byte order)
    uint64 NetworkTimestamp;
    FMemory::Memcpy(&NetworkTimestamp, &Timestamp, sizeof(double));
    for (int32 i = 0; i < sizeof(double); ++i)
    {
        Result.Add((NetworkTimestamp >> ((sizeof(double) - 1 - i) * 8)) & 0xFF);
    }

    // Serialize sender ID
    FTCHARToUTF8 SenderIDUtf8(*SenderID);
    uint16 SenderIDLength = static_cast<uint16>(SenderIDUtf8.Length());
    Result.Add(SenderIDLength & 0xFF);
    Result.Add((SenderIDLength >> 8) & 0xFF);
    Result.Append((uint8*)SenderIDUtf8.Get(), SenderIDUtf8.Length());
    // Add null terminator
    Result.Add(0);

    return Result;
}

bool FTimecodeNetworkMessage::Deserialize(const TArray<uint8>& InData)
{
    // Minimum size check (type + 3 strings with lengths + timestamp)
    if (InData.Num() < 1 + 2 + 1 + 2 + 1 + sizeof(double) + 2 + 1)
    {
        return false; // Data too short
    }

    int32 Offset = 0;

    // Deserialize message type
    MessageType = static_cast<ETimecodeMessageType>(InData[Offset++]);

    // Deserialize timecode
    uint16 TimecodeLength = InData[Offset] | (InData[Offset + 1] << 8);
    Offset += 2;
    if (Offset + TimecodeLength + 1 > InData.Num()) // +1 for null terminator
    {
        return false; // Insufficient data
    }

    // Create a temporary buffer with explicit null termination
    TArray<uint8> TimecodeBuffer;
    TimecodeBuffer.SetNum(TimecodeLength + 1);
    FMemory::Memcpy(TimecodeBuffer.GetData(), &InData[Offset], TimecodeLength);
    TimecodeBuffer[TimecodeLength] = 0; // Ensure null termination
    Timecode = FString(UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(TimecodeBuffer.GetData())));

    Offset += TimecodeLength + 1; // Skip over the null terminator

    // Deserialize data
    uint16 DataLength = InData[Offset] | (InData[Offset + 1] << 8);
    Offset += 2;
    if (Offset + DataLength + 1 > InData.Num()) // +1 for null terminator
    {
        return false; // Insufficient data
    }

    // Create a temporary buffer with explicit null termination
    TArray<uint8> DataBuffer;
    DataBuffer.SetNum(DataLength + 1);
    FMemory::Memcpy(DataBuffer.GetData(), &InData[Offset], DataLength);
    DataBuffer[DataLength] = 0; // Ensure null termination
    this->Data = FString(UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(DataBuffer.GetData())));

    Offset += DataLength + 1; // Skip over the null terminator

    // Deserialize timestamp
    if (Offset + sizeof(double) > InData.Num())
    {
        return false; // Insufficient data
    }

    // Network byte order to host byte order
    uint64 NetworkTimestamp = 0;
    for (int32 i = 0; i < sizeof(double); ++i)
    {
        NetworkTimestamp = (NetworkTimestamp << 8) | InData[Offset + i];
    }
    FMemory::Memcpy(&Timestamp, &NetworkTimestamp, sizeof(double));
    Offset += sizeof(double);

    // Deserialize sender ID
    uint16 SenderIDLength = InData[Offset] | (InData[Offset + 1] << 8);
    Offset += 2;
    if (Offset + SenderIDLength + 1 > InData.Num()) // +1 for null terminator
    {
        return false; // Insufficient data
    }

    // Create a temporary buffer with explicit null termination
    TArray<uint8> SenderIDBuffer;
    SenderIDBuffer.SetNum(SenderIDLength + 1);
    FMemory::Memcpy(SenderIDBuffer.GetData(), &InData[Offset], SenderIDLength);
    SenderIDBuffer[SenderIDLength] = 0; // Ensure null termination
    SenderID = FString(UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(SenderIDBuffer.GetData())));

    return true;
}