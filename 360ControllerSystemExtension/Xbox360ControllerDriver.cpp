#include "Xbox360ControllerDriver.h"

#define super IOUSBHostDevice
OSDefineMetaClassAndStructors(Xbox360ControllerDriver, IOUSBHostDevice);

kern_return_t Xbox360ControllerDriver::Start(IOService * provider)
{
    kern_return_t ret = kIOReturnSuccess;
    
    ret = super::Start(provider);
    if (ret != kIOReturnSuccess) {
        return ret;
    }
    
    os_log(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Starting system extension");
    
    // Initialize USB interface
    interface = OSDynamicCast(IOUSBHostInterface, provider);
    if (!interface) {
        os_log_error(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Failed to cast provider to IOUSBHostInterface");
        return kIOReturnInvalid;
    }
    
    // Open the interface
    ret = interface->Open(this, 0, nullptr);
    if (ret != kIOReturnSuccess) {
        os_log_error(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Failed to open interface: 0x%x", ret);
        return ret;
    }
    
    // Find and configure pipes
    const IOUSBConfigurationDescriptor * configDesc = nullptr;
    const IOUSBInterfaceDescriptor * interfaceDesc = nullptr;
    
    ret = interface->CopyConfigurationDescriptor(&configDesc);
    if (ret != kIOReturnSuccess || !configDesc) {
        os_log_error(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Failed to get configuration descriptor");
        return kIOReturnError;
    }
    
    ret = interface->CopyInterfaceDescriptor(&interfaceDesc);
    if (ret != kIOReturnSuccess || !interfaceDesc) {
        os_log_error(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Failed to get interface descriptor");
        return kIOReturnError;
    }
    
    // Set up input and output pipes for Xbox 360 controller
    for (uint8_t pipeIndex = 0; pipeIndex < interfaceDesc->bNumEndpoints; pipeIndex++) {
        const IOUSBEndpointDescriptor * endpointDesc = nullptr;
        ret = interface->CopyEndpointDescriptor(0, pipeIndex, &endpointDesc);
        
        if (ret == kIOReturnSuccess && endpointDesc) {
            if ((endpointDesc->bmAttributes & 0x03) == kUSBInterrupt) {
                IOUSBHostPipe * pipe = nullptr;
                ret = interface->CopyPipe(pipeIndex, &pipe);
                
                if (ret == kIOReturnSuccess && pipe) {
                    if ((endpointDesc->bEndpointAddress & 0x80) == 0x80) {
                        // Input pipe (controller -> host)
                        inPipe = pipe;
                        os_log(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Found input pipe");
                    } else {
                        // Output pipe (host -> controller)
                        outPipe = pipe;
                        os_log(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Found output pipe");
                    }
                }
            }
        }
    }
    
    if (!inPipe || !outPipe) {
        os_log_error(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Failed to find required pipes");
        return kIOReturnError;
    }
    
    // Start reading from controller
    ret = StartAsyncReads();
    if (ret != kIOReturnSuccess) {
        os_log_error(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Failed to start async reads: 0x%x", ret);
        return ret;
    }
    
    os_log(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Successfully started");
    return kIOReturnSuccess;
}

kern_return_t Xbox360ControllerDriver::Stop(IOService * provider)
{
    os_log(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Stopping system extension");
    
    if (inPipe) {
        inPipe->Abort();
        inPipe->Release();
        inPipe = nullptr;
    }
    
    if (outPipe) {
        outPipe->Abort();
        outPipe->Release();
        outPipe = nullptr;
    }
    
    if (interface) {
        interface->Close(this, 0);
        interface = nullptr;
    }
    
    return super::Stop(provider);
}

kern_return_t Xbox360ControllerDriver::NewUserClient(uint32_t type, IOUserClient ** userClient)
{
    // Create user client for application communication
    return super::NewUserClient(type, userClient);
}

kern_return_t Xbox360ControllerDriver::StartAsyncReads()
{
    if (!inPipe) {
        return kIOReturnError;
    }
    
    // Create buffer for controller input
    const size_t inputBufferSize = 20; // Xbox 360 controller input report size
    
    IOBufferMemoryDescriptor * inputBuffer = nullptr;
    kern_return_t ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionIn, inputBufferSize, 0, &inputBuffer);
    
    if (ret != kIOReturnSuccess || !inputBuffer) {
        return ret;
    }
    
    // Create completion for async read
    IOUSBHostIOSource * completion = nullptr;
    ret = CreateIOUSBHostIOSource(&completion, ^(IOReturn status, uint32_t actualByteCount) {
        if (status == kIOReturnSuccess) {
            // Process controller data
            this->HandleControllerData(inputBuffer, actualByteCount);
            
            // Continue reading
            this->StartAsyncReads();
        } else if (status != kIOReturnAborted) {
            os_log_error(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Read error: 0x%x", status);
            // Retry after delay
            this->StartAsyncReads();
        }
    });
    
    if (ret == kIOReturnSuccess && completion) {
        ret = inPipe->AsyncIO(inputBuffer, inputBufferSize, completion, 0);
    }
    
    inputBuffer->Release();
    return ret;
}

kern_return_t Xbox360ControllerDriver::HandleControllerData(IOMemoryDescriptor * data, uint32_t length)
{
    if (!data || length < 3) {
        return kIOReturnBadArgument;
    }
    
    // Read the controller data
    uint8_t buffer[20];
    if (data->ReadBytes(0, buffer, length) != kIOReturnSuccess) {
        return kIOReturnError;
    }
    
    // Check for Xbox 360 controller input report (type 0x00, size 0x14)
    if (buffer[0] == 0x00 && buffer[1] == 0x14) {
        // This is Xbox 360 controller input data
        // Convert to HID gamepad format and send to system
        ConvertXbox360ToHID(buffer + 2, length - 2);
    }
    
    return kIOReturnSuccess;
}

kern_return_t Xbox360ControllerDriver::ConvertXbox360ToHID(const uint8_t * xbox360Data, uint32_t length)
{
    // Convert Xbox 360 controller format to standard HID gamepad format
    // This would create HID reports and send them to the HID system
    
    if (length < 18) {
        return kIOReturnBadArgument;
    }
    
    // Xbox 360 format:
    // [0-1]: Left stick X/Y
    // [2-3]: Right stick X/Y  
    // [4]: Left trigger
    // [5]: Right trigger
    // [6-7]: Buttons
    
    // Create HID report for gamepad
    uint8_t hidReport[8];
    
    // Map buttons (simplified mapping)
    hidReport[0] = xbox360Data[6]; // Button byte 1
    hidReport[1] = xbox360Data[7]; // Button byte 2
    
    // Map analog sticks (convert from signed 16-bit to unsigned 8-bit)
    hidReport[2] = (xbox360Data[0] + 128); // Left X
    hidReport[3] = (xbox360Data[1] + 128); // Left Y
    hidReport[4] = (xbox360Data[2] + 128); // Right X
    hidReport[5] = (xbox360Data[3] + 128); // Right Y
    
    // Map triggers
    hidReport[6] = xbox360Data[4]; // Left trigger
    hidReport[7] = xbox360Data[5]; // Right trigger
    
    // Send HID report to system (implementation would depend on HID framework)
    // This is where you'd integrate with IOHIDDevice or similar
    
    return kIOReturnSuccess;
}

kern_return_t Xbox360ControllerDriver::SendControllerCommand(const uint8_t * command, size_t length)
{
    // Send commands to Xbox 360 controller (e.g., LED control, rumble)
    if (!outPipe || !command || length == 0) {
        return kIOReturnBadArgument;
    }
    
    IOBufferMemoryDescriptor * buffer = nullptr;
    kern_return_t ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionOut, length, 0, &buffer);
    
    if (ret != kIOReturnSuccess || !buffer) {
        return ret;
    }
    
    ret = buffer->SetBytes(0, command, length);
    if (ret != kIOReturnSuccess) {
        buffer->Release();
        return ret;
    }
    
    IOUSBHostIOSource * completion = nullptr;
    ret = CreateIOUSBHostIOSource(&completion, ^(IOReturn status, uint32_t actualByteCount) {
        if (status != kIOReturnSuccess) {
            os_log_error(OS_LOG_DEFAULT, "Xbox360ControllerDriver: Write error: 0x%x", status);
        }
    });
    
    if (ret == kIOReturnSuccess) {
        ret = outPipe->AsyncIO(buffer, length, completion, 0);
    }
    
    buffer->Release();
    return ret;
}