#include "Halide.h"

namespace {

using namespace Halide;
using namespace Halide::ConciseCasts;

class ConvolutionKernel : public Halide::Generator<ConvolutionKernel> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func kernel("kernel");
        Func conv("conv");
        RDom r(0, 2,
               0, 1);

        kernel(x,y) = 0;
        kernel(0,0) = 1;        kernel(1,0) = 3;

        conv(x, y) = u16(0);

        Func input_copy, hw_input("hw_input");
        input_copy(x, y) = u16(input(x, y));
        hw_input(x, y) = input_copy(x, y);
        conv(x, y)  += u16(kernel(r.x, r.y)) * hw_input(x + r.x, y + r.y);

        Func hw_output("hw_output");
        hw_output(x, y) = u8(conv(x, y));
        output(x, y) = hw_output(x,y);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
          Var xi,yi, xo,yo;

          hw_input.compute_root();
          hw_output.compute_root();

          output.bound(x, 0, 63);
          output.bound(y, 0, 64);
          conv.bound(x, 0, 63);
          conv.bound(y, 0, 64);

          hw_output.tile(x,y, xo,yo, xi,yi, 64-1, 64)
            .hw_accelerate(xi, xo);

          conv.update()
            .unroll(r.x, 2)
            .unroll(r.y, 1);

          conv.linebuffer();

          hw_input.compute_at(hw_output, xi).store_at(hw_output, xo);
          hw_input.stream_to_accelerator();

        } else if (get_target().has_feature(Target::Clockwork)) {
          Var xi,yi, xo,yo;

          output.bound(x, 0, 63);
          output.bound(y, 0, 64);

          hw_output.compute_root();
          hw_output
              .tile(x,y, xo,yo, xi,yi, 64-1, 64)
              .hw_accelerate(xi, xo);

          conv.update()
            .unroll(r.x, 2)
            .unroll(r.y, 1);

          conv.compute_at(hw_output, xo);

          kernel.compute_at(conv, x);

          hw_input.compute_at(hw_output, xo).store_at(hw_output, xo);
          hw_input.stream_to_accelerator();

          input_copy.compute_root();
          
        } else {  // schedule to CPU
          kernel.compute_root();
          conv.compute_root();
          conv.update()
            .unroll(r.x, 2)
            .unroll(r.y, 1);
        }

    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ConvolutionKernel, conv_1_2)
