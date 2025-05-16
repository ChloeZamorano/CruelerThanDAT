const std = @import("std");

// You should never really deploy builds that use this, it's only for debugging,
// but it's easier to get started with it.
// This function is just the Zig equivalent of printf as a shortcut to stderr.
const print = std.debug.print;

const span = std.mem.span;

// We're gonna use C types in the signature since we wanna call this function
// from C/C++, which isn't gonna like Ziggy things.
// We also pass the length because I'm too lazy to implement my own strlen for this.
export fn HelloZig(msg: [*]const c_char, num: c_int) callconv(.C) void {
	// Convert the C style string to a Zig slice of u8 which print expects.
	const ptr: [*:0]const u8 = @ptrCast(msg);

	// We build an slice from the pointer and the length.
	// The cast functions usually (but not always) expect you to specify the target type.
	// Above we just specify the type of ptr explicitly, and @ptrCast infers it.
	// Here we're not doing that, instead we're using @as to specify the type explicitly
	// so we can dereference it directly here, instead of making slice a pointer and
	// dereferencing it in print.
	const slice: [:0]const u8 = span(ptr);

	// Should be self explanatory, but it works kinda like in C#.
	print("{s}{}\n", .{ slice, num, });
}
