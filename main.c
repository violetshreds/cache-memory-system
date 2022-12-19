/*

This is a cache memory simulator that takes an input text file 
containing memory addresses and R/W specifications, and prompts the 
user for main memory size, cache size, cache block size, degree
of set associativity, replacement, write policy and name of the
memory reference file. 
Using this input, the program generates the main memory table, final 
cache status, and calculates the total address lines, number of bits 
for the offset, index, and tag, as well as total cache size, best 
possible hit rate, and average hit rate
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/*struct used to create and store the user's input*/
typedef struct _memory {
    int MainMemorySize;
    int CacheSize;
    int CacheBlockSize;
    int SetAssoc;
    char ReplacementPolicy[2];
    char WritePolicy[2];
} Memory;
/*struct used to store cache characteristics*/
typedef struct _size {
    int AddressBits;
    int OffsetBits; 
    int IndexBits; 
    int TagBits; 
    int NumCacheBlocks; 
    int NumCacheSets;
    int TotalTagBits; 
    int ValidBit;
    int DirtyBit;
    int TotalCacheSize;
} Size;
/*struct used to store the input file data*/
typedef struct _filecontents {
    int ReadOrWrite; 
    int Address; 
    int NumberofMemoryReferences;
    
} FileContents;
/*struct used to create main memory table*/
typedef struct _memorylocationtable {
    int MainMemoryBlockNum; 
    int CacheMemorySetNum;
    int CacheMemoryBlockNum;
    char HitOrMiss[10];
    int tag;
} MemoryLocationTable;
/*struct used to fill cache */
typedef struct _cache {
    char DirtyBit[2];
    int ValidBit;
    char tag[100];
    char data[10];
    int setnum;
    int blknum;
    char HitorMiss[10];
} Cache;

//below are the function declarations used 
FileContents *ReadFile();
MemoryLocationTable *CreateMemLocTable();
int CalculateMainMemBlkNum();
int CalculateCacheMemBlkNum();
int CalculateCacheMemSetNum();
void CalculateBestPossibleHitRate();
Cache CreateCache();

/*This function takes input from the user and stores it in a struct to 
be passed to other functions to create the cache and main memory tables*/
Memory* GetUserData(char *filename) {
    Memory *userinput = (Memory*)malloc(sizeof(Memory));

    printf("Enter the size of main memory in bytes: ");
    scanf("%d", &userinput->MainMemorySize);
    printf("Enter the size of the cache in bytes: ");
    scanf("%d", &userinput->CacheSize);
    printf("Enter the cache block/line size: ");
    scanf("%d", &userinput->CacheBlockSize);
    printf("Enter the degree of set-associativity (input n for an n-way set-associative mapping): ");
    scanf("%d", &userinput->SetAssoc);
    printf("Enter the replacement policy (L = LRU , F = FIFO) : ");
    scanf("%s", userinput->ReplacementPolicy);
    printf("Enter the write policy (B = write-back , T = write-through) : ");
    scanf("%s", userinput->WritePolicy);
    printf("Enter the name of the input file containing the list of memory references generated by the CPU: ");
    scanf("%s", filename);
    printf("%s\n", filename);
    
    return userinput;
}

