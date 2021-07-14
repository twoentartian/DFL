//
// Created by gzr on 07-07-21.
//

#pragma once

#include <vector>
#include <string>
#include <fstream>

#pragma warning(disable:4996)
#include <lz4.h>

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
		return LZ4_compress_fast(src, dst + SizeBytes, srcSize, dstCapacity - SizeBytes, acceleration) + SizeBytes;
	}
	
//	static void Compress(const char* src, int srcSize, std::vector<char>& dst, int acceleration = 1)
//	{
//		int dstSize = LZ4_compressBound(srcSize) + SizeBytes;
//
//		char* compressedData = new char[dstSize];
//		memcpy(compressedData, &srcSize, SizeBytes);
//
//		dstSize = LZ4_compress_fast(src, compressedData + SizeBytes, srcSize, dstSize - SizeBytes, acceleration);
//
//		dst.reserve(dstSize);
//		for (int i = 0; i < dstSize; ++i)
//		{
//			dst.push_back(compressedData[i]);
//		}
//		dst.shrink_to_fit();
//		delete[] compressedData;
//	}
	
	static int Decompress_CalculateDstSize(const char* src)
	{
		int dstCapacity;
		memcpy(&dstCapacity, src, SizeBytes);
		return dstCapacity;
	}
	
//	static int Decompress_CalculateDstSize(const std::vector<char>& compressed)
//	{
//		char decompressLengthBytes[SizeBytes];
//		int decompressLength;
//		for (uint8_t i = 0; i < SizeBytes; ++i)
//		{
//			decompressLengthBytes[i] = compressed[i];
//		}
//		memcpy(&decompressLength, decompressLengthBytes, SizeBytes);
//		return decompressLength;
//	}
	
	static int Decompress(const char* src, int srcSize, char* dst)
	{
		int dstCapacity;
		memcpy(&dstCapacity, src, SizeBytes);
		return LZ4_decompress_safe(src + SizeBytes, dst, srcSize - SizeBytes, dstCapacity);
	}
	
//	static void Decompress(const std::vector<char>& compressed, char* dst)
//	{
//		char decompressLengthBytes[SizeBytes];
//		int decompressLength;
//		for (uint8_t i=0; i< SizeBytes; ++i)
//		{
//			decompressLengthBytes[i] = compressed[i];
//		}
//		memcpy(&decompressLength, decompressLengthBytes, SizeBytes);
//
//		LZ4_decompress_safe(&compressed[0] + SizeBytes, dst, static_cast<int>(compressed.size()), decompressLength);
//	}
	

	
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
