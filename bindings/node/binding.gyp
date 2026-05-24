{
  "targets": [
    {
      "target_name": "g16addon",
      "sources": [ "src/addon.cc" ],
      "include_dirs": [ "<!(node -p \"require('node-addon-api').include\")", "../../include", "../c" ],
      "cflags_cc": [ "-std=c++20" ],
      "libraries": [ "<(module_root_dir)/../../build/libg16_cabi.a" ],
      "defines": [ "NAPI_CPP_EXCEPTIONS" ]
    }
  ]
}
