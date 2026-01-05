#ifndef HD_H_INCLUDED
#define HD_H_INCLUDED
#define MM 1024*1024 //支持压缩文件的大小

extern int weight[256];//权重，0-255所有字节值

//存储二进制数据
typedef struct
{
    unsigned char* s;
    int length;
} SqString; //ByteData原始字节数据

//哈夫曼树节点
typedef struct
{
    unsigned char data;
    int weight;
    int parent;
    int lchild;
    int rchild;
} HTNode;

//哈夫曼编码存储
typedef struct
{
    char* cd;//栈溢出？
    int start;
} HCode;


//构建哈夫曼数模块
int getWeight(SqString msg);
void createNode(HTNode ht[],SqString msg);
void  createHT(HTNode ht[],int n0);//HTNode *ht???
void createHCode(HTNode ht[],HCode hcd[],int n0);

//压缩文件模块
void runCompress(FILE* fp1,char* hufCode);
char* printHCode(SqString msg,HTNode ht[],HCode hcd[],int n0);
SqString readOriginalFile(const char *originalFile);

//解压缩文件模块
void rundeCompress(const char* compressFile,const char* decompressFile,FILE*fpCompress,FILE*fpDecompress);


#endif // HD_H_INCLUDED
