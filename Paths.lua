BoxfishOutputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Project Directories
BoxfishLibDir = "Boxfish-Lib/"
BoxfishCliDir = "Boxfish-Cli/"
BoxfishTestDir = "Boxfish-Test/"
BoxfishBookDir = "Boxfish-Book/"

-- Include directories relative to solution directory
BoxfishIncludeDirs = {}
BoxfishIncludeDirs["spdlog"] =     BoxfishLibDir .. "vendor/spdlog/include/"
BoxfishIncludeDirs["Boxfish"] =    BoxfishLibDir .. "src/"
BoxfishIncludeDirs["Catch"] =      BoxfishTestDir .. "vendor/catch/include/"

-- Library directories relative to solution directory
LibraryDirs = {}

-- Links
Links = {}