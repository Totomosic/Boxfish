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
    if os.isfile("SwigConfigWindows.lua") then
        include ("SwigConfigWindows.lua")
    end
else
    -- Linux
    if os.isfile("SwigConfigLinux.lua") then
        include ("SwigConfigLinux.lua")
    end
end

include ("Boxfish-Swig")
