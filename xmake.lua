add_rules("mode.debug", "mode.release")

add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

set_languages("c++latest")

set_policy("build.c++.modules", true)
set_policy("build.c++.modules.std", true)

add_requires("spdlog")
add_requires("stdexec")

target("gomoku")
    set_kind("binary")
    set_targetdir("bin")
    -- add_includedirs("include")
    add_files("main.cpp", "src/*.cpp")
    add_packages("spdlog", "stdexec")

    on_load(function (target)
        import("lib.detect.find_tool")
        import("core.base.semver")

        local clang = find_tool("clang", {version = true})
        if clang and clang.version and semver.compare(clang.version, "20.0") >= 0 then
            target:set("toolchains", "llvm")
            target:add("cxxflags", "-stdlib=libc++")
            target:set("runtimes", "c++_shared")
        elseif target:has_tool("cxx", "cl") then
            target:add("cxxflags", "/utf-8", "/EHsc")
        elseif target:has_tool("cxx", "clang", "clang++") then
            target:add("cxxflags", "-stdlib=libc++")
            target:set("runtimes", "c++_shared")
        end
    end)
