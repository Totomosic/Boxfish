workspace "Boxfish"
    architecture "x64"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

    flags
    {
        "MultiProcessorCompile"
    }

    filter "system:linux"
        configurations
        {
            "DistShared",
            "ReleaseShared",
        }

include ("Paths.lua")

include (BoxfishLibDir)
include (BoxfishCliDir)
include (BoxfishTestDir)
include (BoxfishBookDir)

if os.target() == "windows" then
    -- Windows
    include ("SwigConfigWindows.lua")
else
    -- Linux
    include ("SwigConfigLinux.lua")
end

include ("Boxfish-Swig")
