const std = @import("std");
const Target = @import("std").Target;
const CrossTarget = @import("std").zig.CrossTarget;
const Feature = @import("std").Target.Cpu.Feature;

pub fn build(b: *std.build.Builder) void {
    const features = Target.x86.Feature;

    var disabled_features = Feature.Set.empty;
    var enabled_features = Feature.Set.empty;

    disabled_features.addFeature(@enumToInt(features.mmx));
    disabled_features.addFeature(@enumToInt(features.sse));
    disabled_features.addFeature(@enumToInt(features.sse2));
    disabled_features.addFeature(@enumToInt(features.avx));
    disabled_features.addFeature(@enumToInt(features.avx2));
    enabled_features.addFeature(@enumToInt(features.soft_float));

    const target = CrossTarget{ .cpu_arch = Target.Cpu.Arch.x86_64, .os_tag = Target.Os.Tag.freestanding, .abi = Target.Abi.none, .cpu_features_sub = disabled_features, .cpu_features_add = enabled_features };

    const mode = b.standardReleaseOptions();

    const init = b.addExecutable("init", "src/init.zig");
    init.setTarget(target);
    init.setBuildMode(mode);
    init.install();

    const sh = b.addExecutable("sh", "src/sh.zig");
    sh.setTarget(target);
    sh.setBuildMode(mode);
    sh.install();

    const helloworld = b.addExecutable("helloworld", "src/helloworld.zig");
    helloworld.setTarget(target);
    helloworld.setBuildMode(mode);
    helloworld.install();

    const ls = b.addExecutable("ls", "src/ls.zig");
    ls.setTarget(target);
    ls.setBuildMode(mode);
    ls.install();
}
