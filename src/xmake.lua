add_requires("assimp",{alias = "assimp"})

target("Renderer")
    add_files("**.cpp|Test/*.cpp")
    set_kind("static")
    add_syslinks("User32", "kernel32", "Gdi32", "Shell32", "DXGI", "D3D12", "D3DCompiler","dxguid")
    add_linkdirs("$(projectdir)/lib")
    add_links("WinPixEventRuntime")
    add_includedirs("./",{public = true})
    set_pcxxheader("./stdafx.h")
    add_defines("UNICODE")
    add_deps("3rdparty")
    add_packages("assimp")
    if is_mode("debug") then
        add_defines("USE_PIX")
    end


target("Beacon")
    set_kind("binary")
    add_files("Test/SingletonBeacon.cpp")
    add_deps("Renderer")
    add_defines("UNICODE")

target("CrossBeacon")
    set_kind("binary")
    add_files("Test/FramePipelineBeacon.cpp")
    add_deps("Renderer")
    add_defines("UNICODE")

target("CrossDevice")
    set_kind("binary")
    add_files("Test/CrossDeviceTest.cpp")
    add_deps("Renderer")
    add_syslinks("User32", "kernel32", "Gdi32", "Shell32", "DXGI", "D3D12", "D3DCompiler","dxguid")
    add_defines("UNICODE")
