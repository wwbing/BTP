// Copyright (c) 2023 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "common.h"
#include "file_utils.h"
#include "image_utils.h"
#include "yolov6.h"


static long long get_current_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int init_yolov6_model(const char *model_path, rknn_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    printf("开始初始化YOLOv6模型...\n");
    printf("模型路径: %s\n", model_path);

    // Load RKNN Model
    printf("正在加载RKNN模型文件...\n");
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL)
    {
        printf("模型文件加载失败!\n");
        return -1;
    }
    printf("模型文件加载成功，大小: %d bytes\n", model_len);

    printf("正在初始化RKNN上下文...\n");
    ret = rknn_init(&ctx, model, model_len, RKNN_NPU_CORE_0_1_2, NULL);
    free(model);
    if (ret < 0)
    {
        printf("RKNN初始化失败! ret=%d\n", ret);
        return -1;
    }
    printf("RKNN上下文初始化成功\n");

    // 设置NPU核心掩码
    printf("正在设置NPU核心掩码 (RKNN_NPU_CORE_0_1_2)...\n");
    ret = rknn_set_core_mask(ctx, RKNN_NPU_CORE_0_1_2);
    if (ret != RKNN_SUCC)
    {
        printf("NPU核心掩码设置失败! ret=%d\n", ret);
        return -1;
    }
    printf("NPU核心掩码设置成功，将使用所有3个NPU核心\n");

    // Get Model Input Output Number
    printf("正在查询模型输入输出数量...\n");
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC)
    {
        printf("查询输入输出数量失败! ret=%d\n", ret);
        return -1;
    }
    printf("模型输入数量: %d, 输出数量: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("正在查询模型输入信息...\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("查询输入属性失败! ret=%d\n", ret);
            return -1;
        }
        printf("输入 %d: name=%s, dims=[%d, %d, %d, %d], fmt=%d\n",
               i, input_attrs[i].name,
               input_attrs[i].dims[0], input_attrs[i].dims[1],
               input_attrs[i].dims[2], input_attrs[i].dims[3],
               input_attrs[i].fmt);
    }

    // Get Model Output Info
    printf("正在查询模型输出信息...\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("查询输出属性失败! ret=%d\n", ret);
            return -1;
        }
        printf("输出 %d: name=%s, dims=[%d, %d, %d, %d], type=%d\n",
               i, output_attrs[i].name,
               output_attrs[i].dims[0], output_attrs[i].dims[1],
               output_attrs[i].dims[2], output_attrs[i].dims[3],
               output_attrs[i].type);
    }

    // Set to context
    app_ctx->rknn_ctx = ctx;

    // 检查模型是否为量化模型
    if (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC && output_attrs[0].type == RKNN_TENSOR_INT8)
    {
        app_ctx->is_quant = true;
        printf("模型为INT8量化模型\n");
    }
    else
    {
        app_ctx->is_quant = false;
        printf("模型为FP32非量化模型\n");
    }

    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        app_ctx->model_channel = input_attrs[0].dims[1];
        app_ctx->model_height = input_attrs[0].dims[2];
        app_ctx->model_width = input_attrs[0].dims[3];
    }
    else
    {
        app_ctx->model_height = input_attrs[0].dims[1];
        app_ctx->model_width = input_attrs[0].dims[2];
        app_ctx->model_channel = input_attrs[0].dims[3];
    }
    printf("模型输入尺寸: %dx%dx%d (HxWxC)\n",
           app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);
    printf("YOLOv6模型初始化完成!\n");

    return 0;
}

int release_yolov6_model(rknn_app_context_t *app_ctx)
{
    printf("开始释放YOLOv6模型资源...\n");

    if (app_ctx->input_attrs != NULL)
    {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
        printf("已释放输入属性内存\n");
    }
    if (app_ctx->output_attrs != NULL)
    {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
        printf("已释放输出属性内存\n");
    }
    if (app_ctx->rknn_ctx != 0)
    {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
        printf("已销毁RKNN上下文\n");
    }
    printf("YOLOv6模型资源释放完成!\n");
    return 0;
}

