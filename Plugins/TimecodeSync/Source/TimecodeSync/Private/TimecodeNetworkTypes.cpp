#include "TimecodeNetworkTypes.h"

TArray<uint8> FTimecodeNetworkMessage::Serialize() const
{
    TArray<uint8> Result;

    // Serialize message type
    uint8 TypeValue = static_cast<uint8>(MessageType);
    Result.Add(TypeValue);

    // Serialize timecode
    FTCHARToUTF8 TimecodeUtf8(*Timecode);
    uint16 TimecodeLength = static_cast<uint16>(TimecodeUtf8.Length());
    Result.Add(TimecodeLength & 0xFF);
    Result.Add((TimecodeLength >> 8) & 0xFF);
    Result.Append((uint8*)TimecodeUtf8.Get(), TimecodeUtf8.Length());

    // Serialize data
    FTCHARToUTF8 DataUtf8(*Data);
    uint16 DataLength = static_cast<uint16>(DataUtf8.Length());
    Result.Add(DataLength & 0xFF);
    Result.Add((DataLength >> 8) & 0xFF);
    Result.Append((uint8*)DataUtf8.Get(), DataUtf8.Length());

    // Serialize timestamp
    uint8* TimestampBytes = (uint8*)&Timestamp;
    for (int32 i = 0; i < sizeof(double); ++i)
    {
        Result.Add(TimestampBytes[i]);
    }

    // Serialize sender ID
    FTCHARToUTF8 SenderIDUtf8(*SenderID);
    uint16 SenderIDLength = static_cast<uint16>(SenderIDUtf8.Length());
    Result.Add(SenderIDLength & 0xFF);
    Result.Add((SenderIDLength >> 8) & 0xFF);
    Result.Append((uint8*)SenderIDUtf8.Get(), SenderIDUtf8.Length());

    return Result;
}

bool FTimecodeNetworkMessage::Deserialize(const TArray<uint8>& InData)
{
    if (InData.Num() < 1 + 2 + 2 + sizeof(double) + 2)
    {
        return false; // Data too short
    }

    int32 Offset = 0;

    // Deserialize message type
    MessageType = static_cast<ETimecodeMessageType>(InData[Offset++]);

    // Deserialize timecode
    uint16 TimecodeLength = InData[Offset] | (InData[Offset + 1] << 8);
    Offset += 2;
    if (Offset + TimecodeLength > InData.Num())
    {
        return false; // Insufficient data
    }
    Timecode = FString(UTF8_TO_TCHAR(&InData[Offset]));
    Offset += TimecodeLength;

    // Deserialize data
    uint16 DataLength = InData[Offset] | (InData[Offset + 1] << 8);
    Offset += 2;
    if (Offset + DataLength > InData.Num())
    {
        return false; // Insufficient data
    }
    this->Data = FString(UTF8_TO_TCHAR(&InData[Offset]));
    Offset += DataLength;

    // Deserialize timestamp
    if (Offset + sizeof(double) > InData.Num())
    {
        return false; // Insufficient data
    }
    FMemory::Memcpy(&Timestamp, &InData[Offset], sizeof(double));
    Offset += sizeof(double);

    // Deserialize sender ID
    uint16 SenderIDLength = InData[Offset] | (InData[Offset + 1] << 8);
    Offset += 2;
    if (Offset + SenderIDLength > InData.Num())
    {
        return false; // Insufficient data
    }
    SenderID = FString(UTF8_TO_TCHAR(&InData[Offset]));

    return true;
}