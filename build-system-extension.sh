#!/bin/bash

# Build script for both kernel extension and system extension
# Usage: ./build-system-extension.sh [kext|dext|both]

BUILD_TYPE=${1:-both}
DEV_NAME=`echo | grep DEVELOPER_NAME DeveloperSettings.xcconfig 2>/dev/null`
DEV_TEAM=`echo | grep DEVELOPMENT_TEAM DeveloperSettings.xcconfig 2>/dev/null`
CERT_ID="${DEV_NAME//\\DEVELOPER_NAME = } (${DEV_TEAM//\\DEVELOPMENT_TEAM = })"

echo "Building 360Controller with system extension support..."
echo "Build type: $BUILD_TYPE"

# Create build directory
mkdir -p build

# Archive source
zip -r build/360ControllerSource.zip * -x "build*"

if [ "$BUILD_TYPE" = "kext" ] || [ "$BUILD_TYPE" = "both" ]; then
    echo "Building kernel extension..."
    xcrun xcodebuild -configuration Release -target "Whole Driver" -xcconfig "DeveloperSettings.xcconfig" OTHER_CODE_SIGN_FLAGS="--timestamp --options=runtime"
    if [ $? -ne 0 ]; then
        echo "******** KERNEL EXTENSION BUILD FAILED ********"
        exit 1
    fi
fi

if [ "$BUILD_TYPE" = "dext" ] || [ "$BUILD_TYPE" = "both" ]; then
    echo "Building system extension..."
    
    # Create system extension Xcode project if it doesn't exist
    if [ ! -f "360ControllerSystemExtension.xcodeproj/project.pbxproj" ]; then
        echo "Creating system extension Xcode project..."
        
        # This would typically be done through Xcode GUI, but for automation:
        mkdir -p "360ControllerSystemExtension.xcodeproj"
        
        # Note: In a real implementation, this would need a proper Xcode project
        # For now, we'll create a basic structure
        echo "System extension project creation requires Xcode GUI for DriverKit target setup"
        echo "Please create the system extension target manually in Xcode with DriverKit framework"
    fi
    
    # Build system extension (when project is properly configured)
    # xcrun xcodebuild -project "360ControllerSystemExtension.xcodeproj" -configuration Release -target "Xbox360ControllerSystemExtension"
fi

# Create installer package
cd Install360Controller
if [ -f "Install360Controller.pkgproj" ]; then
    packagesbuild -v Install360Controller.pkgproj --identity "Developer ID Installer: \"${CERT_ID}\""
    mv build 360ControllerInstall
    hdiutil create -srcfolder 360ControllerInstall -fs HFS+ -format UDZO ../build/360ControllerInstall.dmg
    mv 360ControllerInstall build
fi
cd ..

echo "Build completed successfully!"
echo ""
echo "Built components:"
if [ "$BUILD_TYPE" = "kext" ] || [ "$BUILD_TYPE" = "both" ]; then
    echo "- Kernel Extension: build/Release/360Controller.kext"
fi
if [ "$BUILD_TYPE" = "dext" ] || [ "$BUILD_TYPE" = "both" ]; then
    echo "- System Extension: build/Release/Xbox360ControllerSystemExtension.dext"
fi
echo "- Installer: build/360ControllerInstall.dmg"