int inference_yolov6_model(rknn_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results)
{
    int ret;
    image_buffer_t dst_img;
    letterbox_t letter_box;
    rknn_input inputs[app_ctx->io_num.n_input];
    rknn_output outputs[app_ctx->io_num.n_output];
    const float nms_threshold = NMS_THRESH;      // 默认的NMS阈值
    const float box_conf_threshold = BOX_THRESH; // 默认的置信度阈值
    int bg_color = 114;

    // Time measurement variables
    long long start_time, preprocess_time = 0, inference_time = 0, postprocess_time = 0, total_time;
    long long postprocess_start = 0;

    start_time = get_current_time_ms();

    if ((!app_ctx) || !(img) || (!od_results))
    {
        printf("推理参数错误: app_ctx=%p, img=%p, od_results=%p\n", app_ctx, img, od_results);
        return -1;
    }

    memset(od_results, 0x00, sizeof(*od_results));
    memset(&letter_box, 0, sizeof(letterbox_t));
    memset(&dst_img, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    printf("开始预处理...\n");
    // Pre Process
    long long preprocess_start = get_current_time_ms();
    dst_img.width = app_ctx->model_width;
    dst_img.height = app_ctx->model_height;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = get_image_size(&dst_img);
    dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
    if (dst_img.virt_addr == NULL)
    {
        printf("预处理内存分配失败!\n");
        return -1;
    }

    // letterbox
    printf("正在执行letterbox变换 (输入: %dx%d -> 输出: %dx%d)...\n",
           img->width, img->height, dst_img.width, dst_img.height);
    ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
    if (ret < 0)
    {
        printf("letterbox变换失败!\n");
        return -1;
    }
    preprocess_time = get_current_time_ms() - preprocess_start;
    printf("预处理完成，耗时: %lld ms\n", preprocess_time);

    // Set Input Data
    printf("正在设置输入数据...\n");
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
    inputs[0].buf = dst_img.virt_addr;

    ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
    if (ret < 0)
    {
        printf("输入数据设置失败! ret=%d\n", ret);
        return -1;
    }
    printf("输入数据设置成功\n");

    // Run
    printf("开始NPU推理...\n");
    long long inference_start = get_current_time_ms();
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    inference_time = get_current_time_ms() - inference_start;
    if (ret < 0)
    {
        printf("NPU推理失败! ret=%d\n", ret);
        return -1;
    }
    printf("NPU推理完成，耗时: %lld ms\n", inference_time);

    // Get Output
    printf("正在获取推理结果...\n");
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < app_ctx->io_num.n_output; i++)
    {
        outputs[i].index = i;
        outputs[i].want_float = (!app_ctx->is_quant);
    }
    ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
    if (ret < 0)
    {
        printf("获取推理结果失败! ret=%d\n", ret);
        goto out;
    }
    printf("推理结果获取成功，共 %d 个输出\n", app_ctx->io_num.n_output);

    // Post Process
    printf("开始后处理 (NMS和过滤)...\n");
    postprocess_start = get_current_time_ms();
    post_process(app_ctx, outputs, &letter_box, box_conf_threshold, nms_threshold, od_results);
    postprocess_time = get_current_time_ms() - postprocess_start;
    printf("后处理完成，耗时: %lld ms\n", postprocess_time);

    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);

out:
    if (dst_img.virt_addr != NULL)
    {
        free(dst_img.virt_addr);
    }

    total_time = get_current_time_ms() - start_time;

    // 推理性能统计 (只统计NPU推理和后处理)
    long long core_inference_time = inference_time + postprocess_time;
    printf("=== 推理性能统计 ===\n");
    printf("NPU推理: %lld ms (%.1f%%)\n", inference_time, (float)inference_time / core_inference_time * 100);
    printf("后处理: %lld ms (%.1f%%)\n", postprocess_time, (float)postprocess_time / core_inference_time * 100);
    printf("核心推理总耗时: %lld ms (100%%)\n", core_inference_time);
    printf("===================\n");

    return ret;
}