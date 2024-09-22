set_toolchains("gcc")
set_languages("c99", "c++20")
set_warnings("all")

if is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
    set_strip("all")
    add_cxflags("-fomit-frame-pointer")
    add_mxflags("-fomit-frame-pointer") 
end

target("main")
  set_kind("binary")
  add_files("src/*.cpp")
  add_includedirs("include")
