{
 "targets":[
  {
   "target_name":"nodeyolojs",
   "sources":[
    "src/module.cpp"
   ],
   "libraries":[
    "<(module_root_dir)/yolo/libyolo.a"
   ],
   "defines":[
    "NAPI_DISABLE_CPP_EXCEPTIONS"
   ],
   "include_dirs":[
    "<(module_root_dir)/yolo/src"
   ],
   "cflags":[
    "-Wall",
    "-Wfatal-errors",
    "-fPIC"
   ]
  }
 ]
}