function use_retgui()
    links { 
        "$(VULKAN_SDK)/Lib/vulkan-1.lib",
        "RetGui" 
    }
    externalincludedirs { "$(VULKAN_SDK)/Include", "%{wks.location}/RetGui/include" }
end

project "RetGui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    targetname "base"
    targetdir("../bin/" .. outputdir)
    objdir("../bin/" .. outputdir .. "/obj/%{prj.name}")
    staticruntime "Off"

    flags
    {
        "MultiProcessorCompile",
        "FatalCompileWarnings",
    }
    warnings "High"
    externalwarnings "Off"
    externalanglebrackets "On"

    files { "src/**.cpp", "include/**.hpp" }
    includedirs { "include" }

    externalincludedirs { "$(VULKAN_SDK)/Include" }

    filter "configurations:Debug"
        targetsuffix "-debug"
        defines { "_DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG", }
        optimize "On"