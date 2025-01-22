const std = @import("std");

pub fn build(b: *std.Build) void {
    const exe = b.addExecutable(.{
        .name = "fakerm",
        .target = b.host,
    });
    exe.linkLibCpp();
    exe.addCSourceFiles(.{
        .files = &[_][]const u8 {
            "src/main.cpp",
            "src/fake_shells.cpp",
            "src/utils.cpp",
        },
        .flags = &[_][]const u8 {
            "-Iinclude/"
        }
    });
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    const run_step = b.step("run", "Run fakerm");
    run_step.dependOn(&run_cmd.step);
}

