/*
 * An application for applying a Gaussian blur.
 * It uses a 3x3 stencil with constant weights.
 */

#include "Halide.h"

namespace {

using namespace Halide;

// Size of blur for gradients.
const int blockSize = 3;
int imgSize = 64-blockSize+1;

class GaussianBlur : public Halide::Generator<GaussianBlur> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");
        Var xo("xo"), yo("yo"), xi("xi"), yi("yi");

        // Create a reduction domain of the correct bounds.
        RDom win(0, blockSize, 0, blockSize);

        Func hw_input;
        hw_input(x, y) = cast<int16_t>(input(x, y));

        // create the gaussian kernel
        Func kernel_f;
        float sigma = 1.5f;
        kernel_f(x) = exp(-x*x/(2*sigma*sigma)) / (sqrtf(2*M_PI)*sigma);

        // create a normalized set of 8bit weights
        Func kernel;
        Expr sum_kernel[blockSize];
        for (int i=0; i<blockSize; ++i) {
          if (i==0) {
            sum_kernel[i] = kernel_f(i-blockSize/2);
          } else {
            sum_kernel[i] = kernel_f(i-blockSize/2) + sum_kernel[i-1];
          }
        }
        kernel(x) = cast<uint16_t>(kernel_f(x) * 256 / sum_kernel[blockSize-1]);

        // Use a 2D filter to blur the input
        Func blur_unnormalized, blur;
        blur_unnormalized(x, y) = 0;
        blur_unnormalized(x, y) += cast<uint16_t>( kernel(win.x) * kernel(win.y) * hw_input(x+win.x, y+win.y) );
        blur(x, y) = blur_unnormalized(x, y) / 256 / 256;

        Func hw_output;
        hw_output(x, y) = cast<uint8_t>( blur(x, y) );
        output(x, y) = hw_output(x, y);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {

          hw_output.bound(x, 0, imgSize);
          hw_output.bound(y, 0, imgSize);
          output.bound(x, 0, imgSize);
          output.bound(y, 0, imgSize);
          blur_unnormalized.bound(x, 0, imgSize);
          blur_unnormalized.bound(y, 0, imgSize);

          //hw_input.compute_root();
          //kernel.compute_root();
          hw_output.compute_root();

          hw_output
            //            .compute_at(output, xo)
            .tile(x, y, xo, yo, xi, yi, imgSize, imgSize)
            .hw_accelerate(xi, xo);

          blur_unnormalized.update()
            .unroll(win.x, blockSize)
            .unroll(win.y, blockSize);

          blur_unnormalized.linebuffer();

          //hw_output.accelerate({hw_input}, xi, xo);
          hw_input.compute_at(hw_output, xi).store_at(hw_output, xo);
          hw_input.stream_to_accelerator();

        } else {    // schedule to CPU

          /*output.tile(x, y, xo, yo, xi, yi, imgSize, imgSize)
            .vectorize(xi, 8)
            .fuse(xo, yo, xo)
            .parallel(xo);*/

        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(GaussianBlur, gaussian)

