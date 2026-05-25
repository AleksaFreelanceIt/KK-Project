add_llvm_library( LLVMOurLoopUnrollingPass MODULE
	OurLoopUnrollingPass.cpp

	DEPENDS
	intrinsics_gen
	PLUGIN_TOOL
	opt
	)
