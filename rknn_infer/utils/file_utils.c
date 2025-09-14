#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_TEXT_LINE_LENGTH 1024

unsigned char* load_model(const char* filename, int* model_size)
{
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("fopen %s fail!\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    int model_len = ftell(fp);
    unsigned char* model = (unsigned char*)malloc(model_len);
    fseek(fp, 0, SEEK_SET);
    if (model_len != fread(model, 1, model_len, fp)) {
        printf("fread %s fail!\n", filename);
        free(model);
        fclose(fp);
        return NULL;
    }
    *model_size = model_len;
    fclose(fp);
    return model;
}

int read_data_from_file(const char *path, char **out_data)
{
    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        printf("fopen %s fail!\n", path);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    char *data = (char *)malloc(file_size+1);
    data[file_size] = 0;
    fseek(fp, 0, SEEK_SET);
    if(file_size != fread(data, 1, file_size, fp)) {
        printf("fread %s fail!\n", path);
        free(data);
        fclose(fp);
        return -1;
    }
    if(fp) {
        fclose(fp);
    }
    *out_data = data;
    return file_size;
}

int write_data_to_file(const char *path, const char *data, unsigned int size)
{
    FILE *fp;

    fp = fopen(path, "w");
    if(fp == NULL) {
        printf("open error: %s\n", path);
        return -1;
    }

    fwrite(data, 1, size, fp);
    fflush(fp);

    fclose(fp);
    return 0;
}

int count_lines(FILE* file)
{
    int count = 0;
    char ch;

    while(!feof(file))
    {
        ch = fgetc(file);
        if(ch == '\n')
        {
            count++;
        }
    }
    count += 1;

    rewind(file);
    return count;
}

char** read_lines_from_file(const char* filename, int* line_count)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open the file.\n");
        return NULL;
    }

    int num_lines = count_lines(file);
    printf("num_lines=%d\n", num_lines);
    char** lines = (char**)malloc(num_lines * sizeof(char*));
    memset(lines, 0, num_lines * sizeof(char*));

    char buffer[MAX_TEXT_LINE_LENGTH];
    int line_index = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';  // 移除换行符

        lines[line_index] = (char*)malloc(strlen(buffer) + 1);
        strcpy(lines[line_index], buffer);

        line_index++;
    }

    fclose(file);

    *line_count = num_lines;
    return lines;
}

void free_lines(char** lines, int line_count)
{
    for (int i = 0; i < line_count; i++) {
        if (lines[i] != NULL) {
            free(lines[i]);
        }
    }
    free(lines);
}

int is_image_file(const char* filename)
{
    if (!filename) return 0;

    const char* ext = strrchr(filename, '.');
    if (!ext) return 0;

    // 支持的图片格式
    const char* image_extensions[] = {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".tif"};
    int num_extensions = sizeof(image_extensions) / sizeof(image_extensions[0]);

    for (int i = 0; i < num_extensions; i++) {
        if (strcasecmp(ext, image_extensions[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

char** get_image_files_from_directory(const char* directory_path, int* file_count)
{
    DIR* dir;
    struct dirent* entry;
    char** image_files = NULL;
    int count = 0;
    int capacity = 10;

    *file_count = 0;

    dir = opendir(directory_path);
    if (dir == NULL) {
        printf("无法打开目录: %s\n", directory_path);
        return NULL;
    }

    // 分配初始内存
    image_files = (char**)malloc(capacity * sizeof(char*));
    if (image_files == NULL) {
        printf("内存分配失败\n");
        closedir(dir);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 .. 目录
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 检查是否是图片文件
        if (is_image_file(entry->d_name)) {
            // 构建完整路径
            char full_path[1024];
            int path_length = snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);

            if (path_length >= sizeof(full_path)) {
                printf("路径过长: %s/%s\n", directory_path, entry->d_name);
                continue;
            }

            // 检查是否是普通文件
            struct stat file_stat;
            if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
                // 如果容量不足，扩展数组
                if (count >= capacity) {
                    capacity *= 2;
                    char** temp = (char**)realloc(image_files, capacity * sizeof(char*));
                    if (temp == NULL) {
                        printf("内存重新分配失败\n");
                        break;
                    }
                    image_files = temp;
                }

                // 分配内存并复制路径
                image_files[count] = (char*)malloc(strlen(full_path) + 1);
                if (image_files[count] == NULL) {
                    printf("内存分配失败\n");
                    break;
                }

                strcpy(image_files[count], full_path);
                count++;
            }
        }
    }

    closedir(dir);

    *file_count = count;
    return image_files;
}

void free_file_list(char** file_list, int file_count)
{
    if (file_list == NULL) return;

    for (int i = 0; i < file_count; i++) {
        if (file_list[i] != NULL) {
            free(file_list[i]);
        }
    }
    free(file_list);
}