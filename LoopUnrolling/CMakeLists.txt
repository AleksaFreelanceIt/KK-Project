add_llvm_library( LLVMLoopUnrollingPass MODULE
    LoopUnrollingPass.cpp

	DEPENDS
	intrinsics_gen
	PLUGIN_TOOL
	opt
	)
