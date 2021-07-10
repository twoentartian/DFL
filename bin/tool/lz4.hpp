//
// Created by gzr on 07-07-21.
//

#ifndef LZ4_HPP
#define LZ4_HPP

#include <vector>
#include <string>
#include <fstream>

#pragma warning(disable:4996)
//#include <lz4.h>
#include "../3rd/lz4/lib/lz4.h"
#include "../3rd/lz4/lib/lz4.c"


class LZ4
{
private:
    const static uint8_t SizeBytes = 4;

public:
    static int Compress_CalculateDstSize(int srcSize)
    {
        return LZ4_compressBound(srcSize) + SizeBytes;
    }

    static int Compress(const char* src, int srcSize, char* dst, int dstCapacity, int acceleration = 1)
    {
        memcpy(dst, &srcSize, SizeBytes);
        return LZ4_compress_fast(src, dst+ SizeBytes, srcSize, dstCapacity - SizeBytes, acceleration);
    }

    static void Compress(const char* src, int srcSize, std::vector<char>& dst, int acceleration = 1)
    {
        int dstSize = LZ4_compressBound(srcSize) + SizeBytes;

        char* compressedData = new char[dstSize];
        memcpy(compressedData, &srcSize, SizeBytes);

        dstSize = LZ4_compress_fast(src, compressedData + SizeBytes, srcSize, dstSize - SizeBytes, acceleration);

        dst.reserve(dstSize);
        for (int i = 0; i < dstSize; ++i)
        {
            dst.push_back(compressedData[i]);
        }

        delete[] compressedData;
    }

    static int Decompress_CalculateDstSize(const char* src)
    {
        int dstCapacity;
        memcpy(&dstCapacity, src, SizeBytes);
        return dstCapacity;
    }

    static int Decompress_CalculateDstSize(const std::vector<char>& compressed)
    {
        char decompressLengthBytes[SizeBytes];
        int decompressLength;
        for (uint8_t i = 0; i < SizeBytes; ++i)
        {
            decompressLengthBytes[i] = compressed[i];
        }
        memcpy(&decompressLength, decompressLengthBytes, SizeBytes);
        return decompressLength;
    }

    static int Decompress(const char* src, int srcSize, char* dst)
    {
        int dstCapacity;
        memcpy(&dstCapacity, src, SizeBytes);
        return LZ4_decompress_safe(src+ SizeBytes, dst, srcSize, dstCapacity);
    }

    static void Decompress(const std::vector<char>& compressed, char* dst)
    {
        char decompressLengthBytes[SizeBytes];
        int decompressLength;
        for (uint8_t i=0; i< SizeBytes; ++i)
        {
            decompressLengthBytes[i] = compressed[i];
        }
        memcpy(&decompressLength, decompressLengthBytes, SizeBytes);

        LZ4_decompress_safe(&compressed[0] + SizeBytes, dst, static_cast<int>(compressed.size()), decompressLength);
    }

    //// Write Memory Data to Disk
    static void WriteToFile(const std::string& path, const char* data, int dataSize, int acceleration = 1)
    {
        int dstCapacity = LZ4_compressBound(dataSize) + SizeBytes;
        char* dst = new char[dstCapacity];
        memcpy(dst, &dataSize, SizeBytes);
        int realSize = LZ4_compress_fast(data, dst+ SizeBytes, dataSize, dstCapacity - SizeBytes, acceleration) + SizeBytes;

        std::ofstream file;
        file.open(path, std::ios::binary | std::ios::out);
        file.write(dst, realSize);
        file.close();

        delete[] dst;
    }

    static int ReadFromFile_CalculateDstSize(const std::string& path)
    {
        std::ifstream file;
        file.open(path, std::ios::binary | std::ios::in);
        char decompressLengthBytes[SizeBytes];
        int decompressLength;
        file.read(decompressLengthBytes, SizeBytes);
        file.close();

        memcpy(&decompressLength, decompressLengthBytes, SizeBytes);
        return decompressLength;
    }

    //Make sure that the data is pointed to an array of length returned by ReadFromFile_CalculateDstSize.
    static void ReadFromFile(const std::string& path, char* data)
    {
        std::ifstream file;
        file.open(path, std::ios::binary | std::ios::in);
        file.seekg(0, std::ios::end);
        const int srcSize = static_cast<int>(file.tellg());
        file.seekg(0, std::ios::beg);
        char* src = new char[srcSize];
        file.read(src, srcSize);
        file.close();

        int dstCapacity;
        memcpy(&dstCapacity, src, SizeBytes);
        LZ4_decompress_safe(src + SizeBytes, data, srcSize, dstCapacity);

        delete[] src;
    }

    //// Zip and Unzip
    static void Zip(const std::string& inputPath, const std::string& outputPath)
    {
        std::ifstream inputFile;
        inputFile.open(inputPath, std::ios::in | std::ios::binary);
        if (!inputFile)
        {
            //throw std::exception("Input File not found");
            LOG(ERROR) << "Input File not found";
        }
        inputFile.seekg(0, std::ios::end);
        const int inputFileSize = static_cast<int>(inputFile.tellg());
        inputFile.seekg(0, std::ios::beg);

        char* inputData = new char[inputFileSize];
        inputFile.read(inputData, inputFileSize);
        inputFile.close();

        int outputFileSize = Compress_CalculateDstSize(inputFileSize);
        char* outputData = new char[outputFileSize];
        outputFileSize = Compress(inputData, inputFileSize, outputData, outputFileSize);

        std::ofstream outputFile;
        outputFile.open(outputPath, std::ios::binary | std::ios::out);
        outputFile.write(outputData, outputFileSize);
        outputFile.close();

        delete[] inputData;
        delete[] outputData;
    }

    static void Unzip(const std::string& inputPath, const std::string& outputPath)
    {
        std::ifstream inputFile;
        inputFile.open(inputPath, std::ios::in | std::ios::binary);
        if (!inputFile)
        {
            //throw std::exception("Input File not found");
            LOG(ERROR) << "Input File not found";
        }
        inputFile.seekg(0, std::ios::end);
        const int inputFileSize = static_cast<int>(inputFile.tellg());
        inputFile.seekg(0, std::ios::beg);

        char* inputData = new char[inputFileSize];
        inputFile.read(inputData, inputFileSize);
        inputFile.close();

        int outputFileSize = Decompress_CalculateDstSize(inputData);
        char* outputData = new char[outputFileSize];
        assert(Decompress(inputData, inputFileSize, outputData) == outputFileSize);

        std::ofstream outputFile;
        outputFile.open(outputPath, std::ios::binary | std::ios::out);
        outputFile.write(outputData, outputFileSize);
        outputFile.close();

        delete[] inputData;
        delete[] outputData;
    }
};

#endif // !LZ4_HPP