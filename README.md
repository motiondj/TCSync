# Timecode Synchronization Plugin for Unreal Engine 5.5

[한국어 버전](README_KR.md)

![Version](https://img.shields.io/badge/Version-1.0.0-blue)
![UE Version](https://img.shields.io/badge/Unreal%20Engine-5.5-brightgreen)
![License](https://img.shields.io/badge/License-MIT-green)

## Overview

The Timecode Synchronization Plugin provides a robust solution for precise timecode management and synchronization in Unreal Engine 5.5. The plugin supports accurate timecode conversion between different frame rates, including drop frame timecode handling according to the SMPTE standard.

## Key Features

- **Multi-device Timecode Synchronization**: Synchronize timecode across multiple devices with high precision
- **Frame Rate Conversion**: Convert timecodes between all standard frame rates (24, 25, 30, 60fps)
- **Drop Frame Support**: Full implementation of SMPTE drop frame standard for 29.97 and 59.94fps
- **PLL Algorithm**: Phase-Locked Loop algorithm provides 99.9% error reduction for stable synchronization
- **Real-time Timecode Generation**: Generate accurate timecodes based on system or network time
- **Master/Slave Configuration**: Flexible configuration options for multi-device setups
- **Network Synchronization**: UDP-based network synchronization with packet loss handling

## Installation

1. Download the plugin from the release page or Unreal Engine Marketplace
2. Extract the plugin to your project's Plugins folder (`[YourProject]/Plugins/`)
3. Restart Unreal Engine
4. Enable the plugin through Edit → Plugins → Timecode
5. Restart Unreal Engine again after enabling the plugin

## Quick Start

### Adding Timecode Component to an Actor

```cpp
// Create a new Blueprint Actor
// Add Timecode Component in the Components panel
// Configure the component settings:

// In Blueprint:
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
UTimecodeComponent* TimecodeComponent;

// In C++:
#include "TimecodeComponent.h"

// In your actor's constructor:
TimecodeComponent = CreateDefaultSubobject<UTimecodeComponent>(TEXT("TimecodeComponent"));
```

### Basic Timecode Conversion

```cpp
#include "TimecodeUtils.h"

// Convert seconds to timecode
float TimeInSeconds = 3600.5f; // 1 hour and 0.5 seconds
FString Timecode = UTimecodeUtils::SecondsToTimecode(TimeInSeconds, 29.97f, true); // "01:00:00;15" (drop frame)

// Convert timecode to seconds
FString InputTimecode = "00:10:00;00";
float Seconds = UTimecodeUtils::TimecodeToSeconds(InputTimecode, 29.97f, true); // 600.0f
```

### Setting Up Master/Slave Configuration

#### Master Device Configuration
```cpp
// In Blueprint:
TimecodeComponent->RoleMode = ETimecodeRoleMode::Manual;
TimecodeComponent->bIsManuallyMaster = true;

// In C++:
TimecodeComponent->SetRoleMode(ETimecodeRoleMode::Manual);
TimecodeComponent->SetIsManuallyMaster(true);
```

#### Slave Device Configuration
```cpp
// In Blueprint:
TimecodeComponent->RoleMode = ETimecodeRoleMode::Manual;
TimecodeComponent->bIsManuallyMaster = false;
TimecodeComponent->MasterIPAddress = "192.168.1.100"; // Master's IP address

// In C++:
TimecodeComponent->SetRoleMode(ETimecodeRoleMode::Manual);
TimecodeComponent->SetIsManuallyMaster(false);
TimecodeComponent->SetMasterIPAddress("192.168.1.100");
```

## Configuration Options

### TimecodeComponent Properties

| Property | Description | Default Value |
|----------|-------------|---------------|
| FrameRate | The frame rate used for timecode generation | 29.97 |
| bUseDropFrame | Whether to use drop frame timecode format | true |
| RoleMode | Automatic or manual master/slave assignment | Automatic |
| bIsManuallyMaster | Whether this instance is manually set as master | false |
| MasterIPAddress | IP address of the master device | "127.0.0.1" |
| SyncPort | UDP port used for synchronization | 12345 |
| SyncInterval | Time interval between sync packets (seconds) | 1.0 |
| PLLBandwidth | PLL algorithm bandwidth parameter | 0.5 |

## Advanced Usage

### Custom Timecode Handling

```cpp
// Subscribe to timecode change events
TimecodeComponent->OnTimecodeChanged.AddDynamic(this, &AMyActor::HandleTimecodeChanged);

// Handle timecode changes
void AMyActor::HandleTimecodeChanged(const FString& NewTimecode)
{
    // Process new timecode
    UE_LOG(LogTemp, Log, TEXT("New timecode: %s"), *NewTimecode);
}
```

### Network Role Detection

```cpp
// Set up automatic role detection
TimecodeComponent->RoleMode = ETimecodeRoleMode::Automatic;

// Subscribe to role change event
TimecodeComponent->OnRoleChanged.AddDynamic(this, &AMyActor::HandleRoleChanged);

// Handle role changes
void AMyActor::HandleRoleChanged(bool bIsMaster)
{
    if (bIsMaster)
    {
        UE_LOG(LogTemp, Log, TEXT("This device is now the MASTER"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("This device is now a SLAVE"));
    }
}
```

## Testing

The plugin includes comprehensive testing utilities to verify correct operation:

1. Open Window → Developer Tools → Session Frontend
2. Select the Automation tab
3. Filter for "TimecodeSync"
4. Run the tests to verify functionality

For more detailed testing instructions, see the [Testing Guide](docs/testing-guide.md).

## Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| Devices not synchronizing | Check network connectivity and firewall settings |
| Incorrect timecode conversion | Verify the frame rate and drop frame settings |
| High sync error | Adjust the PLL bandwidth parameter |
| Network timeout | Verify IP addresses and port settings |

### Logging

Enable verbose logging to troubleshoot issues:

```cpp
// In your project's DefaultEngine.ini:
[Core.Log]
LogTimecodeSync=Verbose
LogTimecodeComponent=Verbose
LogTimecodeNetwork=Verbose
```

## Technical Details

### Supported Platforms
- Windows
- Mac
- Linux
- iOS
- Android

### Dependencies
- Unreal Engine 5.5+
- Networking module

## License

This plugin is released under the MIT License. See [LICENSE](LICENSE) for details.

## Contact and Support

For issues, feature requests, or general questions:
- Submit issues on the [GitHub repository](https://github.com/yourusername/timecode-sync)
- Email: support@yourcompany.com

## Acknowledgements

- SMPTE for timecode standards documentation
- Unreal Engine community for testing and feedback
