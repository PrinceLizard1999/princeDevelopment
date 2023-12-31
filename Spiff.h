#ifndef __SPIFFS_PART_h__
#define __SPIFFS_PART_h__

#define CACHE_SIZE   2561  
//2561
typedef struct
{
  String Filename;
  String SortedFilename;
  unsigned long FileUTC[CACHE_SIZE];
  unsigned long File_Count;
} tsSPIFFS; //SPIFFs management cache structure



void vSortDirectory(void);
unsigned long  vLoadSPIFFsDirectory(void);
void data_transfer_wifi();

void Http_postfor_PostingSpiffs_data();
void Http_readfor_PostingSpiffs_data(String stored_file_name);
void Readfile(String file_send);
void vFileDelete(String  Filename_with_ext);
#endif
