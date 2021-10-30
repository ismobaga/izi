require "vscode"

-- delete a file if the clean action is running
newaction {
   trigger = "clean" ,
   description = "remove all bin and int files",
   execute = function (  )
      print("Removing  bin...")
      os.rmdir("./bin")
      print("Removing  interme...")
      os.rmdir("./obj")
      print("Removing  project files...")
      os.rmdir("./.vscode")
      os.remove("./**.code-workspace")
      os.remove("./Makefile")
      -- os.remove("**make)
      print("Done")
   end


}
workspace "Izi"
   architecture "x64"
   configurations { "Debug", "Release" }


project "izi"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    toolset ("clang")
    staticruntime "on"
    location "../"
    files {"**.h", "**.cpp"}

    filter { "configurations:Debug" }
       defines { "DEBUG" }
       symbols "On"
       runtime "Debug"
 
    filter { "configurations:Release" }
       defines { "NDEBUG" }
       optimize "On"
   -- filter { "system:linux", "action:gmake" }
      -- buildoptions { "`wx-config --cxxflags`", "-ansi", "-pedantic" }
