#!/bin/bash

# Verify ARM64 Configuration Script
# This script checks if the project is properly configured for ARM64 builds

echo "360Controller ARM64 Configuration Checker"
echo "========================================"

# Check if DeveloperSettings.xcconfig exists
if [ ! -f "DeveloperSettings.xcconfig" ]; then
    echo "❌ DeveloperSettings.xcconfig not found"
    echo "   Please create this file with your signing information"
    echo "   See README.md for details"
    exit 1
else
    echo "✅ DeveloperSettings.xcconfig found"
fi

# Check if ARM64 architectures are configured
if grep -q "arm64" DeveloperSettings.xcconfig; then
    echo "✅ ARM64 architecture configured in DeveloperSettings.xcconfig"
else
    echo "❌ ARM64 architecture not found in DeveloperSettings.xcconfig"
    echo "   Add: ARCHS = x86_64 arm64"
    echo "   Add: VALID_ARCHS = x86_64 arm64"
fi

# Check deployment target
if grep -q "MACOSX_DEPLOYMENT_TARGET.*=.*1[1-9]" DeveloperSettings.xcconfig; then
    echo "✅ macOS deployment target set to 11.0 or later (required for ARM64)"
else
    echo "⚠️  Consider setting MACOSX_DEPLOYMENT_TARGET = 11.0 or later for ARM64 support"
fi

# Check if old architecture settings were removed from project
if grep -q "ARCHS_STANDARD_32_64_BIT" "360 Driver.xcodeproj/project.pbxproj"; then
    echo "❌ Old 32/64-bit architecture settings found in project"
    echo "   These should be removed to use the ARM64 configuration"
else
    echo "✅ Old architecture settings cleaned up from project"
fi

echo ""
echo "Configuration check complete!"
echo ""
echo "To build universal binaries (Intel + Apple Silicon):"
echo "1. Update DeveloperSettings.xcconfig with your signing information"
echo "2. Run: ./build.sh"
echo "3. Check the lipo output to verify both architectures are included"