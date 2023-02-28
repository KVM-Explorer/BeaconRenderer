
target("BeaconRenderer")
    add_files("**.cpp")
    add_syslinks("User32", "kernel32", "Gdi32", "Shell32", "DXGI", "D3D12", "D3DCompiler","dxguid")
    add_includedirs("./")
    set_pcxxheader("./stdafx.h")
    add_deps("3rdparty")