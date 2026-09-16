#ifndef M360_LOG_H
#define M360_LOG_H

#include <stdio.h>

#define M360_LOG(channel, level, fmt, ...) \
    printf("[M360][%s][%s] " fmt "\n", channel, level, ##__VA_ARGS__)
#define M360_LOG_INFO(fmt, ...)  M360_LOG("BOOT", "INFO", fmt, ##__VA_ARGS__)
#define M360_LOG_WARN(fmt, ...)  M360_LOG("BOOT", "WARN", fmt, ##__VA_ARGS__)
#define M360_LOG_ERROR(fmt, ...) M360_LOG("BOOT", "ERROR", fmt, ##__VA_ARGS__)
#define M360_LOG_GPU(fmt, ...)   M360_LOG("VIDEO", "INFO", fmt, ##__VA_ARGS__)
#define M360_LOG_AUDIO(fmt, ...) M360_LOG("AUDIO", "INFO", fmt, ##__VA_ARGS__)
#define M360_LOG_INPUT(fmt, ...) M360_LOG("INPUT", "INFO", fmt, ##__VA_ARGS__)
#define M360_LOG_FS(fmt, ...)    M360_LOG("FS", "INFO", fmt, ##__VA_ARGS__)
#define M360_LOG_MEM(fmt, ...)   M360_LOG("MEM", "INFO", fmt, ##__VA_ARGS__)

#endif

