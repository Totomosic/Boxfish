project "Boxfish-Swig"
    location ""
    kind "SharedLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("../bin/" .. BoxfishOutputDir .. "/Boxfish-Swig")
    objdir ("../bin-int/" .. BoxfishOutputDir .. "/Boxfish-Swig")
    targetname ("_Boxfish")

    prebuildcommands
    {
        "\"" .. PYTHON_EXECUTABLE .. "\" generate_swig.py --swig \"" .. SWIG_EXECUTABLE .. "\" " .. "\"../bin/" .. BoxfishOutputDir .. "/Boxfish-Swig\""
    }

    files
    {
        "Boxfish_wrapper.cpp"
    }

    includedirs
    {
        "../%{BoxfishIncludeDirs.spdlog}",
        "../%{BoxfishIncludeDirs.Boxfish}",
        PYTHON_INCLUDE_DIR,
    }

    links
    {
        "Boxfish-Lib",
        PYTHON_LIB_FILE,
    }

    filter "system:windows"
        systemversion "latest"

        targetextension (".pyd")

        defines
        {
            "BOX_PLATFORM_WINDOWS",
            "BOX_BUILD_STATIC",
            "_CRT_SECURE_NO_WARNINGS",
            "NOMINMAX"
        }

    filter "system:linux"
        systemversion "latest"

        targetextension (".so")

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