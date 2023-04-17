add_requires("assimp",{alias = "assimp"})
target("Beacon")
    add_files("**.cpp")
    remove_files("Test/*.cpp")
    remove_files("CrossMain.cpp")
    add_syslinks("User32", "kernel32", "Gdi32", "Shell32", "DXGI", "D3D12", "D3DCompiler","dxguid")
    add_links("WinPixEventRuntime")
    add_linkdirs("$(projectdir)/lib")
    add_includedirs("./")
    set_pcxxheader("./stdafx.h")
    add_defines("UNICODE")
    add_deps("3rdparty")
    add_packages("assimp")
    if is_mode("debug") then
        add_defines("USE_PIX")
        
    end

target("CrossBeacon")
    add_files("**.cpp")
    remove_files("Test/*.cpp")
    remove_files("main.cpp")
    add_syslinks("User32", "kernel32", "Gdi32", "Shell32", "DXGI", "D3D12", "D3DCompiler","dxguid")
    add_links("WinPixEventRuntime")
    add_linkdirs("$(projectdir)/lib")
    add_includedirs("./")
    set_pcxxheader("./stdafx.h")
    add_defines("UNICODE")
    add_deps("3rdparty")
    add_packages("assimp")
    if is_mode("debug") then
        add_defines("USE_PIX")
        
    end

target("UnitTest")
    add_files("**.cpp")
    remove_files("main.cpp")
    add_syslinks("User32", "kernel32", "Gdi32", "Shell32", "DXGI", "D3D12", "D3DCompiler","dxguid")
    add_links("WinPixEventRuntime")
    add_linkdirs("$(projectdir)/lib")
    add_includedirs("./")
    set_pcxxheader("./stdafx.h")
    add_defines("UNICODE")
    add_deps("3rdparty")
    add_packages("assimp")
    if is_mode("debug") then
        add_defines("USE_PIX")
        
    end
