APP_NAME = Grav
BINARY = grav
SRC = main.cpp
CFLAGS = -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit
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
	
clean:
	rm -rf $(APP_NAME).app