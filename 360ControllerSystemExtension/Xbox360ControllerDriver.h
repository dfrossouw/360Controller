// DriverKit-based System Extension for Xbox 360 Controller
// This replaces the IOKit kernel extension with a modern user-space driver

#ifndef Xbox360ControllerDriver_h
#define Xbox360ControllerDriver_h

#include <DriverKit/DriverKit.h>
#include <USBDriverKit/USBDriverKit.h>
#include <HIDDriverKit/HIDDriverKit.h>
#include <os/log.h>

class Xbox360ControllerDriver : public IOUSBHostDevice
{
    OSDeclareDefaultStructors(Xbox360ControllerDriver);
    
public:
    virtual kern_return_t Start(IOService * provider) override;
    virtual kern_return_t Stop(IOService * provider) override;
    
    virtual kern_return_t NewUserClient(uint32_t type, IOUserClient ** userClient) override;
    
private:
    IOUSBHostInterface * interface;
    IOUSBHostPipe * inPipe;
    IOUSBHostPipe * outPipe;
    
    kern_return_t StartAsyncReads();
    kern_return_t HandleControllerData(IOMemoryDescriptor * data, uint32_t length);
    kern_return_t ConvertXbox360ToHID(const uint8_t * xbox360Data, uint32_t length);
    kern_return_t SendControllerCommand(const uint8_t * command, size_t length);
};

#endif /* Xbox360ControllerDriver_h */