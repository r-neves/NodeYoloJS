{
 "variables":{
  "with_openmp%":"<!(node ./util/has_lib.js openmp)",
  "with_cuda%":"<!(node ./util/has_lib.js cuda)"
 },
 "targets":[
  {
   "target_name":"nodeyolo",
   "sources":[
    "src/module.cpp"
   ],
   "libraries":[
    "<(module_root_dir)/yolo/libyolo.a",
    "-lopencv_core",
    "-lopencv_highgui",
    "-lopencv_videoio",
    "-lopencv_video"
   ],
   "defines":[
    "OPENCV",
    "NAPI_DISABLE_CPP_EXCEPTIONS"
   ],
   "include_dirs":[
    "<(module_root_dir)/yolo/src",
    "<(module_root_dir)/darknet/src",
    "<(module_root_dir)/darknet/include",
    "<(module_root_dir)/stack/",
    "<(module_root_dir)/common/include"
   ],
   "cflags":[
    "-Wall",
    "-Wfatal-errors",
    "-fPIC",
    "-Ofast"
   ],
   "xcode_settings":{
    "GCC_ENABLE_CPP_EXCEPTIONS":"YES",
    "CLANG_CXX_LIBRARY":"libc++",
    "MACOSX_DEPLOYMENT_TARGET":"10.14"
   },
   "msvs_settings":{
    "VCCLCompilerTool":{
	"ExceptionHandling":1
    }
   },
   "conditions":[
   [
   	'with_openmp=="true"',
   	{
   	 "cflags":[
   	  "-fopenmp"
   	 ],
   	 "libraries":[
   	  "-lomp",
   	 ]
   	}
    ],
    [
	'with_cuda=="true"',
	{
	 "defines":[
	  "GPU"
	 ],
	 "libraries":[
	  "-L/usr/local/cuda/lib64",
	  "-L/usr/lib/x86_64-linux-gnu/",
	  "-lcuda",
	  "-lcudart",
	  "-lcublas",
	  "-lcurand",
	  "-lcudnn"
	 ],
	 "include_dirs":[
	  "/usr/local/cuda/include"
	 ]
	}
    ]
   ]
  }
 ]
}
