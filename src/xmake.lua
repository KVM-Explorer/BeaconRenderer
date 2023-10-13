add_requires("assimp",{alias = "assimp"})
add_requires("yaml-cpp")

target("Renderer")
    add_files("**.cpp|Test/*.cpp|Beacon.cpp|CrossBeacon.cpp|StageBeacon.cpp")
    set_kind("static")
    add_syslinks("User32", "kernel32", "Gdi32", "Shell32", "DXGI", "D3D12", "D3DCompiler","dxguid")
    add_linkdirs("$(projectdir)/lib")
    add_links("WinPixEventRuntime")
    add_includedirs("./",{public = true})
    add_packages("assimp")
    add_packages("yaml-cpp")
    set_pcxxheader("./stdafx.h")
    add_defines("UNICODE")
    add_deps("3rdparty")
    if is_mode("debug") then
        add_defines("USE_PIX")
    end


target("Beacon")
    set_kind("binary")
    add_files("Test/SingletonBeacon.cpp","Beacon.cpp")
    add_deps("Renderer")
    add_packages("yaml-cpp")
    add_defines("UNICODE")

target("CrossBeacon")
    set_kind("binary")
    add_files("Test/CrossPipelineBeacon.cpp","CrossBeacon.cpp")
    add_deps("Renderer")
    add_packages("yaml-cpp")
    add_defines("UNICODE")

target("StageBeacon")
    set_kind("binary")
    add_files("Test/StagePipelineBeacon.cpp","StageBeacon.cpp")
    add_deps("Renderer")
    add_packages("yaml-cpp")
    add_defines("UNICODE")

target("AFRBeacon")
    set_kind("binary")
    add_files("Test/AFRBeacon.cpp","AFRBeacon.cpp")
    add_deps("Renderer")
    add_packages("yaml-cpp")
    add_defines("UNICODE")

target("AsyncTest")
    set_kind("binary")
    add_files("Test/AsyncCoroutineTest.cpp")

target("PPLTest")
    set_kind("binary")
    add_files("Test/PPLTest.cpp")

target("ConfigTest")
    set_kind("binary")
    add_files("Test/ConfigTest.cpp")
    add_packages("yaml-cpp")