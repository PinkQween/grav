APP_NAME = Grav
BINARY = grav
SRC = main.cpp

# Default to native architecture
ARCH ?= $(shell uname -m)

# Architecture-specific flags
# Local development: Use dual Homebrew setup
# CI: Handle Apple Silicon runners differently
ifeq ($(ARCH),arm64)
    CFLAGS = -std=c++11 -arch arm64 -I/opt/homebrew/include
    LDFLAGS = -arch arm64 -L/opt/homebrew/lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit
else ifeq ($(ARCH),x86_64)
    # For local development, use Intel Homebrew paths
    # For CI on Apple Silicon runners, this will be overridden by environment
    CFLAGS = -std=c++11 -arch x86_64 -I/usr/local/include
    LDFLAGS = -arch x86_64 -L/usr/local/lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit
else
    # Fallback to native architecture
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
	clang++ $(SRC) -std=c++11 $(CFLAGS) $(LDFLAGS) -o $(APP_NAME).app/Contents/MacOS/$(BINARY)

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
	
	# Build x86_64 version using local Intel Homebrew
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
