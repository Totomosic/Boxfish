outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Project Directories
BoxfishLibDir = "Boxfish-Lib/"
BoxfishCliDir = "Boxfish-Cli/"

-- Include directories relative to solution directory
IncludeDirs = {}
IncludeDirs["spdlog"] =     BoxfishLibDir .. "vendor/spdlog/include/"
IncludeDirs["Boxfish"] =    BoxfishLibDir .. "src/"

-- Library directories relative to solution directory
LibraryDirs = {}

-- Links
Links = {}