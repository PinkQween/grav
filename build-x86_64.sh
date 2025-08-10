#!/bin/bash

echo "🔨 Building for x86_64..."

# Check if we're on Apple Silicon
if [ "$(uname -m)" = "arm64" ]; then
    echo "💻 Building x86_64 on Apple Silicon..."
    
    # Check if x86_64 Homebrew is available
    if [ -d "/usr/local/Homebrew" ] || [ -f "/usr/local/bin/brew" ]; then
        echo "📦 Using x86_64 Homebrew for dependencies..."
        
        # Use x86_64 Homebrew paths
        X86_BREW_PREFIX="/usr/local"
        
        # Check if GLFW is installed for x86_64
        if [ ! -f "$X86_BREW_PREFIX/lib/libglfw.dylib" ]; then
            echo "❌ GLFW not found for x86_64. Please install it first:"
            echo "   arch -x86_64 /usr/local/bin/brew install glfw"
            exit 1
        fi
        
        # Verify x86_64 GLFW
        GLFW_ARCH=$(lipo -info "$X86_BREW_PREFIX/lib/libglfw.dylib" 2>/dev/null | grep -o "x86_64" || echo "")
        if [ -z "$GLFW_ARCH" ]; then
            echo "❌ GLFW library is not x86_64 compatible"
            exit 1
        fi
        
        echo "✅ Found x86_64 GLFW library"
        
        # Build directories
        mkdir -p Grav.app/Contents/MacOS
        mkdir -p Grav.app/Contents/Resources
        
        # Compile for x86_64
        clang++ main.cpp \
          -std=c++11 \
          -arch x86_64 \
          -I"$X86_BREW_PREFIX/include" \
          -L"$X86_BREW_PREFIX/lib" \
          -lglfw \
          -framework Cocoa \
          -framework OpenGL \
          -framework IOKit \
          -o Grav.app/Contents/MacOS/grav
        
        if [ $? -eq 0 ]; then
            echo "✅ x86_64 build successful!"
        else
            echo "❌ x86_64 build failed"
            exit 1
        fi
        
    else
        echo "❌ x86_64 Homebrew not found. Installing..."
        echo "💡 To install x86_64 Homebrew:"
        echo "   arch -x86_64 /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        echo "   arch -x86_64 /usr/local/bin/brew install glfw"
        exit 1
    fi
    
else
    echo "💻 Building on Intel Mac..."
    # Use regular Makefile for Intel Macs
    make clean
    make
fi

# Copy resources
cp -r Resources/* Grav.app/Contents/Resources/ 2>/dev/null || echo "⚠️  No resources to copy"
cp Info.plist Grav.app/Contents/ 2>/dev/null || echo "⚠️  No Info.plist to copy"

# Verify the build
echo "🔍 Verifying build:"
file Grav.app/Contents/MacOS/grav

if [ -f "Grav.app/Contents/MacOS/grav" ]; then
    # Create distribution package
    mkdir -p dist
    zip -r dist/grav-x86_64.zip Grav.app
    zip -j dist/grav-x86_64-binary.zip Grav.app/Contents/MacOS/grav
    echo "📦 Distribution packages created in dist/"
    echo "✅ Build completed successfully!"
else
    echo "❌ Build verification failed - binary not found"
    exit 1
fi
