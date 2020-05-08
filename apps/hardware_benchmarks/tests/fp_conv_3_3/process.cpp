#include <cstdio>

#include "fp_conv_3_3.h"

#include "hardware_process_helper.h"
#include "coreir_interpret.h"
#include "halide_image_io.h"

using namespace Halide::Tools;
using namespace Halide::Runtime;

int main(int argc, char **argv) {

  OneInOneOut_ProcessController<uint8_t> processor("fp_conv_3_3",
                                            {
                                              {"cpu",
                                                  [&]() { fp_conv_3_3(processor.input, processor.output); }
                                              },
                                              {"rewrite",
                                                  [&]() { run_coreir_rewrite_on_interpreter<>("bin/design_top.json", "bin/ubuffers.json", processor.input, processor.output,
                                                                                      "self.in_arg_0_0_0", "self.out_0_0",
                                                                                      /*has_float_input=*/true, /*has_float_output=*/true); }
                                              },
                                              {"coreir",
                                                  [&]() { run_coreir_on_interpreter<>("bin/design_top.json", processor.input, processor.output,
                                                                                      "self.in_arg_0_0_0", "self.out_0_0",
                                                                                      /*has_float_input=*/true, /*has_float_output=*/true); }
                                              }

                                            });

  processor.input = Buffer<uint8_t>(64, 64);
  processor.output = Buffer<uint8_t>(62, 62);

  return processor.process_command(argc, argv);

}
