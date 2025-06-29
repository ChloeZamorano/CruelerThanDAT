const std = @import("std");
const Compile = std.Build.Step.Compile;
const allocPrint = std.fmt.allocPrint;
const getEnvVarOwned = std.process.getEnvVarOwned;

const zcc = @import("zcc");

pub fn build(b: *std.Build) !void {
	const target = b.standardTargetOptions(.{});
	const optimize = b.standardOptimizeOption(.{});

	const exe_mod = b.createModule(.{
		.target = target,
		.optimize = optimize,
	});

	const exe = b.addExecutable(.{
		.name = "CruelerThanDAT",
		.root_module = exe_mod,
	});

	const stb = b.dependency("stb", .{
		.optimize = optimize,
		.target = target,
	});

	const fbx_dir = try getEnvVarOwned(b.allocator, "FBXSDK_DIR");
	const fbx_inc_dir = try allocPrint(b.allocator, "{s}/include/", .{fbx_dir});
	const fbx_lib_dir = try allocPrint(b.allocator, "{s}/lib/x64/debug/", .{fbx_dir});

	const dx_dir = try getEnvVarOwned(b.allocator, "DXSDK_DIR");
	const dx_inc_dir = try allocPrint(b.allocator, "{s}/Include/", .{dx_dir});
	const dx_lib_dir = try allocPrint(b.allocator, "{s}/Lib/x64/", .{dx_dir});

	exe.addIncludePath(b.path("CruelerThanDAT/"));
	exe.addIncludePath(b.path("CruelerThanDAT/inc/"));

	exe.addIncludePath(stb.path(""));

	exe.addIncludePath(b.path("depends/curl/include/curl/"));
	exe.addIncludePath(b.path("depends/SDL3/include/"));
	exe.addIncludePath(b.path("depends/imgui/inc/"));
	exe.addIncludePath(b.path("depends/imstb/"));
	exe.addIncludePath(b.path("depends/json/"));
	exe.addIncludePath(b.path("depends/GLAD/include/"));
	exe.addIncludePath(b.path("depends/GLAD/src/"));
	exe.addIncludePath(b.path("depends/gli/"));
	exe.addIncludePath(.{.cwd_relative = fbx_inc_dir});
	exe.addIncludePath(.{.cwd_relative = dx_inc_dir});
	
	exe.addLibraryPath(.{.cwd_relative = fbx_lib_dir});
	exe.addLibraryPath(.{.cwd_relative = dx_lib_dir});
	exe.addLibraryPath(b.path("depends/SDL3/lib/x64/"));
	exe.addLibraryPath(b.path("build-curl/Debug/lib/"));
	
	exe.linkSystemLibrary("SDL3");
	exe.linkSystemLibrary("libfbxsdk");
	exe.linkSystemLibrary("libcurl-d");

	exe.linkSystemLibrary("d3dx9d");
	exe.linkSystemLibrary("d3d9");

	exe.linkSystemLibrary("urlmon");
	exe.linkSystemLibrary("wldap32");
	exe.linkSystemLibrary("advapi32");
	exe.linkSystemLibrary("crypt32");
	exe.linkSystemLibrary("normaliz");
	exe.linkSystemLibrary("ws2_32");
	exe.linkSystemLibrary("comdlg32");
	exe.linkSystemLibrary("ole32");

	exe.linkLibC();
	exe.linkLibCpp();

	exe.addCSourceFiles(.{ .language = .cpp,
		.files = &.{
			"CruelerThanDAT/CruelerThanDAT.cpp",
			"CruelerThanDAT/src/Wwise/wwise.cpp",
			"CruelerThanDAT/src/CRC32.cpp",
			"CruelerThanDAT/src/FileNodes.cpp",
			"CruelerThanDAT/src/FileUtils.cpp",
			"CruelerThanDAT/src/glad.cpp",
			"CruelerThanDAT/src/TextEditor.cpp",
			"CruelerThanDAT/src/themeLoader.cpp",
			"CruelerThanDAT/src/tinyxml2.cpp",

			"depends/imgui/src/imgui_draw.cpp",
			"depends/imgui/src/imgui_impl_opengl3.cpp",
			"depends/imgui/src/imgui_impl_sdl3.cpp",
			"depends/imgui/src/imgui_impl_sdlrenderer3.cpp",
			"depends/imgui/src/imgui_tables.cpp",
			"depends/imgui/src/imgui_widgets.cpp",
			"depends/imgui/src/imgui.cpp",
		},
		.flags = &.{
			"-DmLefttChild=mLeftChild",
			"-DUNICODE",
			"-std=c++20",
			"-Wno-all",
			"-Wno-error",
		},
	});

	b.installArtifact(exe);

	var targets = try std.ArrayList(*std.Build.Step.Compile).initCapacity(b.allocator, 1);
	try targets.append(exe);
	zcc.createStep(b, "zcc", try targets.toOwnedSlice());

	const run_cmd = b.addRunArtifact(exe);

	run_cmd.step.dependOn(b.getInstallStep());

	if (b.args) |args| {
		run_cmd.addArgs(args);
	}

	const run_step = b.step("run", "Run the app");
	run_step.dependOn(&run_cmd.step);
}
