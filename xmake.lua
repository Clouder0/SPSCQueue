set_toolchains("gcc")
target("main")
  set_kind("binary")
  add_files("src/*.cpp")
