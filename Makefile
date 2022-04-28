default:
	clang example.c src/surface.c src/window_mac.m -x objective-c -fno-objc-arc -framework Cocoa -framework AppKit -Iinclude -o test
