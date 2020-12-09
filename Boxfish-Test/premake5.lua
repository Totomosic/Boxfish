project "Boxfish-Test"
    location ""
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    
    targetdir ("../bin/" .. BoxfishOutputDir .. "/Boxfish-Test")
    objdir ("../bin-int/" .. BoxfishOutputDir .. "/Boxfish-Test")
    
    files
    {
        "src/**.h",
        "src/**.cpp",
        "vendor/catch/include/**.hpp",
        "vendor/catch/include/**.cpp"
    }
    
    includedirs
    {
        "src",
        "../%{BoxfishIncludeDirs.spdlog}",
        "../%{BoxfishIncludeDirs.Boxfish}",
        "../%{BoxfishIncludeDirs.Catch}"
    }

    links
    {
        "Boxfish-Lib"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "BOX_PLATFORM_WINDOWS",
            "BOX_BUILD_STATIC",
            "_CRT_SECURE_NO_WARNINGS",
            "NOMINMAX"
        }

    filter "system:linux"
        systemversion "latest"

        removeconfigurations { "DistShared", "ReleaseShared" }

        defines
        {
            "BOX_PLATFORM_LINUX",
            "BOX_BUILD_STATIC"
        }

        links
        {
            "pthread"
        }

    filter "system:macosx"
        systemversion "latest"

        defines
        {
            "BOX_PLATFORM_MAC",
            "BOX_BUILD_STATIC"
        }

    filter "configurations:Debug"
        defines "BOX_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "BOX_RELEASE"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        defines "BOX_DIST"
        runtime "Release"
        optimize "on"