#include <Arduino.h>
#include "SPIFFS.h"
#include "Spiffs_part.h"

tsSPIFFS sSPIFFS;

extern void BG96_Serial_Clearbuf();
extern void BG96_Serial_Read();
extern char cellular_Res[100];
extern void Bg96_Init();

extern class HardwareSerial Serial_port;

extern String Filename_with_ext, stored_file_name,Filename_with_ext1,Filename_with_ext2
;
extern uint32_t DataSize;
extern void Contenttypefor_Postingdata();

String ReadString;

/***************************************************************************************
   funciton name  ：vLoadSPIFFsDirectory
   parameter      ：Void
   return value   ：unsigned long
   effect         ：Here we are loading the files inside the Directory and keeping a count of files
                    that we created.
  /****************************************************************************************/

unsigned long  vLoadSPIFFsDirectory(void)
{
  sSPIFFS.File_Count = 0;
  //Allow stack operation, SPIFFS is not time critical
  //Serial.println("Dir found SPIFFS");

  File F_Name = SPIFFS.open("/cache");
  File file = F_Name.openNextFile();

  while (file)
  {
    sSPIFFS.Filename = file.name();
    Serial.print("File name is => ");Serial.println(sSPIFFS.Filename);
    long UTC = sSPIFFS.Filename.substring(7, 17).toInt();
    Serial.print("UTC value => ");Serial.println(UTC);
    sSPIFFS.FileUTC[sSPIFFS.File_Count] = UTC;
    sSPIFFS.File_Count += 1;
    file = F_Name.openNextFile();

  }
  return sSPIFFS.File_Count;

}

/***************************************************************************************
   funciton name  ：vFileDelete
   parameter      ：Filename_with_ext
   return value   ：Void
   effect         ：Deleting the Specified file
  /****************************************************************************************/

void vFileDelete(String  Filename_with_ext)
{
  Serial.println(Filename_with_ext);
  Serial.println("Trying to delete :" + Filename_with_ext + "this file" + Filename_with_ext1);


  SPIFFS.remove(Filename_with_ext);
  Serial.print(Filename_with_ext);
  SPIFFS.remove(Filename_with_ext1);
  Serial.print(Filename_with_ext);
  SPIFFS.remove(Filename_with_ext2);
   Serial.print(Filename_with_ext2);
  Serial.print("and");
  Serial.println(Filename_with_ext1);
  Serial.print("=>");
  Serial.println("file is deleted");

}

/***************************************************************************************
   funciton name  ：vSortDirectory
   parameter      ：Void
   return value   ：Void
   effect         ：Sorting the files inside the Directory for FIFO Implementation.
  /****************************************************************************************/

void vSortDirectory(void)
{
  yield();
  if (sSPIFFS.File_Count > 1)
  {
    Serial.println(sSPIFFS.File_Count);
    for (int i = 0; i < (sSPIFFS.File_Count - 1); i++) {
      for (int o = 0; o < (sSPIFFS.File_Count - (i + 1)); o++) {
        if (sSPIFFS.FileUTC[o] > sSPIFFS.FileUTC[o + 1]) {
          long t = sSPIFFS.FileUTC[o];
          sSPIFFS.FileUTC[o] = sSPIFFS.FileUTC[o + 1];
          sSPIFFS.FileUTC[o + 1] = t;
          Serial.println(sSPIFFS.FileUTC[o]);
        }
      }
    }
  }
}

void Http_postfor_PostingSpiffs_data()
{
 while (1)
  {
    
    char buf[DataSize];
    BG96_Serial_Clearbuf();
    Serial.println("Hey in the Function2");
    Serial.println(F("AT+QHTTPPOST=380,80,80"));
    sprintf(buf, "AT+QHTTPPOST=%d,%d,%d\r", DataSize, 80, 80);
    Serial_port.println(buf);
    BG96_Serial_Read( );
    if (strstr(cellular_Res, "CONNECT")    ) {                                        /* if return value is "OK" than program delay 1second jump out loop */
      Serial_port.println(ReadString);
      delay(10);
      break;
    }


  }

}

void Http_readfor_PostingSpiffs_data(String stored_file_name)
{
  while (1)
  {
    BG96_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPREAD=80"));
    Serial_port.println("AT+QHTTPREAD=80");
    BG96_Serial_Read( );
    if (strstr(cellular_Res, "CONNECT") != NULL) {                                        /* if return value is "OK" than program delay 1second jump out loop */
      Serial.print("SPIFFS file sent successfully to the server");
      vFileDelete(stored_file_name);
      delay(10);
      break;
    }
  }
}


void data_transfer_wifi() {


  //Contenttypefor_Postingdata();
  Http_postfor_PostingSpiffs_data();
  Http_readfor_PostingSpiffs_data(stored_file_name);

}


/* Only reading the file which we are sending to server */
void Readfile(String file_send)
{
  yield();
  ReadString = "";
  File SPIFFS_file;
  if (file_send)
  {
    
    stored_file_name =  file_send;                                                                              /* This the filename that we created inside SPIFFS */
                                                                                                                /* Checking wheather File exists or not */
    if (SPIFFS.exists( file_send))
    {
      SPIFFS_file = SPIFFS.open( file_send, "r");
    }
    if (!SPIFFS_file)
    {
      Serial.println("SPIFFs file open failure");
    }
    else
    {
      ReadString = SPIFFS_file.readStringUntil('\0');   //Reading the JSON format upto NULL character
    }
    File Fname = SPIFFS.open("/cache");
    File file = Fname.openNextFile();
  }
  
}
