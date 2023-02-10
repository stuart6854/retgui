project "ExampleApp"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetname "app"
    targetdir("../bin/" .. outputdir)
    objdir("../bin/" .. outputdir .. "/obj/%{prj.name}")
    debugdir ("../bin/" .. outputdir)
    staticruntime "Off"

    flags
    {
        "MultiProcessorCompile",
        "FatalCompileWarnings",
    }
    warnings "High"
    externalwarnings "Off"
    externalanglebrackets "On"

    linkoptions { conan_exelinkflags }

    files { "src/**.hpp", "src/**.h", "src/**.cpp" }
    includedirs { "src", "../backends" }

    use_retgui()

    filter "configurations:Debug"
        targetsuffix "-debug"
        defines { "_DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"