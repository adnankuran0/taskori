--premake5.lua

workspace "taskori"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "Sandbox"

    filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Sandbox"
	include "Core/Build-Sandbox.lua"
group "Tests"
	include "Core/Build-Tests.lua"
group ""