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
