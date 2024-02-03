workspace "packet_master"
    configurations {"Debug", "Release"}

startproject "Tests"
project "Tests"
    kind "ConsoleApp"
    language "C"
    targetdir ("bin/%{cfg.buildcfg}/%{prj.name}")
	objdir ("bin/obj/%{cfg.buildcfg}/%{prj.name}")

    files {
        "tests/**.h",
        "tests/**.c",
        "src/**.h",
        "src/**.c"
    }

    includedirs {
        "src/"
    }

    filter "system:windows"
		systemversion "latest"

    filter "configurations:Debug"
        defines {"DEBUG"}
        symbols "On"
    
    filter "configurations:Release"
        defines {"NDEBUG"}
        optimize "On"