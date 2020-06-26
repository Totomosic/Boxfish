BoxfishOutputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Project Directories
BoxfishLibDir = "Boxfish-Lib/"
BoxfishCliDir = "Boxfish-Cli/"

-- Include directories relative to solution directory
BoxfishIncludeDirs = {}
BoxfishIncludeDirs["spdlog"] =     BoxfishLibDir .. "vendor/spdlog/include/"
BoxfishIncludeDirs["Boxfish"] =    BoxfishLibDir .. "src/"

-- Library directories relative to solution directory
LibraryDirs = {}

-- Links
Links = {}