#!/bin/bash

echo "🔍 GLFW Installation Diagnostic"
echo "================================"

echo ""
echo "📱 System Information:"
echo "Architecture: $(uname -m)"
echo "macOS Version: $(sw_vers -productVersion)"

echo ""
echo "🍺 Homebrew Installations:"

# Check Apple Silicon Homebrew
if [ -d "/opt/homebrew" ]; then
    echo "✅ Apple Silicon Homebrew found at /opt/homebrew"
    if [ -f "/opt/homebrew/bin/brew" ]; then
        echo "   Version: $(/opt/homebrew/bin/brew --version | head -1)"
    fi
    
    if [ -d "/opt/homebrew/include/GLFW" ]; then
        echo "   ✅ GLFW headers found: /opt/homebrew/include/GLFW"
        ls -la /opt/homebrew/include/GLFW/ | head -3
    else
        echo "   ❌ GLFW headers not found in /opt/homebrew/include/"
    fi
    
    if ls /opt/homebrew/lib/libglfw* 2>/dev/null; then
        echo "   ✅ GLFW libraries found:"
        ls -la /opt/homebrew/lib/libglfw*
        lipo -info /opt/homebrew/lib/libglfw.dylib 2>/dev/null || echo "   (Cannot read architecture info)"
    else
        echo "   ❌ GLFW libraries not found in /opt/homebrew/lib/"
    fi
else
    echo "❌ Apple Silicon Homebrew not found at /opt/homebrew"
fi

echo ""

# Check Intel Homebrew
if [ -d "/usr/local" ]; then
    echo "✅ Intel Homebrew location found at /usr/local"
    if [ -f "/usr/local/bin/brew" ]; then
        echo "   Version: $(/usr/local/bin/brew --version | head -1)"
    fi
    
    if [ -d "/usr/local/include/GLFW" ]; then
        echo "   ✅ GLFW headers found: /usr/local/include/GLFW"
        ls -la /usr/local/include/GLFW/ | head -3
    else
        echo "   ❌ GLFW headers not found in /usr/local/include/"
    fi
    
    if ls /usr/local/lib/libglfw* 2>/dev/null; then
        echo "   ✅ GLFW libraries found:"
        ls -la /usr/local/lib/libglfw*
        lipo -info /usr/local/lib/libglfw.dylib 2>/dev/null || echo "   (Cannot read architecture info)"
    else
        echo "   ❌ GLFW libraries not found in /usr/local/lib/"
    fi
else
    echo "❌ Intel Homebrew location not found at /usr/local"
fi

echo ""
echo "🧪 Compilation Tests:"

# Test compilation for each architecture
for arch in arm64 x86_64; do
    echo ""
    echo "Testing $arch compilation..."
    
    # Choose appropriate include path
    if [ "$arch" = "arm64" ]; then
        include_path="/opt/homebrew/include"
    else
        include_path="/usr/local/include"
    fi
    
    # Create test program
    cat > test_$arch.cpp << EOF
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    if (glfwInit()) {
        std::cout << "GLFW initialized successfully for $arch" << std::endl;
        glfwTerminate();
        return 0;
    } else {
        std::cout << "Failed to initialize GLFW for $arch" << std::endl;
        return 1;
    }
}
EOF
    
    # Test compilation
    if clang++ -arch $arch -I$include_path -std=c++11 test_$arch.cpp -c -o test_$arch.o 2>/dev/null; then
        echo "   ✅ Headers compilation test passed for $arch"
        rm -f test_$arch.o
    else
        echo "   ❌ Headers compilation test failed for $arch"
        echo "   Trying with verbose output:"
        clang++ -arch $arch -I$include_path -std=c++11 test_$arch.cpp -c -o test_$arch.o -v
    fi
    
    rm -f test_$arch.cpp test_$arch.o
done

echo ""
echo "📋 Makefile Variable Preview:"
echo "Current ARCH setting: ${ARCH:-$(uname -m)}"

# Test make variables
for test_arch in arm64 x86_64; do
    echo ""
    echo "For ARCH=$test_arch:"
    make -n ARCH=$test_arch 2>&1 | grep "clang++" | head -1 | sed 's/clang++/   clang++/'
done

echo ""
echo "🔧 Recommendations:"
echo ""
if [ "$(uname -m)" = "arm64" ]; then
    echo "On Apple Silicon Mac:"
    echo "1. Install Apple Silicon GLFW: brew install glfw"
    echo "2. Install Intel GLFW: arch -x86_64 /usr/local/bin/brew install glfw"
    echo "3. Run setup script: ./setup-dependencies.sh"
else
    echo "On Intel Mac:"
    echo "1. Install GLFW: brew install glfw"
fi

echo ""
echo "🎯 Next Steps:"
echo "- If headers are missing, reinstall GLFW for the target architecture"
echo "- If compilation fails, check the clang++ command line above"
echo "- For CI issues, ensure both Homebrew installations are present"
