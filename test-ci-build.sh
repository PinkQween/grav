#!/bin/bash

echo "🧪 Testing CI build process locally..."

# Test arm64 build
echo "📱 Testing arm64 build..."
make clean
if make ARCH=arm64; then
    echo "✅ arm64 build successful"
    file Grav.app/Contents/MacOS/grav
else
    echo "❌ arm64 build failed"
    exit 1
fi

echo ""

# Test x86_64 build
echo "💻 Testing x86_64 build..."
make clean
if make ARCH=x86_64; then
    echo "✅ x86_64 build successful"
    file Grav.app/Contents/MacOS/grav
else
    echo "❌ x86_64 build failed"
    exit 1
fi

echo ""

# Test universal build
echo "🌍 Testing universal build..."
make clean
if make universal; then
    echo "✅ Universal build successful"
    lipo -info Grav.app/Contents/MacOS/grav
else
    echo "❌ Universal build failed"
    exit 1
fi

echo ""

# Test packaging
echo "📦 Testing packaging..."
if make package-universal; then
    echo "✅ Packaging successful"
    ls -la dist/
else
    echo "❌ Packaging failed"
    exit 1
fi

echo ""
echo "🎉 All CI build tests passed! The GitHub Actions workflow should work correctly."