/*This function reads the input file and stores the info in a struct*/
FileContents* ReadFile(char *filename, Memory *userinput, Size *BytesAndBits) {

    FileContents *txtFileContents = (FileContents*)malloc(sizeof(FileContents));
    FILE *fp;
    fp = fopen(filename, "r");

    if (!filename) {
    printf("FILE NOT FOUND\n");
    return NULL;
    }
    int num;
    fscanf(fp, "%d", &num);//stores the number of mem reference lines
    txtFileContents->NumberofMemoryReferences = num;
    FileContents *Matrix[num];
        for (int i = 0; i < num; i++) {
            Matrix[i] = (FileContents*)malloc(sizeof(FileContents));
    }
    //create an array of struct type FileContents to store each line
    FileContents matrixtry[num];
    char str[5];
    int intparser;
    int array[num];
    int i = 0;
    //scan W/R and memory address
    while (fscanf(fp, "%s %d\n", str, &intparser) != EOF) {
        //stores R/W data as an integer 0 or 1
        if (strcmp(str, "R") == 0) {
            matrixtry[i].ReadOrWrite = 0;
            array[i] = 0;
        }
        if (strcmp(str, "W") == 0) {
            matrixtry[i].ReadOrWrite = 1;
            array[i] = 1;
        }
        matrixtry[i].Address = intparser;
        matrixtry[i].NumberofMemoryReferences = num;
      i++;
    }
    fclose(fp);
    //call function below at the end to create the memory location table
    CreateMemLocTable(matrixtry, userinput, BytesAndBits);
    return *Matrix;//was returning txtFileContents so this could cause some problems
}
/*This funcation calculates the cache size characterisitics and returns them as a struct*/
Size* CalculateSize(Memory *userinput) {
    Size *BytesAndBits = (Size*)malloc(sizeof(Size));

    BytesAndBits->AddressBits = log2(userinput->MainMemorySize);
    BytesAndBits->OffsetBits = log2(userinput->CacheBlockSize);
    BytesAndBits->IndexBits = log2(userinput->CacheSize/(userinput->CacheBlockSize*userinput->SetAssoc));
    BytesAndBits->TagBits = BytesAndBits->AddressBits-(BytesAndBits->OffsetBits + BytesAndBits->IndexBits);
    BytesAndBits->NumCacheBlocks = userinput->CacheSize / userinput->CacheBlockSize;
    BytesAndBits->NumCacheSets = BytesAndBits->NumCacheBlocks / userinput->SetAssoc;
    BytesAndBits->TotalTagBits = (BytesAndBits->TagBits * BytesAndBits->NumCacheBlocks) / 8; 
    BytesAndBits->ValidBit = BytesAndBits->NumCacheBlocks / 8;
    BytesAndBits->DirtyBit = BytesAndBits->ValidBit;
    BytesAndBits->TotalCacheSize = userinput->CacheSize + BytesAndBits->TotalTagBits + BytesAndBits->ValidBit + BytesAndBits->DirtyBit;
    
    return BytesAndBits;
}
/*print function to print cache size data */
void PrintBytesAndBits(Size *BytesAndBits) {
    printf("Total address lines required: %d\n", BytesAndBits->AddressBits); 
    printf("Number of bits for offset: %d\n", BytesAndBits->OffsetBits);
    printf("Number of bits for index: %d\n", BytesAndBits->IndexBits);
    printf("Number of bits for tag: %d\n", BytesAndBits->TagBits);
    printf("Total cache size required: %d bytes\n", BytesAndBits->TotalCacheSize);
    return;
}

