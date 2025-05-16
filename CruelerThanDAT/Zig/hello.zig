const std = @import("std");

// You should never really deploy builds that use this, it's only for debugging,
// but it's easier to get started with it.
// This function is just the Zig equivalent of printf as a shortcut to stderr.
// https://ziglang.org/documentation/0.14.0/std/#std.debug.print
const print = std.debug.print;

// https://ziglang.org/documentation/0.14.0/std/#std.mem.span
const span = std.mem.span;


export	// Declares the function to be exported as if for a library. In Zig, this means
		// that the function will use a standard C ABI instead of doing weird Ziggy things.
		// When building this file, the msvc in the target triple means things like this are
		// gonna use the msvc ABI. gnu, musl and others can also be specified if you wanna
		// target a different compiler. You can also set it to native which should use the
		// default of the computer you're compiling on, but I'm not sure if that's always msvc
		// on Windows.

// We're gonna use C types in the signature since we wanna call this function
// from C/C++, which isn't gonna like Ziggy things. It *might* be fine if we
// do Zig types the same size as the C ones, like u32, but I'm not sure.
fn HelloZig(msg: [*]const c_char, num: c_int)

// callconv specifies the calling convention for this function. In layman's terms, how the
// parameters and the return should be used, in which registers are they expected? Or is it
// in memory rather, things like that. Zig does weird stuff with the calling convention by
// default but with this, it's gonna do it the same way a C compiler would, at least for this
// function. Other calling conventions can be specified, `.C` is an inferred enum constant.
callconv(.C) void {
	// Convert the C string to..still a C string, but now it's u8 instead of c_char,
	// and the type also specifies we expect a null terminator, since it's a guarantee
	// for C++ string literals. If this string doesn't come from a literal, that *might*
	// not be a guarantee.
	const ptr: [*:0]const u8 = @ptrCast(msg);

	// span will go walk through pointer like strlen to return a regular, null terminated Zig slice.
	const slice: [:0]const u8 = span(ptr);

	// Should be self explanatory, but it works kinda like in C#.
	print("{s}{}\n", .{ slice, num, });
}
