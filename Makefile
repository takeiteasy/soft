default:
	clang example.c src/surface.c src/*.m -x objective-c -fno-objc-arc -framework Cocoa -framework AppKit -framework OpengGL -Iinclude -o test