/* this function creates the memory table  */
MemoryLocationTable* CreateMemLocTable(FileContents *filecontents, Memory *userinput, Size *BytesAndBits) {
    MemoryLocationTable *table = (MemoryLocationTable*)malloc(sizeof(MemoryLocationTable));
    int size =  userinput->CacheSize / userinput->CacheBlockSize;
    int setAssoc = userinput->SetAssoc;
    //create array of type Cache to store the contents of each block
    Cache initializeCache[size];
    char s[] = "X";
    char q[] = "?";
    //loop populates or initializes the empty cache
    //size variable is number of cache blocks used
    for (int i = 0; i < size; i++) {
        initializeCache[i].ValidBit = 0;
        strcpy(initializeCache[i].DirtyBit, s);
        strcpy(initializeCache[i].tag, s); 
        strcpy(initializeCache[i].data, q);
        initializeCache[i].blknum = i;
        initializeCache[i].setnum = i / setAssoc;
    }

    /*LRU matrix used to hold the cache block number of the least
    recently accessed location-upon initialization the lru for each
    cache set is always the first block in the set*/ 
    int LRU_matrix[size][BytesAndBits->NumCacheSets];
    for (int st = 0; st < BytesAndBits->NumCacheSets; st++) {
        for (int sz = 0; sz < setAssoc; sz++) {
            LRU_matrix[st][sz] = (st * setAssoc) + sz;
        }
    }
    /*table array created to store data of each line in the memory location table*/
    MemoryLocationTable tableArray[filecontents[0].NumberofMemoryReferences];
    int numMemRef = filecontents[0].NumberofMemoryReferences;

    /*cache block number matrix to generate the set of cache blocks associated with 
    each memory address */
    int cacheblkNumMatrix[numMemRef][userinput->SetAssoc];
   //loop below stores data into the main memory table struct array by calling 3 functions
    for (int w = 0; w < filecontents[0].NumberofMemoryReferences; w++) {
        tableArray[w].MainMemoryBlockNum = CalculateMainMemBlkNum(filecontents, BytesAndBits, userinput, w);
        tableArray[w].CacheMemorySetNum = CalculateCacheMemSetNum(filecontents, BytesAndBits, userinput, w, tableArray);
        tableArray[w].tag = tableArray[w].MainMemoryBlockNum / pow(2, BytesAndBits->IndexBits);
        //for loop below used to determine the set of cache blocks associated with each memory address
        for (int j = 0; j < userinput->SetAssoc; j++) {
        cacheblkNumMatrix[w][j] = CalculateCacheMemBlkNum(filecontents, BytesAndBits, userinput, w, tableArray, j);
        }
    }
    char repL[] = "L";
    char repB[] = "B";
    int SetisFull = 1;
    int CachePos;
    int ActualHits = 0;
    int hitFlag = 0;
    int k = 0;
    int flag = 0;
    
    int LRU[BytesAndBits->NumCacheSets];
    int FIFO[BytesAndBits->NumCacheSets];
    //initializes LRU of each set to be the first block in the set
    for (int j = 0; j < BytesAndBits->NumCacheSets; j++) {
        LRU[j] = LRU_matrix[j][0];
    }

        for (int i = 0; i < numMemRef; i++) {//loop through number of mem references
            for (int j = 0; j < BytesAndBits->NumCacheSets; j++) {//loop number of cache sets
            LRU[j] = LRU_matrix[j][0];//re-initialize LRU location
                if (tableArray[i].CacheMemorySetNum == j) { //if cache set matches mm set
                        hitFlag = 0;
                        SetisFull = 1;
                        //initialize cache block number in relation to set associativity
                        int cacheblocknumber = (j * (userinput->SetAssoc))+k;
                        for (k = 0; k < userinput->SetAssoc; k++) {//loop blocks in the set
                        cacheblocknumber = (j * (userinput->SetAssoc))+k;
                        char str[100];
                        sprintf(str, "%d", tableArray[i].tag);//convert tag to a string
                            //if the tags match we get a cache hit 
                            if (strcmp(str, initializeCache[cacheblocknumber].tag) == 0) { 
                            hitFlag = 1;
                            strcpy(tableArray[i].HitOrMiss, "Hit");
                            strcpy(initializeCache[cacheblocknumber].tag, str);
                            ActualHits++;//keep track of number of cache hits
                            FIFO[j] = cacheblocknumber;//update FIFO
                        
                            initializeCache[cacheblocknumber].ValidBit = 1;
                                //if memory address is a Write 
                                if (filecontents[i].ReadOrWrite == 1) {
                                    //if write-back policy 
                                    if (strcmp(userinput->WritePolicy, "B") == 0) {
                                        strcpy(initializeCache[cacheblocknumber].DirtyBit, "1");
                                    }
                                    //if write-through policy
                                    else if (strcmp(userinput->WritePolicy, "T") == 0) {
                                        strcpy(initializeCache[cacheblocknumber].DirtyBit, "X");
                                    }
                                }
                                //if memory address is a Read
                                else if (filecontents[i].ReadOrWrite == 0) {
                                    if (strcmp(userinput->WritePolicy, "B") == 0) {
                                        strcpy(initializeCache[cacheblocknumber].DirtyBit, "0");
                                    }
                                    else if (strcmp(userinput->WritePolicy, "T") == 0) {
                                        strcpy(initializeCache[cacheblocknumber].DirtyBit, "X");
                                    }
                                }
                            //this if statement handles the case where the LRU is at the bottom of the cache set and the cache is full
                            if ((j == (BytesAndBits->NumCacheSets) - 1) || strcmp(initializeCache[(j * (userinput->SetAssoc)) + ((userinput->SetAssoc)-1)].tag, "X") != 0){
                                //store location just accessed in temporary variable
                                int temp = LRU_matrix[j][cacheblocknumber];
                                //for loop shifts all other values in the LRU matrix up 
                                for (int mov = k+1; mov < size-1; mov++) {
                                    LRU_matrix[j][mov-1] = LRU_matrix[j][mov];
                                    //stores the most recently used at the bottom or end of the matrix
                                    LRU_matrix[j][size-1] = temp;
                                }
                                //this is so that the LRU of that set can always be found at position 0
                                LRU[j] = LRU_matrix[j][0];
                            }
                            if (hitFlag)break;
                          }
                         if (hitFlag)break;
                        }
                    if (!hitFlag) {//if no cache hit and the set is not full
                            for (k = 0; k < userinput->SetAssoc; k++) {//loop through blocks
                                cacheblocknumber = j * (userinput->SetAssoc)+k;
                                if (strcmp(initializeCache[cacheblocknumber].tag, "X") == 0) {
                                    SetisFull = 0;
                                    //fill the next available spot 
                                    //update the FIFO
                                    FIFO[j] = cacheblocknumber;
                                    strcpy(tableArray[i].HitOrMiss, "Miss");
                                    char str[100];
                                    sprintf(str, "%d", tableArray[i].tag);
                                    strcpy(initializeCache[cacheblocknumber].tag, str);
                                    sprintf(initializeCache[cacheblocknumber].data, "%d", tableArray[i].MainMemoryBlockNum);
                                    initializeCache[cacheblocknumber].ValidBit = 1;
                                    if (filecontents[i].ReadOrWrite == 1) {
                                        if (strcmp(userinput->WritePolicy, "B") == 0) {
                                            strcpy(initializeCache[cacheblocknumber].DirtyBit, "1");
                                        }
                                        else if (strcmp(userinput->WritePolicy, "T") == 0) {
                                            strcpy(initializeCache[cacheblocknumber].DirtyBit, "X");
                                        }
                                    }
                                    else if (filecontents[i].ReadOrWrite == 0) {
                                        if (strcmp(userinput->WritePolicy, "B") == 0) {
                                            strcpy(initializeCache[cacheblocknumber].DirtyBit, "0");
                                        }
                                        else if (strcmp(userinput->WritePolicy, "T") == 0) {
                                            strcpy(initializeCache[cacheblocknumber].DirtyBit, "X");
                                        }
                                    }
                                    hitFlag = 1;
                                    break;
                                }
                            }

                        if (SetisFull) {//if no cache hits and the set is full
                            flag = 1;
                            strcpy(tableArray[i].HitOrMiss, "Miss");
                            char str[100]; 
                            sprintf(str, "%d", tableArray[i].tag);
                                //if user selects LRU policy
                                if (strcmp(userinput->ReplacementPolicy, "L") == 0) {
                                    //the new contents get stored at the LRU position stored in the LRU array
                                    strcpy(initializeCache[LRU[j]].tag, str);
                                    sprintf(initializeCache[LRU[j]].data, "%d", tableArray[i].MainMemoryBlockNum);
                                        if (filecontents[i].ReadOrWrite == 1) {
                                            if (strcmp(userinput->WritePolicy, "B") == 0) {
                                                strcpy(initializeCache[LRU[j]].DirtyBit, "1");
                                            }
                                            else if (strcmp(userinput->WritePolicy, "T") == 0) {
                                                strcpy(initializeCache[LRU[j]].DirtyBit, "X");
                                            }
                                        }
                                        else if (filecontents[i].ReadOrWrite == 0) {
                                            if (strcmp(userinput->WritePolicy, "B") == 0) {
                                                strcpy(initializeCache[LRU[j]].DirtyBit, "0");
                                            }
                                            else if (strcmp(userinput->WritePolicy, "T") == 0) {
                                                strcpy(initializeCache[LRU[j]].DirtyBit, "X");
                                            }
                                        }
                                        //for loop used to shift the contents of the LRU 
                                        int temp = LRU_matrix[j][0];
                                        for (int mov = 0; mov < k; mov++) {
                                            LRU_matrix[j][mov] = LRU_matrix[j][mov+1];
                                        }
                                        LRU_matrix[j][k-1] = temp;
                                        LRU[j] = LRU_matrix[j][0];   
                                        }
                                        //if user selects FIFO replacement policy
                                        else if (strcmp(userinput->ReplacementPolicy, "F") == 0) {
                                            sprintf(str, "%d", tableArray[i].tag); 
                                            strcpy(initializeCache[FIFO[j]].tag, str);
                                            sprintf(initializeCache[FIFO[j]].data, "%d", tableArray[i].MainMemoryBlockNum);
                                            if (filecontents[i].ReadOrWrite == 1) {
                                                if (strcmp(userinput->WritePolicy, "B") == 0) {
                                                    strcpy(initializeCache[FIFO[j]].DirtyBit, "1");
                                                }
                                                else if (strcmp(userinput->WritePolicy, "T") == 0) {
                                                    strcpy(initializeCache[FIFO[j]].DirtyBit, "X");
                                                }
                                            }
                                            else if (filecontents[i].ReadOrWrite == 0) {
                                                 if (strcmp(userinput->WritePolicy, "B") == 0) {
                                                    strcpy(initializeCache[FIFO[j]].DirtyBit, "0");
                                                }
                                                else if (strcmp(userinput->WritePolicy, "T") == 0) {
                                                    strcpy(initializeCache[FIFO[j]].DirtyBit, "X");
                                                }
                                            }
                                            FIFO[j] = FIFO[j]; 
                                        }
                                    hitFlag = 1;
                                    if (hitFlag)break;
                        }
                        if (hitFlag)break;
                    }
                }
            }
        }
//printing main memory location table
printf("\nmain memory address\tmm blk#\t\t   cm set#\t    cm blk#\t     hit/miss\n");
printf("______________________________________________________________________________________\n");
for (int i = 0; i < filecontents[0].NumberofMemoryReferences; i++) {
    printf("  %d\t\t\t", filecontents[i].Address);
    printf("  %d\t\t    ", tableArray[i].MainMemoryBlockNum);
    printf("  %d\t\t    ", tableArray[i].CacheMemorySetNum);
    if (userinput->SetAssoc == 1) {
        printf("  %d\t\t", cacheblkNumMatrix[i][0]);
    }
    else {
        printf("  %d-%d\t\t", cacheblkNumMatrix[i][0], cacheblkNumMatrix[i][(userinput->SetAssoc)-1]);
    }
    printf("%s\n", tableArray[i].HitOrMiss);    
}
//printing final status of the cache
printf("\nFINAL STATUS OF THE CACHE:\n");
printf("Cache blk#\tdirty bit\tvalid bit\ttag\t\tData\n");
printf("______________________________________________________________________________________\n");
for (int i = 0; i < size; i++) {
        printf(" %d\t\t", initializeCache[i].blknum);
        printf(" %s\t\t", initializeCache[i].DirtyBit);
        printf(" %d\t\t", initializeCache[i].ValidBit);
        if (strcmp(initializeCache[i].tag, "X") == 0) {
            for (int t = 0; t < BytesAndBits->TagBits; t++) {
                printf("X");
            }
        }
        else {//convert tag to binary
            int num = atoi(initializeCache[i].tag);
            int rem;
            int bin[BytesAndBits->TagBits];
            for (int t = BytesAndBits->TagBits; t > 0; t--) {
                rem = num % 2;
                bin[t-1] = rem;
                num = num / 2;
            }
            for (int t = 0; t < BytesAndBits->TagBits; t++) {
                printf("%d", bin[t]);
            }
        }
        printf("\t\tmm blk #%s\n", initializeCache[i].data);   
    }

printf("\n");
//print actual cache hit rate
printf("Actual Hit Rate = %d/%d = %.2f%%\n", ActualHits, numMemRef, (float)100*ActualHits/numMemRef);

int refsize = filecontents[0].NumberofMemoryReferences;
//calculates best possible hit rate
CalculateBestPossibleHitRate(tableArray, refsize);

    /*return table;*/
} 

