#include "taichi/codegen/spirv/kernel_compiler.h"

#include "taichi/ir/analysis.h"
#include "taichi/ir/visitors.h"
#include "taichi/codegen/spirv/spirv_codegen.h"
#include "taichi/codegen/spirv/compiled_kernel_data.h"

namespace taichi::lang {
namespace spirv {

namespace {
class LocalVjpExecutableCheck : public BasicStmtVisitor {
 public:
  using BasicStmtVisitor::visit;
  LocalVjpExecutableCheck() {
    allow_undefined_visitor = true;
    invoke_default_visitor = true;
  }
  void visit(AtomicOpStmt *stmt) override {
    TI_ERROR("Local VJP requires unique gradient writes; residual atomics are unsupported\n{}",
             stmt->get_tb());
  }
  void visit(Stmt *stmt) override {
    if (stmt->is<GlobalTemporaryStmt>() || stmt->is<AdStackAllocaStmt>() ||
        stmt->is<AdStackLoadTopStmt>() || stmt->is<AdStackLoadTopAdjStmt>() ||
        stmt->is<AdStackPopStmt>() || stmt->is<AdStackPushStmt>() ||
        stmt->is<AdStackAccAdjointStmt>())
      TI_ERROR("Local VJP cannot require a replay stack or global temporary resource\n{}",
               stmt->get_tb());
  }
};
}  // namespace

KernelCompiler::IRNodePtr KernelCompiler::compile(
    const CompileConfig &compile_config,
    const Kernel &kernel_def) const {
  auto ir = irpass::analysis::clone(kernel_def.ir.get());
  irpass::compile_to_executable(ir.get(), compile_config, &kernel_def,
                                kernel_def.autodiff_mode,
                                /*ad_use_stack=*/false, compile_config.print_ir,
                                /*lower_global_access=*/true,
                                /*make_thread_local=*/false);
  if (kernel_def.autodiff_mode == AutodiffMode::kReverse) {
    if (ir->as<Block>()->size() != 1 ||
        !ir->as<Block>()->statements[0]->is<OffloadedStmt>() ||
        ir->as<Block>()->statements[0]->as<OffloadedStmt>()->task_type !=
            OffloadedTaskType::range_for)
      TI_ERROR("Local VJP must lower to one independent range-for task");
    LocalVjpExecutableCheck check;
    ir->accept(&check);
  }
  return ir;
}

KernelCompiler::CKDPtr KernelCompiler::compile(
    const CompileConfig &compile_config,
    const DeviceCapabilityConfig &device_caps,
    const Kernel &kernel_def,
    IRNode &chi_ir) const {
  if (device_caps.get(DeviceCapability::spirv_has_physical_storage_buffer))
    TI_ERROR("The compiler-only engine buffer ABI requires logical 32-bit byte offsets");
  TI_TRACE("VK codegen for Taichi kernel={}", kernel_def.name);
  KernelCodegen::Params params;
  params.ti_kernel_name = kernel_def.name;
  params.kernel = &kernel_def;
  params.ir_root = &chi_ir;
  params.arch = compile_config.arch;
  params.caps = device_caps;
  params.enable_spv_opt = compile_config.external_optimization_level > 0;
  params.fast_math = compile_config.fast_math;
  spirv::KernelCodegen codegen(params);
  spirv::CompiledKernelData::InternalData internal_data;
  internal_data.metadata.required_capabilities = device_caps;
  codegen.run(internal_data.metadata.kernel_attribs,
              internal_data.src.spirv_src);
  return std::make_unique<CompiledKernelData>(std::move(internal_data));
}

}  // namespace spirv
}  // namespace taichi::lang
