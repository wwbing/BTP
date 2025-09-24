/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "file_utils.h"
#include "image_drawing.h"
#include "image_utils.h"
#include "yolov6.h"

/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("%s <model_path> <image_path_or_directory>\n", argv[0]);
        return -1;
    }

    const char *model_path = argv[1];
    const char *input_path = argv[2];

    // 检查输入是文件还是目录
    struct stat path_stat;
    if (stat(input_path, &path_stat) != 0)
    {
        printf("无法访问路径: %s\n", input_path);
        return -1;
    }

    int is_directory = S_ISDIR(path_stat.st_mode);

    int ret;
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));

    init_post_process();

    printf("正在初始化RKNN模型...\n");
    struct timeval model_init_start, model_init_end;
    gettimeofday(&model_init_start, NULL);

    ret = init_yolov6_model(model_path, &rknn_app_ctx);
    if (ret != 0)
    {
        printf("init_yolov6_model fail! ret=%d model_path=%s\n", ret, model_path);
        goto out;
    }

    gettimeofday(&model_init_end, NULL);
    long model_init_time = (model_init_end.tv_sec - model_init_start.tv_sec) * 1000 +
                          (model_init_end.tv_usec - model_init_start.tv_usec) / 1000;
    printf("RKNN模型初始化完成，耗时: %ld ms\n", model_init_time);

    if (is_directory)
    {
        // 批量处理目录中的图片
        printf("开始批量处理目录: %s\n", input_path);

        int image_count = 0;
        char** image_files = get_image_files_from_directory(input_path, &image_count);

        if (image_files == NULL || image_count == 0)
        {
            printf("在目录中未找到图片文件: %s\n", input_path);
            goto out;
        }

        printf("找到 %d 个图片文件\n", image_count);

        for (int i = 0; i < image_count; i++)
        {
            printf("\n处理图片 [%d/%d]: %s\n", i + 1, image_count, image_files[i]);

            image_buffer_t src_image;
            memset(&src_image, 0, sizeof(image_buffer_t));

            // 读取图片时间统计
            struct timeval read_start, read_end;
            gettimeofday(&read_start, NULL);

            ret = read_image(image_files[i], &src_image);

            if (ret != 0)
            {
                printf("读取图片失败! ret=%d image_path=%s\n", ret, image_files[i]);
                continue;
            }

            gettimeofday(&read_end, NULL);
            long read_time = (read_end.tv_sec - read_start.tv_sec) * 1000 +
                            (read_end.tv_usec - read_start.tv_usec) / 1000;
            printf("图片读取完成，耗时: %ld ms，图片尺寸: %dx%d\n", read_time, src_image.width, src_image.height);

            // 执行推理
            object_detect_result_list od_results;
            struct timeval inference_start, inference_end;
            gettimeofday(&inference_start, NULL);

            ret = inference_yolov6_model(&rknn_app_ctx, &src_image, &od_results);

            gettimeofday(&inference_end, NULL);
            long inference_time = (inference_end.tv_sec - inference_start.tv_sec) * 1000 +
                                (inference_end.tv_usec - inference_start.tv_usec) / 1000;
            printf("核心推理完成，耗时: %ld ms\n", inference_time);

            if (ret != 0)
            {
                printf("推理失败! ret=%d\n", ret);
                free(src_image.virt_addr);
                continue;
            }

            // 生成输出文件名
            char output_path[1024];
            const char* filename = strrchr(image_files[i], '/');
            if (filename == NULL) filename = image_files[i];
            else filename++;

            snprintf(output_path, sizeof(output_path), "out_%s", filename);

            // 画框和概率
            printf("检测到 %d 个目标:\n", od_results.count);
            char text[256];
            for (int j = 0; j < od_results.count; j++)
            {
                object_detect_result *det_result = &(od_results.results[j]);
                printf("  - %s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
                       det_result->box.left, det_result->box.top,
                       det_result->box.right, det_result->box.bottom,
                       det_result->prop);
                int x1 = det_result->box.left;
                int y1 = det_result->box.top;
                int x2 = det_result->box.right;
                int y2 = det_result->box.bottom;

                draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);

                sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
                draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 10);
            }

            // 保存结果图片
            struct timeval save_start, save_end;
            gettimeofday(&save_start, NULL);

            ret = write_image(output_path, &src_image);

            gettimeofday(&save_end, NULL);
            long save_time = (save_end.tv_sec - save_start.tv_sec) * 1000 +
                            (save_end.tv_usec - save_start.tv_usec) / 1000;

            if (ret != 0)
            {
                printf("保存图片失败: %s\n", output_path);
            }
            else
            {
                printf("结果已保存到: %s，耗时: %ld ms\n", output_path, save_time);
            }

            // 释放图片内存
            if (src_image.virt_addr != NULL)
            {
                free(src_image.virt_addr);
            }

            // 计算单张图片总处理时间
            struct timeval img_process_end;
            gettimeofday(&img_process_end, NULL);
            long img_process_time = (img_process_end.tv_sec - read_start.tv_sec) * 1000 +
                                   (img_process_end.tv_usec - read_start.tv_usec) / 1000;
            printf("单张图片总处理时间: %ld ms\n", img_process_time);
        }

        // 释放文件列表内存
        free_file_list(image_files, image_count);
        printf("\n批量处理完成! 共处理 %d 个图片文件\n", image_count);
    }
    else
    {
        // 单张图片处理
        printf("开始处理单张图片: %s\n", input_path);

        struct timeval read_start, read_end;
        gettimeofday(&read_start, NULL);

        ret = read_image(input_path, &src_image);

        if (ret != 0)
        {
            printf("read image fail! ret=%d image_path=%s\n", ret, input_path);
            goto out;
        }

        gettimeofday(&read_end, NULL);
        long read_time = (read_end.tv_sec - read_start.tv_sec) * 1000 +
                        (read_end.tv_usec - read_start.tv_usec) / 1000;
        printf("图片读取完成，耗时: %ld ms，图片尺寸: %dx%d\n", read_time, src_image.width, src_image.height);

        object_detect_result_list od_results;

        struct timeval inference_start, inference_end;
        gettimeofday(&inference_start, NULL);

        ret = inference_yolov6_model(&rknn_app_ctx, &src_image, &od_results);

        gettimeofday(&inference_end, NULL);
        long inference_time = (inference_end.tv_sec - inference_start.tv_sec) * 1000 +
                            (inference_end.tv_usec - inference_start.tv_usec) / 1000;
        printf("核心推理完成，耗时: %ld ms\n", inference_time);

        if (ret != 0)
        {
            printf("inference_yolov6_model fail! ret=%d\n", ret);
            goto out;
        }

        // 画框和概率
        printf("检测到 %d 个目标:\n", od_results.count);
        char text[256];
        for (int i = 0; i < od_results.count; i++)
        {
            object_detect_result *det_result = &(od_results.results[i]);
            printf("  - %s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
                   det_result->box.left, det_result->box.top,
                   det_result->box.right, det_result->box.bottom,
                   det_result->prop);
            int x1 = det_result->box.left;
            int y1 = det_result->box.top;
            int x2 = det_result->box.right;
            int y2 = det_result->box.bottom;

            draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);

            sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
            draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 10);
        }

        struct timeval save_start, save_end;
        gettimeofday(&save_start, NULL);

        ret = write_image("out.jpg", &src_image);

        gettimeofday(&save_end, NULL);
        long save_time = (save_end.tv_sec - save_start.tv_sec) * 1000 +
                        (save_end.tv_usec - save_start.tv_usec) / 1000;

        if (ret != 0)
        {
            printf("保存图片失败: out.jpg\n");
        }
        else
        {
            printf("结果已保存到: out.jpg，耗时: %ld ms\n", save_time);
        }

        // 计算总处理时间
        struct timeval total_end;
        gettimeofday(&total_end, NULL);
        long total_time = (total_end.tv_sec - read_start.tv_sec) * 1000 +
                         (total_end.tv_usec - read_start.tv_usec) / 1000;
        printf("单张图片总处理时间: %ld ms\n", total_time);
    }

out:
    deinit_post_process();

    ret = release_yolov6_model(&rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_yolov6_model fail! ret=%d\n", ret);
    }

    if (src_image.virt_addr != NULL)
    {
        free(src_image.virt_addr);
    }

    return 0;
}