/* this function will  be called by the CreateMemLocTable function and calculate mm blk#*/
int CalculateMainMemBlkNum(FileContents *filecontents, Size *BytesAndBits, Memory *userinput, int i) {
    int blocknum = filecontents[i].Address / userinput->CacheBlockSize;
    return blocknum;
}

/* this function will  be called by the CreateMemLocTable function and calculate cm blk#*/
int CalculateCacheMemBlkNum(FileContents *filecontents, Size *BytesAndBits, Memory *userinput, int i, MemoryLocationTable *tableArray, int j) {
    int blknum = filecontents[i].Address / userinput->CacheBlockSize;
    int cache =  userinput->CacheSize / userinput->CacheBlockSize;
    int num = blknum % cache;
    int possiblecacheblocknums[userinput->CacheBlockSize][userinput->SetAssoc];
    int blkmatrix[userinput->SetAssoc];
    int totalCacheSets = (userinput->CacheSize / userinput->CacheBlockSize) / userinput->SetAssoc;
    int k = 0;
    for (int w = 0; w < totalCacheSets; w++) {
        for (int p = 0; p < userinput->SetAssoc; p++) {
            blkmatrix[p] = k;
            possiblecacheblocknums[w][p] = blkmatrix[p];
            k++;
        }
    }
    //returns one block number at a time associated with each cache set 
    //this is because this function is called from inside a loop 
    int CacheSetNum = (tableArray[i].MainMemoryBlockNum) % totalCacheSets;
    return possiblecacheblocknums[CacheSetNum][j];
}

