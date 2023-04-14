add_requires("assimp",{alias = "assimp"})
target("BeaconRenderer")
    add_files("**.cpp")
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
