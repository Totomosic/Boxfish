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

include ("Paths.lua")

include (BoxfishLibDir)
include (BoxfishCliDir)
include (BoxfishTestDir)
include (BoxfishBookDir)

filter "system:windows"
    include ("SwigConfigWindows.lua")

filter "system:linux"
    include ("SwigConfigLinux.lua")

include ("Boxfish-Swig")
