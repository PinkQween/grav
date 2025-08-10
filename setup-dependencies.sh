#!/bin/bash

echo "🚀 Setting up Grav build dependencies..."

# Check if we're on Apple Silicon
if [[ $(uname -m) == "arm64" ]]; then
    echo "📱 Detected Apple Silicon Mac"
    
    # Install Apple Silicon Homebrew if not present
    if ! command -v /opt/homebrew/bin/brew &> /dev/null; then
        echo "🍺 Installing Apple Silicon Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    else
        echo "✅ Apple Silicon Homebrew already installed"
    fi
    
    # Install GLFW for arm64
    echo "📦 Installing GLFW for arm64..."
    /opt/homebrew/bin/brew install glfw
    
    # Install Intel Homebrew if not present
    if ! command -v /usr/local/bin/brew &> /dev/null; then
        echo "🍺 Installing Intel Homebrew (via Rosetta 2)..."
        arch -x86_64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    else
        echo "✅ Intel Homebrew already installed"
    fi
    
    # Install GLFW for x86_64
    echo "📦 Installing GLFW for x86_64..."
    arch -x86_64 /usr/local/bin/brew install glfw
    
    echo ""
    echo "🔍 Verifying installations:"
    echo "arm64 GLFW: $(lipo -info /opt/homebrew/lib/libglfw.dylib 2>/dev/null || echo 'Not found')"
    echo "x86_64 GLFW: $(lipo -info /usr/local/lib/libglfw.dylib 2>/dev/null || echo 'Not found')"
    
else
    echo "💻 Detected Intel Mac"
    
    # Install regular Homebrew if not present
    if ! command -v brew &> /dev/null; then
        echo "🍺 Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    else
        echo "✅ Homebrew already installed"
    fi
    
    # Install GLFW
    echo "📦 Installing GLFW..."
    brew install glfw
    
    echo ""
    echo "🔍 Verifying installation:"
    echo "GLFW: $(lipo -info /usr/local/lib/libglfw.dylib 2>/dev/null || echo 'Not found')"
fi

echo ""
echo "✨ Setup complete! You can now build with:"
echo "   make            # Build for native architecture"
echo "   make arm64      # Build for Apple Silicon"
echo "   make x86_64     # Build for Intel"
echo "   make universal  # Build universal binary"
