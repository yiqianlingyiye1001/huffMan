#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>

#include "hd.h"//头文件
#include "huffMan.h"
#include "backend.h"

#include <windows.h>


// 哈夫曼压缩/解压核心函数 -释放顺序错误+内存泄漏+资源兜底释放

int pureHUFFMAN()
{
    const char* originalFile = "./test/123.txt";//解压缩后文件
    const char* compressFile = "./test/compress123.HM";//解压缩后文件
    const char* decompressFile = "./test/decompress123.txt";//解压缩后文件


    //--------------读取原始文件-------------------
    SqString data = readOriginalFile(originalFile);

    //------------------构建哈夫曼树--------------------
    int n0 = getWeight(data);

    HTNode* ht = new HTNode[n0 * 2 - 1];
    createNode(ht, data);
    createHT(ht, n0);

    HCode* hcd = new HCode[n0];
    createHCode(ht, hcd, n0);

    //-------------------生成压缩文件----------------
    char* hufCode = printHCode(data, ht, hcd, n0);

    FILE* fp1 = fopen(compressFile, "wb");
    if (!fp1)
    {
        printf("压缩文件创建失败\n");
        // 兜底释放所有内存，无内存泄漏
        delete[] data.s;
        delete[] ht;
        for (int i = 0; i < n0; i++)
        {
            delete[] hcd[i].cd;
        }
        delete[] hcd;
        delete[] hufCode;
        return -1;
    }
    else printf("压缩文件创建成功\n");


    fwrite(&n0, sizeof(int), 1, fp1);
    fwrite(ht, sizeof(HTNode), 2 * n0 - 1, fp1);
    fwrite(&data.length, sizeof(int), 1, fp1);

    runCompress(fp1, hufCode);

    fclose(fp1);
    // 先释放子元素，再释放数组本体，无野指针
    delete[] data.s;
    delete[] ht;
    for (int i = 0; i < n0; i++)
    {
        delete[] hcd[i].cd;
    }
    delete[] hcd;
    delete[] hufCode;

    //----------------解压缩文件------------------
    FILE* fpCompress = fopen(compressFile, "rb");
    if (!fpCompress)
    {
        printf("打开压缩文件失败\n");
        return -1;
    }
     else printf("打开压缩文件成功！！！\n");


    FILE* fpDecompress = fopen(decompressFile, "wb");
    if (!fpDecompress)
    {
        printf("创建失败！！！\n");
        fclose(fpCompress); // 兜底关闭文件句柄
        return -1;
    }
      else printf("解压缩文件创建成功！！！\n");


    rundeCompress(compressFile, decompressFile, fpCompress, fpDecompress);

    fclose(fpCompress);
    fclose(fpDecompress);
 printf("qwq任务完成\n");

    return 0;
}



// 哈夫曼压缩/解压核心函数
int HUFFMAN(const char* originalFile, const char* compressFile)
//int HUFFMAN()
{
    SetConsoleOutputCP(CP_UTF8);

    // const char* originalFile = "./test/2.gif";//解压缩后文件
    // const char* compressFile = "./test/compress2.H";//解压缩后文件
     //const char* decompressFile = "./test/decompress123.gif";//解压缩后文件

    const char* decompressFile = "./test/decompress0.txt";//解压缩后文件

    //--------------读取原始文件-------------------
    SqString data = readOriginalFile(originalFile);

    //------------------构建哈夫曼树--------------------
    int n0 = getWeight(data);

    HTNode* ht = new HTNode[n0 * 2 - 1];
    createNode(ht, data);
    createHT(ht, n0);

    HCode* hcd = new HCode[n0];
    createHCode(ht, hcd, n0);

    //-------------------生成压缩文件----------------
    char* hufCode = printHCode(data, ht, hcd, n0);

    FILE* fp1 = fopen(compressFile, "wb");
    if (!fp1)
    {
        printf("压缩文件创建失败\n");
        // 兜底释放所有内存，无内存泄漏
        delete[] data.s;
        delete[] ht;
        for (int i = 0; i < n0; i++)
        {
            delete[] hcd[i].cd;
        }
        delete[] hcd;
        delete[] hufCode;
        return -1;
    }
    else printf("压缩文件创建成功\n");


    fwrite(&n0, sizeof(int), 1, fp1);
    fwrite(ht, sizeof(HTNode), 2 * n0 - 1, fp1);
    fwrite(&data.length, sizeof(int), 1, fp1);

    runCompress(fp1, hufCode);

    fclose(fp1);
    // 先释放子元素，再释放数组本体，无野指针
    delete[] data.s;
    delete[] ht;
    for (int i = 0; i < n0; i++)
    {
        delete[] hcd[i].cd;
    }
    delete[] hcd;
    delete[] hufCode;

    //----------------解压缩文件------------------
    FILE* fpCompress = fopen(compressFile, "rb");
    if (!fpCompress)
    {
        printf("打开压缩文件失败\n");
        return -1;
    }
    else printf("打开压缩文件成功！！！\n");



    FILE* fpDecompress = fopen(decompressFile, "wb");
    if (!fpDecompress)
    {
        printf("创建失败！！！\n");
        fclose(fpCompress); // 兜底关闭文件句柄
        return -1;
    }
    else printf("解压缩文件创建成功！！！\n");



    rundeCompress(compressFile, decompressFile, fpCompress, fpDecompress);

    fclose(fpCompress);
    fclose(fpDecompress);

    printf("qwq任务完成\n");
    return 0;
}
