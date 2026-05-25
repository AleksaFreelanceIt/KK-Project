add_llvm_library( LLVMLicmPass MODULE
    LICMPass.cpp

    DEPENDS
    intrinsics_gen
    PLUGIN_TOOL
    opt
    )
