# Copyright (c) 2026 Infernux contributors. Apache-2.0.
# Used by the engine wheel build; no .inxpkg archive or project Package tree.
function(infernux_compiler_output target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Missing compiler binding target: ${target}")
    endif()
    set(INFERNUX_COMPILER_OUTPUT_DIR
        "${PROJECT_BINARY_DIR}/wheel/infernux/_compiler/taichi/_vendor/taichi/_lib/core"
        CACHE PATH "Native compiler destination inside the engine wheel staging tree")
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INFERNUX_COMPILER_OUTPUT_DIR}/$<0:>"
        RUNTIME_OUTPUT_DIRECTORY "${INFERNUX_COMPILER_OUTPUT_DIR}/$<0:>"
        ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/$<0:>"
        PDB_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/symbols/$<0:>")
    install(TARGETS ${target}
        RUNTIME DESTINATION infernux/_compiler/taichi/_vendor/taichi/_lib/core COMPONENT infernux_compiler
        LIBRARY DESTINATION infernux/_compiler/taichi/_vendor/taichi/_lib/core COMPONENT infernux_compiler)
    # The actual private frontend closure, not a public Taichi runtime tree.
    set(frontend_files
        __init__.py
        _lib/__init__.py
        _lib/utils.py
        _logging.py
        lang/__init__.py
        lang/_ndrange.py
        lang/_wrap_inspect.py
        lang/any_array.py
        lang/ast/__init__.py
        lang/ast/ast_transformer.py
        lang/ast/ast_transformer_utils.py
        lang/ast/symbol_resolver.py
        lang/ast/transform.py
        lang/common_ops.py
        lang/enums.py
        lang/exception.py
        lang/expr.py
        lang/impl.py
        lang/kernel_arguments.py
        lang/kernel_impl.py
        lang/matrix.py
        lang/matrix_ops.py
        lang/matrix_ops_utils.py
        lang/ops.py
        lang/struct.py
        lang/util.py
        types/__init__.py
        types/annotations.py
        types/buffer_type.py
        types/compound_types.py
        types/primitive_types.py
        types/utils.py)
    foreach(member IN LISTS frontend_files)
        get_filename_component(folder "${member}" DIRECTORY)
        install(FILES "${PROJECT_SOURCE_DIR}/python/taichi/${member}"
            DESTINATION "infernux/_compiler/taichi/_vendor/taichi/${folder}"
            COMPONENT infernux_compiler)
    endforeach()
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/NOTICE"
        DESTINATION infernux/_compiler/licenses/taichi COMPONENT infernux_compiler)
endfunction()
