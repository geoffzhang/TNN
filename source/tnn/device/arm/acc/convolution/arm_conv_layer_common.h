// Tencent is pleased to support the open source community by making TNN available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#ifndef TNN_SOURCE_TNN_DEVICE_ARM_ARM_CONV_LAYER_ACC_COMMON_H_
#define TNN_SOURCE_TNN_DEVICE_ARM_ARM_CONV_LAYER_ACC_COMMON_H_

#include "tnn/device/arm/acc/arm_layer_acc.h"
#include "tnn/utils/omp_utils.h"

namespace TNN_NS {

class ArmConvLayerCommon : public ArmLayerAcc {
public:
    virtual ~ArmConvLayerCommon();

    Status Init(Context *context, LayerParam *param, LayerResource *resource, const std::vector<Blob *> &inputs,
                const std::vector<Blob *> &outputs);

    virtual Status DoForward(const std::vector<Blob *> &inputs, const std::vector<Blob *> &outputs);

    // always true as last solution
    static bool isPrefered(ConvLayerParam *param, const std::vector<Blob *> &inputs,
                           const std::vector<Blob *> &outputs);

    template <typename T>
    Status Exec(const std::vector<Blob *> &inputs, const std::vector<Blob *> &outputs);

    // alloc conv params and set post op
    virtual Status allocateBufferWeight(const std::vector<Blob *> &inputs, const std::vector<Blob *> &outputs);

    virtual Status allocateBufferBias(const std::vector<Blob *> &inputs, const std::vector<Blob *> &outputs);

protected:
    RawBuffer buffer_weight_;
    RawBuffer buffer_bias_;
    PostFunc post_func_ = nullptr;

    template <typename T>
    void PostExec(const std::vector<Blob *> &outputs) {
        const int batch = outputs[0]->GetBlobDesc().dims[0];
        auto dst_origin = reinterpret_cast<T *>(GetBlobHandlePtr(outputs[0]->GetHandle()));
        if (post_func_) {
            OMP_PARALLEL_FOR_
            for (int batch_idx = 0; batch_idx < batch; ++batch_idx) {
                auto output_ptr = dst_origin + batch_idx * k_param_->ow * k_param_->oh * k_param_->oc_r4;
                for (int dz = 0; dz < k_param_->oc_r4; dz += 4) {
                    auto dst_z    = output_ptr + dz * k_param_->ow * k_param_->oh;
                    float *bias_z = reinterpret_cast<float *>(k_param_->bias) + dz;
                    post_func_(dst_z, bias_z, k_param_->ow * k_param_->oh, 1);
                }
            }
        }
    };
};

}  // namespace TNN_NS

#endif  // TNN_SOURCE_TNN_DEVICE_ARM_ARM_CONV_LAYER_ACC_COMMON_H_
