#!/bin/bash

echo "🧪 Testing x86_64 cross-compilation build..."
echo "Runner architecture: $(uname -m)"

# Simulate the CI environment check
if [ "$(uname -m)" = "arm64" ]; then
  echo "💻 Building x86_64 on Apple Silicon - using x86_64 GLFW..."
  
  # Check if x86_64 GLFW is available
  if [ -f "/usr/local/lib/libglfw.dylib" ]; then
    echo "✅ x86_64 GLFW found: $(lipo -info /usr/local/lib/libglfw.dylib)"
  else
    echo "❌ x86_64 GLFW not found at /usr/local/lib/"
    exit 1
  fi
  
  # Try dynamic linking first
  echo "🔨 Attempting dynamic linking..."
  clang++ main.cpp \
    -std=c++11 \
    -arch x86_64 \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lglfw \
    -framework Cocoa \
    -framework OpenGL \
    -framework IOKit \
    -o test-grav-x86_64 && {
    echo "✅ Dynamic linking successful"
    file test-grav-x86_64
  } || {
    echo "❌ Dynamic linking failed, trying static..."
    
    # Fallback to static linking
    clang++ main.cpp \
      -std=c++11 \
      -arch x86_64 \
      -I/usr/local/include \
      -L/usr/local/lib \
      /usr/local/lib/libglfw3.a \
      -framework Cocoa \
      -framework OpenGL \
      -framework IOKit \
      -o test-grav-x86_64 && {
      echo "✅ Static linking successful"
      file test-grav-x86_64
    } || {
      echo "❌ Both dynamic and static linking failed"
      exit 1
    }
  }
  
  # Clean up
  rm -f test-grav-x86_64
  echo "✅ x86_64 cross-compilation test passed!"
  
else
  echo "ℹ️  Running on x86_64 - no cross-compilation needed"
  make clean
  make ARCH=x86_64
  file Grav.app/Contents/MacOS/grav
fi
