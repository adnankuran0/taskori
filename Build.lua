--premake5.lua

workspace "taskori"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "Sandbox"

    filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

include "Sandbox/Build-Sandbox.lua"
include "Tests/Build-Tests.lua"