/* this function will  be called by the CreateMemLocTable function and calculate cm set#*/
int CalculateCacheMemSetNum(FileContents *filecontents, Size *BytesAndBits, Memory *userinput, int i, MemoryLocationTable *tableArray) {
    int numCacheBlocksPerSet = userinput->SetAssoc;
    int totalCacheSets = (userinput->CacheSize / userinput->CacheBlockSize) / numCacheBlocksPerSet; 
    int CacheSetNum = tableArray[i].MainMemoryBlockNum % totalCacheSets; 
    return CacheSetNum;
}

/*  this function will calculate and the best possible hit rate*/
void CalculateBestPossibleHitRate(MemoryLocationTable *tableArray, int size) {
    int hits[size];
    int hit = 0;
    for (int i = 0; i < size; i++) {
        int element = tableArray[i].MainMemoryBlockNum;
        int flag = 0;
        for (int j = i+1; j < size; j++) {
            //compares each block in main memory to the blocks below and counts 
            //the number of repeats 
            if (tableArray[i].MainMemoryBlockNum == tableArray[j].MainMemoryBlockNum) {
                flag = 1;
                hit++;
                hits[hit] = element;
                break;
            }
        }
            if (flag) {
                continue;
            }   
    }
    printf("\n");
    float hitfloat = hit * 1.00;
    float sizefloat = size * 1.00;
    float hitrate = (hitfloat/sizefloat) * 100.00;
    printf("Best possible hit rate = %d/%d = %.2f%%\n", hit, size, hitrate);
    //return;
}

int main () {

char ch = 'y';
do {//while user continues to enter y
char filename[20];
Memory *userinput = GetUserData(filename);
Size *BytesAndBits = CalculateSize(userinput);
PrintBytesAndBits(BytesAndBits);
FileContents *fileContents = ReadFile(filename,userinput, BytesAndBits);
printf("\nContinue? (y = yes, n = no): ");
scanf(" %c", &ch);
}while (ch=='y');
    return 0;
}

