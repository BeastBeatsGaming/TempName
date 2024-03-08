#pragma once
#include <stdio.h>

//This is for malloc()
#include <stdlib.h>

//This is for memset
#include <string.h>

//This is for timestamp of the files
#include <sys/stat.h>

//Defines
#define DEBUG_BREAK() __debugbreak()
#define BIT(x) (1 << x)
#define KB(x) ((unsigned long long)1024 * x)
#define MB(x) ((unsigned long long)1024 * KB(x))
#define GB(x) ((unsigned long long)1024 * MB(x))

//Logging
enum TextColor
{
    TEXT_COLOR_BLACK,
    TEXT_COLOR_RED,
    TEXT_COLOR_GREEN,
    TEXT_COLOR_YELLOW,
    TEXT_COLOR_BLUE,
    TEXT_COLOR_MAGENTA,
    TEXT_COLOR_CYAN,
    TEXT_COLOR_WHITE,
    TEXT_COLOR_BRIGHT_BLACK,
    TEXT_COLOR_BRIGHT_RED,
    TEXT_COLOR_BRIGHT_GREEN,
    TEXT_COLOR_BRIGHT_YELLOW,
    TEXT_COLOR_BRIGHT_BLUE,
    TEXT_COLOR_BRIGHT_MAGENTA,
    TEXT_COLOR_BRIGHT_CYAN,
    TEXT_COLOR_BRIGHT_WHITE,
    TEXT_COLOR_COUNT
};

template <typename... Args>
void _log(char* prefix, char* msg, TextColor textcolor, Args... args)
{
    static char* TextColorTable[TEXT_COLOR_COUNT] =
    {
        "\x1b[30m", //Black
        "\x1b[31m", //Red
        "\x1b[32m", //Green
        "\x1b[33m", //Yellow
        "\x1b[34m", //Blue
        "\x1b[35m", //Magenta
        "\x1b[36m", //Cyan
        "\x1b[37m", //White
        "\x1b[90m", //Bright Black
        "\x1b[91m", //Bright Red
        "\x1b[92m", //Bright Green
        "\x1b[93m", //Bright Yellow
        "\x1b[94m", //Bright Blue
        "\x1b[95m", //Bright Magenta
        "\x1b[96m", //Bright Cyan
        "\x1b[97m", //Bright White
    };

    char formatBuffer[8192] = {};
    sprintf(formatBuffer, "%s %s %s \033[0m", TextColorTable[textcolor], prefix, msg);

    char textBuffer[8192] = {};
    sprintf(textBuffer, formatBuffer, args...);

    puts(textBuffer);
}

#define SM_TRACE(msg,...) _log("TRACE: ", msg, TEXT_COLOR_GREEN, ##__VA_ARGS__);
#define SM_WARN(msg,...) _log("WARN: ", msg, TEXT_COLOR_YELLOW, ##__VA_ARGS__);
#define SM_ERROR(msg,...) _log("ERROR: ", msg, TEXT_COLOR_RED, ##__VA_ARGS__);

#define SM_ASSERT(x,msg,...)            \
{                                       \
    if (!(x))                           \
    {                                   \
        SM_ERROR(msg, ##__VA_ARGS__);   \
        DEBUG_BREAK();                  \
        SM_ERROR("ASSERTION HIT!")      \
    }                                   \
}

//Bump Allocator
struct BumpAllocator
{
    size_t capacity;
    size_t used;
    char* memory;
};

BumpAllocator make_bump_allocator(size_t size)
{
    BumpAllocator ba = {};

    ba.memory = (char*)malloc(size);
    if (ba.memory)
    {
        ba.capacity = size;
        memset(ba.memory, 0, size);
    }
    else
    {
        SM_ASSERT(false,"Failed to allocate memory!");
    }

    return ba;
}

char* bump_alloc(BumpAllocator* bumpAllocator, size_t size)
{
    char* result = nullptr;

    size_t alignedSize = (size + 7) & ~ 7; //Sets the first 4 bits to 0
    if (bumpAllocator->used + alignedSize <= bumpAllocator->capacity)
    {
        result = bumpAllocator->memory + bumpAllocator->used;
        bumpAllocator->used += alignedSize;
    }
    else
    {
        SM_ASSERT(false,"Bump Allocator out of memory!");
    }

    return result;
}

//File I/O
long long get_timestamp(char* file)
{
    struct stat file_stat = {};
    stat(file, &file_stat);
    return file_stat.st_mtime;
}

bool file_exists(char* filePath)
{
    SM_ASSERT(filePath, "No filepath provided!");

    auto file = fopen(filePath,"rb");
    if (!file) return false;
    fclose(file);

    return true;
}

long get_file_size(char* filePath)
{
    long fileSize = 0;
    
    auto file = fopen(filePath,"rb");
    if (!file)
    {
        SM_ERROR("Failed to open file: %s", filePath);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    fclose(file);

    return fileSize;
}

/*
Reads a file into a supplied buffer. We manage our own memory,
therefore we want control over where it is allocated.
*/

char* read_file(char* filePath, int* fileSize, char* buffer)
{
    SM_ASSERT(filePath, "No filepath provided!");
    SM_ASSERT(fileSize, "No fileSize provided!");
    SM_ASSERT(buffer, "No buffer provided!");

    *fileSize = 0;
    auto file = fopen(filePath,"rb");
    if (!file)
    {
        SM_ERROR("Failed to open file: %s", filePath);
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    *fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    memset(buffer, 0, *fileSize + 1);
    fread(buffer, sizeof(char), *fileSize, file);

    fclose(file);

    return buffer;
}

char* read_file(char* filePath, int* fileSize, BumpAllocator* bumpAllocator) //Overload for bump allocator
{
    char* file = nullptr;
    long fileSize2 = get_file_size(filePath);

    if (fileSize2)
    {
        char* buffer = bump_alloc(bumpAllocator, fileSize2 + 1);
        file = read_file(filePath, fileSize, buffer);
    }

    return file;
}

void write_file(char* filePath, char* buffer, int size)
{
    SM_ASSERT(filePath, "No filepath provided!");
    SM_ASSERT(buffer, "No buffer provided!");

    auto file = fopen(filePath,"wb");
    if (!file)
    {
        SM_ERROR("Failed to open file: %s", filePath);
        return;
    }

    fwrite(buffer, sizeof(char), size, file);
    fclose(file);
}

bool copy_file(char* fileName, char* outputName, char* buffer)
{
    int fileSize = 0;
    char* data = read_file(fileName, &fileSize, buffer);

    auto outputFile = fopen(outputName, "wb");
    if (!outputFile)
    {
        SM_ERROR("Failed to open file: %s", outputName);
        return false;
    }

    int result = fwrite(data, sizeof(char), fileSize, outputFile);
    if (!result)
    {
        SM_ERROR("Failed to write to file: %s", outputName);
        return false;
    }

    fclose(outputFile);
    return true;  //______________________________________________???????????
}

bool copy_file(char* fileName, char* outputName, BumpAllocator* bumpAllocator)
{
    char* file = 0;
    long fileSize2 = get_file_size(fileName);

    if (fileSize2)
    {
        char* buffer = bump_alloc(bumpAllocator, fileSize2 + 1);
        return copy_file(fileName, outputName, buffer);
    }

    return false;
}

//Math Stuff
struct Vec2
{
    float x;
    float y;
};

struct IVec2
{
    int x;
    int y;
};