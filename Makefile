APP_NAME = Grav
BINARY = grav
SRC = main.cpp

# Default to native architecture
ARCH ?= $(shell uname -m)

# Architecture-specific flags
# We now have GLFW installed for both architectures:
# - arm64: /opt/homebrew (Apple Silicon Homebrew)
# - x86_64: /usr/local (Intel Homebrew via Rosetta)
# Added fallback paths for CI environments
ifeq ($(ARCH),arm64)
    # Primary paths for arm64
    ARM64_INCLUDE_PATHS = -I/opt/homebrew/include
    ARM64_LIB_PATHS = -L/opt/homebrew/lib
    # Fallback paths
    ifneq ($(HOMEBREW_PREFIX),)
        ARM64_INCLUDE_PATHS += -I$(HOMEBREW_PREFIX)/include
        ARM64_LIB_PATHS += -L$(HOMEBREW_PREFIX)/lib
    endif
    CFLAGS = -std=c++11 -arch arm64 $(ARM64_INCLUDE_PATHS)
    LDFLAGS = -arch arm64 $(ARM64_LIB_PATHS) -lglfw -framework Cocoa -framework OpenGL -framework IOKit
else ifeq ($(ARCH),x86_64)
    # Primary paths for x86_64
    X86_INCLUDE_PATHS = -I/usr/local/include
    X86_LIB_PATHS = -L/usr/local/lib
    # Fallback paths for CI environments
    ifneq ($(HOMEBREW_PREFIX),)
        X86_INCLUDE_PATHS += -I$(HOMEBREW_PREFIX)/include
        X86_LIB_PATHS += -L$(HOMEBREW_PREFIX)/lib
    endif
    # Additional common locations
    X86_INCLUDE_PATHS += -I/usr/local/Cellar/glfw/*/include
    X86_LIB_PATHS += -L/usr/local/Cellar/glfw/*/lib
    CFLAGS = -std=c++11 -arch x86_64 $(X86_INCLUDE_PATHS)
    LDFLAGS = -arch x86_64 $(X86_LIB_PATHS) -lglfw -framework Cocoa -framework OpenGL -framework IOKit
else
    # Fallback to native architecture homebrew paths
    ifeq ($(shell uname -m),arm64)
        CFLAGS = -std=c++11 -I/opt/homebrew/include
        LDFLAGS = -L/opt/homebrew/lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit
    else
        CFLAGS = -std=c++11 -I/usr/local/include
        LDFLAGS = -L/usr/local/lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit
    endif
endif

RESOURCES = Resources/
PLIST = Info.plist

all: clean $(APP_NAME).app/Contents/MacOS/$(BINARY)

$(APP_NAME).app/Contents/MacOS/$(BINARY): $(SRC) $(RESOURCES) $(PLIST)
	@killall grav || true
	@mkdir -p $(APP_NAME).app/Contents/MacOS
	@mkdir -p $(APP_NAME).app/Contents/Resources
	clang++ $(SRC) $(CFLAGS) $(LDFLAGS) -o $(APP_NAME).app/Contents/MacOS/$(BINARY)

	# Copy the icon files and resources
	@cp -r $(RESOURCES)* $(APP_NAME).app/Contents/Resources/
	@cp $(PLIST) $(APP_NAME).app/Contents/
	
# Build for specific architectures
arm64:
	$(MAKE) ARCH=arm64

x86_64:
	$(MAKE) ARCH=x86_64

# Build universal binary (requires both architectures)
universal: clean
	@mkdir -p build/arm64/$(APP_NAME).app/Contents/MacOS build/arm64/$(APP_NAME).app/Contents/Resources
	@mkdir -p build/x86_64/$(APP_NAME).app/Contents/MacOS build/x86_64/$(APP_NAME).app/Contents/Resources
	
	# Build arm64 version
	clang++ $(SRC) -std=c++11 -arch arm64 -I/opt/homebrew/include -arch arm64 -L/opt/homebrew/lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit -o build/arm64/$(APP_NAME).app/Contents/MacOS/$(BINARY)
	@cp -r $(RESOURCES)* build/arm64/$(APP_NAME).app/Contents/Resources/
	@cp $(PLIST) build/arm64/$(APP_NAME).app/Contents/
	
	# Build x86_64 version
	clang++ $(SRC) -std=c++11 -arch x86_64 -I/usr/local/include -arch x86_64 -L/usr/local/lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit -o build/x86_64/$(APP_NAME).app/Contents/MacOS/$(BINARY)
	@cp -r $(RESOURCES)* build/x86_64/$(APP_NAME).app/Contents/Resources/
	@cp $(PLIST) build/x86_64/$(APP_NAME).app/Contents/
	
	# Create universal app bundle
	@mkdir -p $(APP_NAME).app/Contents/MacOS
	@mkdir -p $(APP_NAME).app/Contents/Resources
	lipo -create -output $(APP_NAME).app/Contents/MacOS/$(BINARY) \
		build/arm64/$(APP_NAME).app/Contents/MacOS/$(BINARY) \
		build/x86_64/$(APP_NAME).app/Contents/MacOS/$(BINARY)
	@cp -r $(RESOURCES)* $(APP_NAME).app/Contents/Resources/
	@cp $(PLIST) $(APP_NAME).app/Contents/
	@echo "Universal binary created"

# Package for distribution
package: all
	@mkdir -p dist
	zip -r dist/grav-$(ARCH).zip $(APP_NAME).app
	@echo "Package created: dist/grav-$(ARCH).zip"

package-universal: universal
	@mkdir -p dist
	zip -r dist/grav-universal.zip $(APP_NAME).app
	@echo "Universal package created: dist/grav-universal.zip"

clean:
	rm -rf $(APP_NAME).app build/ dist/

.PHONY: all arm64 x86_64 universal package package-universal clean
