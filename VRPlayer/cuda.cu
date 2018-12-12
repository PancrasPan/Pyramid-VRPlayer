#include <opencv2/core/cuda_types.hpp>
#include <opencv2/cudev/common.hpp>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <stdio.h>
#include <vector_types.h>
using namespace cv;
using namespace cv::cuda;
//自定义内核函数

__global__ void combine_kernel(const PtrStepSz<uchar4> imageL,
    const PtrStepSz<uchar4> imageR,
    const PtrStepSz<float> flowMagL,
    const PtrStepSz<float> flowMagR,
    PtrStep<uchar4> dst){
    int x = threadIdx.x + blockIdx.x * blockDim.x;
    int y = threadIdx.y + blockIdx.y * blockDim.y;

    if(x < imageL.cols && y < imageL.rows)
    {
        uchar4 colorL = imageL(y,x);
        uchar4 colorR = imageR(y,x);
        unsigned char outAlpha;
        if(colorL.w > colorR.w)
        {
            if(colorL.w / 255.0f > 0.1)
                outAlpha = 255;
            else
                outAlpha = 0;
        }
        else
        {
            if(colorR.w / 255.0f > 0.1)
                outAlpha = 255;
            else
                outAlpha = 0;
        }
        uchar4 colorMixed;
        if (colorL.w == 0 && colorR.w == 0) {
            colorMixed = make_uchar4(0, 0, 0, outAlpha);
        } else if (colorL.w == 0) {
            colorMixed = make_uchar4(colorR.x, colorR.y, colorR.z, outAlpha);
        } else if (colorR.w == 0) {
            colorMixed = make_uchar4(colorL.x, colorL.y, colorL.z, outAlpha);
        } else {
            const float magL = flowMagL(y,x) / float(imageL.cols);
            const float magR = flowMagR(y,x) / float(imageL.cols);
            float blendL = float(colorL.w);
            float blendR = float(colorR.w);
            float norm = blendL + blendR;
            blendL /= norm;
            blendR /= norm;
            const float colorDiff =
              (abs(colorL.x - colorR.x) +
               abs(colorL.y - colorR.y) +
               abs(colorL.z - colorR.z)) / 255.0f;
            const float kColorDiffCoef = 10.0f;
            const float kSoftmaxSharpness = 10.0f;
            const float kFlowMagCoef = 20.0f; // NOTE: this is scaled differently than the test version due to normalizing magL & magR by imageL.cols
            const float deghostCoef = tanhf(colorDiff * kColorDiffCoef);
            const double expL = exp(kSoftmaxSharpness * blendL * (1.0 + kFlowMagCoef * magL));
            const double expR = exp(kSoftmaxSharpness * blendR * (1.0 + kFlowMagCoef * magR));
            const double sumExp = expL + expR + 0.00001;
            const float softmaxL = float(expL / sumExp);
            const float softmaxR = float(expR / sumExp);
            colorMixed = make_uchar4(
                float(colorL.x)* (blendL * (1-deghostCoef) + softmaxL * deghostCoef) + float(colorR.x)*(blendR * (1-deghostCoef) + softmaxR * deghostCoef),
                float(colorL.y)* (blendL * (1-deghostCoef) + softmaxL * deghostCoef) + float(colorR.y)*(blendR * (1-deghostCoef) + softmaxR * deghostCoef),
                float(colorL.z)* (blendL * (1-deghostCoef) + softmaxL * deghostCoef) + float(colorR.z)*(blendR * (1-deghostCoef) + softmaxR * deghostCoef),  
                255);         
        }
        dst(y, x) = colorMixed;
        // uchar4 v = imageL(y,x);
        // dst(y,x) = make_uchar4(v.x,v.y,v.z,255);
    }
}

void combine_caller(const PtrStepSz<uchar4>& imageL,
    const PtrStepSz<uchar4>& imageR,
    const PtrStepSz<float>& flowMagL,
    const PtrStepSz<float>& flowMagR,
    PtrStep<uchar4> dst,cudaStream_t stream){
    dim3 block(32,8);
    dim3 grid((imageL.cols + block.x - 1)/block.x,(imageL.rows + block.y - 1)/block.y);

    combine_kernel<<<grid,block,0,stream>>>(imageL,imageR,flowMagL,flowMagR, dst);
    if(stream == 0)
        cudaDeviceSynchronize();
}

__global__ void shift_kernel(const PtrStepSz<float> shiftMat,
    PtrStep<uchar4> dst){
    int x = threadIdx.x + blockIdx.x * blockDim.x;
    int y = threadIdx.y + blockIdx.y * blockDim.y;

    if(x < shiftMat.cols && y < shiftMat.rows)
    {
        uchar4 image = dst(y,x);
        float alpha = shiftMat(y,x);
        image.w = (int)(image.w * alpha);
        dst(y,x) = image;
    }
}


void alpha_cuda_caller(const PtrStepSz<float>& shiftMat,
    PtrStep<uchar4> dst,
    cudaStream_t stream){
    dim3 block(32,8);
    dim3 grid((shiftMat.cols + block.x - 1)/block.x,(shiftMat.rows + block.y - 1)/block.y);

    shift_kernel<<<grid,block,0,stream>>>(shiftMat,dst);
    if(stream == 0)
        cudaDeviceSynchronize();
}
