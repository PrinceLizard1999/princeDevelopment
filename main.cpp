#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <Wire.h>  // we are taking this library for directly reading to i2c adress
#include <MC3672.h> // this library basically for mc3672 accelerometer sensor 
#include <esp_task_wdt.h> //To disable watchdog
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
unsigned long  start_tempTime;
uint8_t tempCount;
uint16_t acmCount;
float accelerometerData[3];
//char  floatInStrings[];
#include <OneWire.h>
#include <DallasTemperature.h>

//declaration  for acclerpmmeter
// Declare the variables at a higher scope so that they can be accessed by both functions
    String minXStr, maxXStr, avgXStr, minYStr, maxYStr, avgYStr, minZStr, maxZStr, avgZStr;
// Define global variables to store accelerometer data
float minX = 0.0;
float maxX = 0.0;
float avgX = 0.0;
float minY = 0.0;
float maxY = 0.0;
float avgY = 0.0;
float minZ = 0.0;
float maxZ = 0.0;
float avgZ = 0.0;

 time_t timeSinceEpoch;

/******************** Libraries for internal time increment ******************/
#include "sys/time.h"
#include "time.h"
#include <ArduinoQueue.h>
/****************************************************************************/
//707 => No SIM_CARD INSERTED
#define QUEUE_SIZE_ITEMS 30
#define true 1
#define false 0
String GpsStr ;
String Trimm_data;
uint16_t StrCount;
//uint16_t strInQ = 0;
//size_t gpsconn_time ;
//uint8_t JsonCount;
unsigned long spiffsFileCounter = 0;
//701 /707 => DATA PACK EXPIRED
#define GPIO_PIN   5
OneWire DS18B20(GPIO_PIN);
DallasTemperature sensors(&DS18B20);
float temperatureC;
int  C_Temp;
byte addr[8];
String smac ;
  uint8_t ulog ;
/*
   PIN_DEFINITIONS
*/
#define RXD1                     23
#define TXD1                     18
#define RXD2                     16
#define TXD2                     17

#define C_MODEM_ENABLE           33
/*************************************************/
MC3672 MC3672_acc = MC3672();   // we make here mc3672 class which is directly  control the register 
int i;

ArduinoQueue<String> intQueue(QUEUE_SIZE_ITEMS);
long int Temp_data[10];
long int Acm_data[30];//  that is the size of storing the data of acclerometer
bool EpochEnable = false;
char *j, *k;


String GPVTG_STR, GNGGA_STR;
String L_STR = "";
bool loop1 = true;
bool isDatainQ = true;
bool enableTask3 = false;
bool StartTimer = false ;

/******************* Library and Spiffs.h file inclusion ********************/
#include "SPIFFS.h"
#include "Spiffs_part.h"
/****************************************************************************/
struct timeval ts;
/******************** Macro Definitions *************************************/

#define FUNC_EXEC_TIME 20


#define TEMP_FETCH_TIME 30
/************** PIN Definitions *****************/
#define RXD1  23                            // Must be connected to DBG_TXD, DBG_RXD
#define TXD1  18

#define RXD2 16                            // Must be connected to TXD, RXD 
#define TXD2 17                            //For getting instant output of GPS

/************************************************/

#define uS_TO_S_FACTOR 1000000ULL                                                              /* Conversion factor for micro seconds to seconds */
//#define TIME_TO_SLEEP   0

/*****************************************************************************/
long UTC_to_EPOCH(char *, char *);
void creatReqStrings(String, String);
/********************** Boolean variables Declarations **********************/
//bool bgetData = false;
bool bPwrOnRST = true;
bool QueueActivate = false;
bool ClkEnable = true;


/****************************************************************************/

  void Contenttypefor_Postingdata(void);
  void Request_header(void);
  void  responseheader_for_getEpoch(void);
  void Http_URLfor_Epoch(void);
  void Http_getfor_Epoch(void);
  void Http_readfor_Epoch(void);
  void createSPIFFSfor_Temp(void);
  
  void createAccelerometerFile(void);


/********************** Global Variable declarations ************************/

long  Epoch;
long int gps_time[10];
String dest_arr[20];
uint8_t u8LogSeq = 0; //Number of modified GPS strings created in TASK2


String Filename_with_ext, stored_file_name,Filename_with_ext1,Filename_with_ext2;
String file_name;
String ACFile_name;


char Response[712], requestBody[2048];
char cellular_Res[2048];
/******************************************** BG95_Serial_Clearbuf() *******************************************/
/******************************************* Fetch_REQ_NMEASTR() *********************************************/
/*
   Description   : Here we will wait until we encountered with proper response from BG95 that is
                   getting Proper String ( GGA + VTG ) with correct variables in it and add
                   GGA+VTG( Combination of two strings ) into Queue
*/



uint16_t DataSize ;
uint16_t AcmVal;
uint8_t TaskCount = 0;
uint8_t EpochTimeCounter = 0;                                                                                                                  
//uint8_t u8LogCounter = 0;

String receive_buff1[10], receive_buff2[10];

char *terminator2;


/*************************** Queue Handlers *******************************/

TaskHandle_t Task1, Task2, Task3,Task4;

/*************************** Hardware Serial port declarations ******************************/
HardwareSerial BG95_Serial1(1);     // UART definition used for Cell-modem
HardwareSerial BG95_Serial2(2);    // UART definition used for GPS connection


/********************************************************************************************/

/*****************************************************************************************************************/
void BG95_Serial_Clearbuf()
{
  BG95_Serial1.flush();
  while (BG95_Serial1.available())
  {
    BG95_Serial1.read();
  }
  BG95_Serial1.flush();

  while (BG95_Serial2.available()) {
    BG95_Serial2.read();
  }
  BG95_Serial2.flush();
}


void BG95_Serial_Read( )
{
  int i = 0;
  memset(cellular_Res, 0 , sizeof(cellular_Res));    /* array is cleaned in here */
  BG95_Serial1.flush();                               /* Waits for the transmission of outgoing serial data to complete */
  delay(2000);                                        /* Waits for respones from server */


  while (BG95_Serial1.available() && i < sizeof(cellular_Res) - 1) {
    cellular_Res[i] = BG95_Serial1.read();            /* read return's value and store in this array */
    i++;
  }
  cellular_Res[i] = '\0';
  Serial.println(cellular_Res);                      /* for debuging */


}

void PriorityCheck()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println("DATA COMING!!!");
    BG95_Serial1.println(F("AT+QGPSCFG=\"priority\""));
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(100);//GSA+GGA
      break;
    }
  }
}

void Cell_modem_Priority()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    BG95_Serial1.println(F("AT+QGPSCFG=\"priority\",1,1"));
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(3000);
      break;
    }
  }
}
void GNSSPriority()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    BG95_Serial1.println(F("AT+QGPSCFG=\"priority\",0,1"));
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(3000);
      break;
    }
  }
}
/*
   HTTP_AT commands for EPOCH value fetching
*/

void EPOCH_from_server_AT()
{
  Contenttypefor_Postingdata();
  Request_header();
  responseheader_for_getEpoch();
  Http_URLfor_Epoch();
  Http_getfor_Epoch();
  Http_readfor_Epoch();
}
/*************************************************************************************************/

void Request_header()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPCFG=\"requestheader\",0"));
    BG95_Serial1.println("AT+QHTTPCFG=\"requestheader\",0");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "OK") != NULL) {                                             /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}
void responseheader_for_getEpoch()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPCFG=\"responseheader\",0"));
    BG95_Serial1.println("AT+QHTTPCFG=\"responseheader\",0");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "OK") != NULL) {                                                /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}

void Http_URLfor_Epoch()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPURL=51,80"));
    BG95_Serial1.println("AT+QHTTPURL=51,80");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "CONNECT") != NULL) {                                              /* if return value is "OK" than program delay 1second jump out loop */
      BG95_Serial1.println("https://lizardmonitoring.com/app/time/getServerTime");
      delay(80);
      break;
    }
  }
}

void Http_getfor_Epoch()
{
  unsigned long start_time = millis();
  unsigned long end_time;
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPGET=80"));
    BG95_Serial1.println("AT+QHTTPGET=80");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "OK")) {                                               /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }

  }
}

void Http_readfor_Epoch()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPREAD=80"));
    BG95_Serial1.println("AT+QHTTPREAD=80");
    BG95_Serial_Read( );
    long val;
    if (strstr(cellular_Res, "CONNECT") != NULL  ) {                                            /* if return value is "OK" than program delay 1second jump out loop */
      delay(1000);
      char *p = cellular_Res;
      while (*p)
      {
        if (isdigit(*p) && isdigit(*(p + 1)) )
        {
          val = strtol(p, &p, 0);                                                              //To get the number from the string which returns the long we use strtol
        }
        p++;
      }

      Epoch = val;
      ts.tv_sec = Epoch;
      Serial.print("Epoch value is =>"); Serial.println(Epoch);
      settimeofday(&ts, NULL);
      delay(80);
      break;
    }
  }
}

char* PDP_Activate_Check()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QIACT?"));
    BG95_Serial1.println("AT+QIACT?");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "OK") != NULL) {                                                /* if return value is "OK" than program delay 1second jump out loop */
      return cellular_Res;
      delay(80);
      break;
    }
  }
}

void PDP_Activate()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QIACT=1"));
    BG95_Serial1.println("AT+QIACT=1");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "OK") != NULL) {                                                /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}

void Contenttypefor_Postingdata()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPCFG=\"contenttype\",1"));
    BG95_Serial1.println("AT+QHTTPCFG=\"contenttype\",1");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "OK") != NULL) {                                                 /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}

void Request_headerfor_Postingdata()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPCFG=\"requestheader\",1"));
    BG95_Serial1.println("AT+QHTTPCFG=\"requestheader\",1");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "OK") != NULL) {                                             /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}

void Response_headerfor_Postingdata()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPCFG=\"responseheader\",1"));
    BG95_Serial1.println("AT+QHTTPCFG=\"responseheader\",1");
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "OK") != NULL) {                                             /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}



void Http_URLfor_Postingdata()
{
  while (1)
  {
    char buf[100];
    char* URL = "http://3.94.8.139:8080/app/vehicleTracking/saveGPSData";
    String URL_1 = "http://3.94.8.139:8080/app/vehicleTracking/saveGPSData";
    //Serial.println(F("AT+QHTTPURL=32,80"));//60
    sprintf(buf, "AT+QHTTPURL=%d,%d", URL_1.length(), 80);
    BG95_Serial1.println(buf);
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "CONNECT") != NULL) {                                    /* if return value is "OK" than program delay 1second jump out loop */
      BG95_Serial1.println(URL_1);
      delay(80);
      break;
    }
  }
}

void sssu()
{
  unsigned long start_time = millis();
  while (1)
  {
    char buf_1[DataSize];
    BG95_Serial_Clearbuf();
   // Serial.println(F("AT+QHTTPPOST=400,80,80"));
    sprintf(buf_1, "AT+QHTTPPOST=%d,%d,%d", DataSize, 80, 80);
    Serial.println(buf_1);
    BG95_Serial1.println(buf_1);
    BG95_Serial_Read( );
    if (strstr(cellular_Res, "CONNECT") ) {                                        /* if return value is "OK" than program delay 1second jump out loop */
      BG95_Serial1.println(requestBody);
      delay(80);
      break;
    }

  }
}


void Http_readfor_Postingdata()
{
  //bgetData = true;
  unsigned long start_time = millis();
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println(F("AT+QHTTPREAD=80"));
    BG95_Serial1.println("AT+QHTTPREAD=80");
    BG95_Serial_Read( );
    //bgetData = true;
    if ((strstr(cellular_Res, "HTTP/1.1 200") != NULL)  || (strstr(cellular_Res, "+QHTTPPOST: 0,200") != NULL ) || (strstr(cellular_Res, "\"status\":200") != NULL) || (strstr(cellular_Res, "HTTP Status 404 ") != NULL ))
    {
      Serial.println("Server seen successfully");

      vFileDelete(stored_file_name);  
      vTaskDelay(80);
      Serial.println("Http_read AT done with 200 response code");
      //vTaskResume(Task2);
      break;

    }
  }

}
/*
  Description: Waiting for the response from the UART2
*/


void vWaitResponseFromBG95_UART2(String cmd, String field)
{
  Serial.println(field);
  memset(Response, 0, sizeof(Response));
  BG95_Serial2.flush();                                             /* save the mcu return message for AT Command */
  i = 0;
  /* Waits for the transmission of outgoing serial data to complete.*/
  delay(2000);                                                                 /* Waits for respones from server */

  while (BG95_Serial2.available() && i < sizeof(Response) - 1)
  {
    Response[i] = BG95_Serial2.read();//read return's value and store in this array
    i++;
  }
  Response[i] = '\0';
  Serial.println("Response from UART2");
  Serial.println(Response);

}

/*void vWaitResponseFromBG95_UART1(String cmd, String field)
  {
  //BG95_Serial1.println(cmd); UART1
  memset(Response, 0, sizeof(Response));
  BG95_Serial1.flush();                                             /* save the mcu return message for AT Command */
//i = 0;
/* Waits for the transmission of outgoing serial data to complete.
  delay(2000);                                                                 Waits for respones from server
  while (BG95_Serial1.available() && i < sizeof(Response) - 1)
  {
  Response[i] = BG95_Serial1.read();//read return's value and store in this array
  //Serial.print("Bit by bit response =>");Serial.println(Response[i]);
  i++;
  }
  Response[i] = '\0';
  Serial.println("...........................");
  Serial.println(Response);

  }*/


/*************************************************************************************************/
/*
   Description   : Here we will wait until we encountered with proper response from BG95 that is
                   getting Proper String (GGA + VTG ) with correct variables in it and add
                   GGA+VTG( Combination of two strings ) into Queue
*/
/************************************************************************************************/



void Fetch_REQ_NMEASTR()
{
  const char code1[] = "$";
  char *delimiter;
  int index1 = 0;
  if (isDatainQ == true)
  {
    vWaitResponseFromBG95_UART2("AT+QGPSLOC?", "GPSLOC");                                             // Waiting for the NMEA_RESPONSE
    size_t len = strlen(Response);
    
    delimiter = strtok(Response, code1);
    Serial.println("Fetch_REQ_NMEASTR!!!");
    while (len >= 256 )
    {
      while (delimiter != NULL)
      {
         
        if (((strstr(delimiter, "GPVTG") != NULL) || strstr(delimiter, "GNVTG") != NULL)) //index1 == 0 && 
        {
          GPVTG_STR = delimiter ;
          if ( GPVTG_STR.length() >= 30)
          {
            Serial.println("VTG_STR ");
          }

          
        }
        else if (((strstr(delimiter, "GPGGA") != NULL) || strstr(delimiter, "GNGGA") != NULL))  //index1 == 1 && 
        {
           
          
          GNGGA_STR = delimiter;
          if ( GNGGA_STR.length() >= 76)
          {
            //intQueue.enqueue(GNGGA_STR); //GNGGA_STR.length() = 76;25
            Serial.println("GGA_STR ");
             
            }
           Serial.print("index value is ");Serial.println(index1);
          break;
          
        }
        delimiter = strtok(NULL, code1);
        
        index1++;
        
      }
      creatReqStrings(GNGGA_STR, GPVTG_STR);
      intQueue.enqueue( Trimm_data );
      printf(" Data is added into Queue");
      TaskCount++;
      isDatainQ = false;
      vTaskDelay(10);
      QueueActivate = true;
      //Serial.println("Queue Activate is ON so TASK1 is OFF");
      
      Trimm_data.clear();
      break;
      }
    if ( len < 256)
    {
      isDatainQ = 1;
      Fetch_REQ_NMEASTR( );

    }

  }
}

void Mode_For_location()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println("MODE FOR LOCATION");
    BG95_Serial1.println(F("AT+QGPSLOC=2"));
    //vWaitResponseFromBG95_UART1("AT+QGPSLOC=2", "QGPSLOC");
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}

/**********************************************************************************************/
/*
   Function_name : UTC_to_EPOCH
   Description   : Here we are given with two parameters by split_Epoch() function, and UTC_to_EPOCH() will
                   do modification + calculation to get EPOCH_VALUE and send it to our called function
                   split_Epoch() function
*/
/*********************************************************************************************/
long UTC_to_EPOCH(char *i, char *j)
{
  Serial.println(i);
  Serial.println(j);
  i = i + 25; //+QGPSLOC: 052422.0
  /*
     AT+QGPSLOC=2
     +QGPSLOC: 054146.000

  */
  // 054558.0
  uint8_t pos = String(i).length();
  String(i).remove(pos - 1);

  Serial.print("Epoch after removing,"); Serial.println(i);
  char h[3] = {0}, s[3] = {0}, m[3] = {0}, m_day[3] = {0}, y[3] = {0}, mon[3] = {0};

  strncpy(h, i, 2);
  Serial.println(h);
  strncpy(m_day, j, 2);
  Serial.println(m_day);

  strncpy(m, i + 2, 2);
  Serial.println(m);
  strncpy(mon, j + 2, 2);
  Serial.println(mon);

  strncpy(s, i + 4, 2);
  Serial.println(s);
  strncpy(y, j + 4, 2);
  Serial.println(y);

  uint8_t hour = atoi(h);
  uint8_t minute = atoi(m);
  uint8_t seconds = atoi(s);

  uint8_t month = atoi(mon);
  uint8_t m_d = atoi(m_day);
  uint8_t y_d = atoi(y);

  char date[5] = "20";
  strcat(date, y);
  uint16_t year = atoi(date);

  Serial.println("Date details => ");

  Serial.println(m_d);
  Serial.println(month);
  Serial.println(year);

  Serial.println("Time details => ");

  Serial.println(h);
  Serial.println(minute);
  Serial.println(seconds);



  struct tm t;  // Initalize to all 0's
  t.tm_year = year - 1900; // This is year-1900, so 112 = 2012
  t.tm_mon = month - 1; //jan = 0 onwards
  t.tm_mday = m_d;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = seconds;
  t.tm_isdst = -1;
  time_t timeSinceEpoch = mktime(&t);

  Serial.println(timeSinceEpoch);
  return timeSinceEpoch;


}

/*************************************************************************************************/
/*
   Function_name : split_Epoch
   Description   : Generally the GPS_STRING that we get will have UTC+DATE_TIME_FORMAT that is
                   052422.0 = UTC and 270422 => 27 = date , 04 = month , 22 = year
                   So, In this particular Function I am separating the above mentioned particular variables
                   to sending them to UTC_to_EPOCH() function to finally get my EPOCH_VALUE
*/
/************************************************************************************************/
void split_Epoch(String str)
{
  uint8_t len = str.length();

  char sGgaDummyBuff[len + 1];
  strcpy(sGgaDummyBuff , str.c_str());
  char *token;
  char *i, *l;
  const char delimeter[] = ",";
  //const char delimeter1[] = "\r\n\r\nOK\r\n,";
  uint8_t index = 0;
  token = strtok(sGgaDummyBuff, delimeter);
  /*+QGPSGNMEA: $GPGGA,120654.00,1556.182918,N,08051.581390,E,1,06,0.7,6.5,M,-76.0,M,,*40
              0            1        2        3       4      5

    "+QGPSLOC: 052422.0,17.42739,78.43182,2.9,588.0,2,174.31,0.0,0.0,270422,03";*/
  while ( token != NULL ) {
    if (index == 0 )
    {
      i = token; // +QGPSLOC: 052422.0
      Serial.print("The UTC from GPS_STRING =>"); Serial.println(i);
    }
    else if ( index == 9)
    {
      j = token; //270422
      Serial.print("The Date_details =>"); Serial.println(j);
      atoi(j);
    }

    token = strtok(NULL, delimeter);
    index++;
  }
  long Epoch = UTC_to_EPOCH(i, j);
  Serial.print("Epochvalue "); Serial.println(Epoch);

  ts.tv_sec = Epoch;
  settimeofday(&ts, NULL);
  gettimeofday(&ts, NULL);
  gps_time[EpochTimeCounter] = ts.tv_sec;
  EpochTimeCounter++;
  gps_time[0] = Epoch ;

}



void GPSStrForEpoch()
{
  uint16_t start_time  = millis();
  /*
     Wait for 40 seconds to get EPOCH_VALUE from GPS STRINGS through UART1
  */
  while ( (millis() - start_time <= (FUNC_EXEC_TIME) * 1000 && ClkEnable == true) )
  {
    BG95_Serial_Clearbuf();
    BG95_Serial1.println(F("AT+QGPSLOC=2"));

    BG95_Serial_Read();
    if (strstr(cellular_Res, "+QGPSLOC: ") != NULL)
    {
      GpsStr = cellular_Res;       
      if (ClkEnable == true)
      {
        split_Epoch(cellular_Res);
        ClkEnable = false;                                                                             /* This boolean is used such that after execution of split_Epoch(), we dont once again just to sam eloop */
        EpochEnable = true;                                                                            /* This boolean is for telling that we got our Epoch_value already */
        break;
      }

    }
  }


  if (millis() - start_time > (FUNC_EXEC_TIME) * 1000 && ClkEnable == true && EpochEnable == false)
  {
    /*
       The program jumps here it means that we didn't get the EPOCH_VALUE from GPS_STRINGS
       within 5 seconds , So we need to change the priority and get our EPOCH value from Server that is by using UART1
    */
    //PriorityCheck();
    //delay(2000);
    Cell_modem_Priority();                                                              /* Giving priority to Cell-Modem */
    //delay(2000);
    PriorityCheck();
    EPOCH_from_server_AT();
    gettimeofday(&ts, NULL);
    gps_time[EpochTimeCounter] = ts.tv_sec;
    EpochTimeCounter++;
    ClkEnable = false ;
    EpochEnable = true;
    Serial.println("********************  we Got EPOCH_VALUE!!! ************************");
    Serial.println("------->  Switching the Priority to GNSS_MODE");


  }
  vTaskDelay(10);
}


/**************************************************************************************
   funciton name  ：vgetSmac
   parameter      ：Void
   return value   ：Void
   effect         ：Getting SMAC value of DS18B20
*****************************************************************************************/

void  vgetSmac()
{

 while (!DS18B20.search(addr))
  {
    Serial.println("waiting to connect");
    DS18B20.reset_search();
    delay(500);
  }
  for (byte i = 0; i < 8; i++) {
    smac  += String(addr[i], HEX);

  }
  Serial.print("The SMAC value is:");
  Serial.print(smac);
  Serial.print(", ");  
}
/**************************************************************************************
   funciton name  ：vSensorReadings
   parameter      ：Void
   return value   ：Void
   effect         ：Getting Temperature value from DS18B20
*****************************************************************************************/
void  vSensorReadings()
{
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);
  float temp = (temperatureC + 273.15) * 10;                                            /* Here we are converting the temperature values into Kelvin */
  C_Temp = round(temp);                                                                 /* Rounding-off to an integer */
  Serial.print("temp = ");
  Serial.println(C_Temp);
}

void AccReadings(){
   
   // Read the raw sensor data count
    MC3672_acc_t rawAccel = MC3672_acc.readRawAccel();
    delay(10);

    float accelerometerData[3];
    
    accelerometerData[0] = rawAccel.XAxis_g;
    accelerometerData[1] = rawAccel.YAxis_g;
    accelerometerData[2] = rawAccel.ZAxis_g;

    // Initialize variables for min, max, and sum
    float minX = accelerometerData[0];
    float minY = accelerometerData[1];
    float minZ = accelerometerData[2];
    float maxX = accelerometerData[0];
    float maxY = accelerometerData[1];
    float maxZ = accelerometerData[2];
    float sumX = accelerometerData[0];
    float sumY = accelerometerData[1];
    float sumZ = accelerometerData[2];

    // Calculate min, max, and sum for X, Y, and Z axes
    for (int i = 1; i < 3; i++) {
        if (accelerometerData[i] < minX) {
            minX = accelerometerData[i];
        }
        if (accelerometerData[i] > maxX) {
            maxX = accelerometerData[i];
        }
        sumX += accelerometerData[i];
    }
    
    for (int i = 1; i < 3; i++) {
        if (accelerometerData[i] < minY) {
            minY = accelerometerData[i];
        }
        if (accelerometerData[i] > maxY) {
            maxY = accelerometerData[i];
        }
        sumY += accelerometerData[i];
    }
    
    for (int i = 1; i < 3; i++) {
        if (accelerometerData[i] < minZ) {
            minZ = accelerometerData[i];
        }
        if (accelerometerData[i] > maxZ) {
            maxZ = accelerometerData[i];
        }
        sumZ += accelerometerData[i];
    }

    // Calculate averages
    float avgX = sumX / 3;
    float avgY = sumY / 3;
    float avgZ = sumZ / 3;

    // Convert the values to strings
          minXStr = String(minX);
          maxXStr = String(maxX);
          avgXStr = String(avgX);
          minYStr = String(minY);
          maxYStr = String(maxY);
          avgYStr = String(avgY);
          minZStr = String(minZ);
          maxZStr = String(maxZ);
          avgZStr = String(avgZ);

    // Display the results
    Serial.println("Accelerometer Data:");
    Serial.print("X: Min = ");
    Serial.print(minXStr);
    Serial.print(", Max = ");
    Serial.print(maxXStr);
    Serial.print(", Avg = ");
    Serial.println(avgXStr);

    Serial.print("Y: Min = ");
    Serial.print(minYStr);
    Serial.print(", Max = ");
    Serial.print(maxYStr);
    Serial.print(", Avg = ");
    Serial.println(avgYStr);

    Serial.print("Z: Min = ");
    Serial.print(minZStr);
    Serial.print(", Max = ");
    Serial.print(maxZStr);
    Serial.print(", Avg = ");
    Serial.println(avgZStr);

   


}
void  Activate_GPS()
{
   while (1)
  {
    BG95_Serial_Clearbuf();
    BG95_Serial1.println(F("AT+QGPS=1"));
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {       /* if return value is "OK" than program delay 1second jump out loop */
      delay(1000);
       break;
    }
}
}

void GPS_ON()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    BG95_Serial1.println(F("AT+QGPS?"));
    BG95_Serial_Read();
    if (strstr(cellular_Res, "+QGPS: 1") != NULL) {       /* if return value is "OK" than program delay 1second jump out loop */
      delay(1000);
       break;
    }
    else
    {
      Activate_GPS();
    }
  }
}

void GPS_OFF()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    BG95_Serial1.println(F("AT+QGPSEND"));// 31 => Enables all the NMEA strings, We can change this according to the need
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(1000);//GSA+GGA
      
      break;
    }
  }
}

void Query_PIN_Status()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    //Serial.println(F("AT+CPIN?"));
    BG95_Serial1.println("AT+CPIN?");
    BG95_Serial_Read( );
    if (strstr(Response,"OK") != NULL) {      /* if return value is "OK" than program delay 1second jump out loop */
      delay(1000);
      break;
    }
  }
}


void SIM_Card_Detection()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    //Serial.println(F("AT+QSIMDET"));
    BG95_Serial1.println("AT+QSIMDET?");
    BG95_Serial_Read( );
    if (strstr(Response,"OK") != NULL) {      /* if return value is "OK" than program delay 1second jump out loop */
      delay(1000);
      break;
    }
  }
}

/*

void checkRange()
{
  switch(MC3672_acc.GetRangeCtrl())
  {
    case MC3672_RANGE_16G:                Serial.println("Range: +/- 16 g"); break;
    case MC3672_RANGE_12G:                Serial.println("Range: +/- 12 g"); break;
    case MC3672_RANGE_8G:                 Serial.println("Range: +/- 8 g"); break;
    case MC3672_RANGE_4G:                 Serial.println("Range: +/- 4 g"); break;
    case MC3672_RANGE_2G:                 Serial.println("Range: +/- 2 g"); break;
    default:                              Serial.println("Range: +/- 8 g"); break;
  }
}


void checkResolution()
{
  switch(MC3672_acc.GetResolutionCtrl())
  {
    case MC3672_RESOLUTION_6BIT:            Serial.println("Resolution: 6bit"); break;
    case MC3672_RESOLUTION_7BIT:            Serial.println("Resolution: 7bit"); break;
    case MC3672_RESOLUTION_8BIT:            Serial.println("Resolution: 8bit"); break;
    case MC3672_RESOLUTION_10BIT:           Serial.println("Resolution: 10bit"); break;
    case MC3672_RESOLUTION_14BIT:           Serial.println("Resolution: 14bit"); break;
    case MC3672_RESOLUTION_12BIT:           Serial.println("Resolution: 12bit"); break;
    default:                                Serial.println("Resolution: 14bit"); break;
  }
}


void checkSamplingRate()
{
  Serial.println("Low Power Mode"); 
  switch(MC3672_acc.GetCWakeSampleRate())
  {
    
    MC3672_CWAKE_SR_DEFAULT_50Hz:     Serial.println("Output Sampling Rate: 54 Hz"); break;
    MC3672_CWAKE_SR_14Hz:                 Serial.println("Output Sampling Rate: 14 Hz"); break;
    MC3672_CWAKE_SR_28Hz:             Serial.println("Output Sampling Rate: 28 Hz"); break;
    MC3672_CWAKE_SR_54Hz:                   Serial.println("Output Sampling Rate: 54 Hz"); break;
    MC3672_CWAKE_SR_105Hz:                  Serial.println("Output Sampling Rate: 105 Hz"); break;
    MC3672_CWAKE_SR_210Hz:                  Serial.println("Output Sampling Rate: 210 Hz"); break;
    MC3672_CWAKE_SR_400Hz:                  Serial.println("Output Sampling Rate: 400 Hz"); break;
    MC3672_CWAKE_SR_600Hz:                  Serial.println("Output Sampling Rate: 600 Hz"); break;  
    default:                                Serial.println("Output Sampling Rate: 54 Hz"); break;
  
  }
}

*/
void Check()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println("GeneralCheck");
    BG95_Serial1.println(F("ATI"));
    //vWaitResponseFromBG95_UART1("AT+QGPSLOC=2", "QGPSLOC");
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}


void NmeaOutputPort()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println("NMEAOPPORT");
    BG95_Serial1.println(F("AT+QGPSCFG=\"outport\",\"uartnmea\",115200"));
    //vWaitResponseFromBG95_UART1("AT+QGPSLOC=2", "QGPSLOC");
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}


void EnableNmeaAcq()
{
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println("NMEAOPPORT");
    BG95_Serial1.println(F("AT+QGPSCFG=\"nmeasrc\",1"));
    //vWaitResponseFromBG95_UART1("AT+QGPSLOC=2", "QGPSLOC");
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}


void GPSNmeaType()
{ //AT+QGPSCFG=\"gpsnmeatype\",31
  while (1)
  {
    BG95_Serial_Clearbuf();
    Serial.println("GPSNmeaType");
    BG95_Serial1.println(F("AT+QGPSCFG=\"gpsnmeatype\",17"));
    BG95_Serial_Read();
    if (strstr(cellular_Res, "OK") != NULL) {     /* if return value is "OK" than program delay 1second jump out loop */
      delay(80);
      break;
    }
  }
}
/********************************* IMPLEMENTATION OF Task1 **********************************************************/

/*
   Function_name : Task1
   Description: In this TASK, we will first configure CLock using either cell-modem connection or GPS connection 
   and then, we will add GPS_STR's until it hits the time specifed ( FUNC_EXEC_TIME = 20*100 = 2000 = 2sec )and send those strings to Queue
*/
void task1(void*arg)
{
 esp_task_wdt_init(30, false);
 SPIFFS.format();
  /*Clock Configuration */
 //vgetSmac();
  AccReadings();
 vSensorReadings();
 //AccReadings();
  Serial.println("Entry into TASK1");
 // Query_PIN_Status();
  //SIM_Card_Detection();
  GPS_ON();
  Serial.println("Entry into after GPS_ON");
  //+QHTTPGET: 714 => No data service
  if (ClkEnable == true)
  {
   Serial.println("Configuring the clock...");
   /*
    Priority check needs to be done but by default it will be in GNSS priority only*/
    PriorityCheck();
    GNSSPriority();                                                                 /* Giving priority to GNSS*/
    PriorityCheck();
    GPSStrForEpoch();
  }
  /*
     Changing the priority to GNSS
  */

  GNSSPriority();                                                                 /* Giving priority to GNSS */
  PriorityCheck();

  Serial.println("______________SETUP_COMPLETE____________________");
  Serial.println();
  
  /*Check();
  NmeaOutputPort(); 
  EnableNmeaAcq(); 
  
  GPSNmeaType();*/
  Mode_For_location();
 
  while (1)
  {
   
    if (QueueActivate == false )
    {
      /* Adding Epoch values to the Epoch_Array */
      gettimeofday(&ts, NULL);
      gps_time[EpochTimeCounter] = ts.tv_sec;
      EpochTimeCounter++;

      //printf("EpochTimeCounter value is %d", EpochTimeCounter);
      //printf("And its value is %d \n",  gps_time[EpochTimeCounter] );

      /******************************************/

      Fetch_REQ_NMEASTR();
      GPS_OFF();
      
      
    }
    else
    {
    vTaskDelay(700);
   }
  }
}

void createSPIFFSfor_Temp()
{
  yield();
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  else {
    
     Serial.println("SUCCESS");
  
  }

  Filename_with_ext1 = "/data/" + String("TempValues") + ".DAT";
  File SPIFFS_file = SPIFFS.open(Filename_with_ext1, "w");

  /* writing to the SPIFFS files */

  if (!SPIFFS_file)
  {

    Serial.println("SPIFFs file open failure");
    return;
  }
  else
  {
    Serial.println("Writing " + Filename_with_ext1 + " to SPIFFS");
    SPIFFS_file.print(Temp_data[tempCount]);
    Serial.print("The values that are added =>");
    Serial.println(Temp_data[tempCount]);

    Serial.print(tempCount);//just for testing purpose
    SPIFFS_file.close();
  }
}


void task4(void*arg)
{
  while(1)
  {
   //  Serial.println("We are in TASK4");
   while (millis() - start_tempTime >= (TEMP_FETCH_TIME) * 1000  && ClkEnable == false) 
    {
     vSensorReadings();
     createSPIFFSfor_Temp();
     Serial.println("Temperature values are added");
     start_tempTime =  millis();
     }
    }
  }

/***********************************************************************************************/
/************************************** Http_postfor_Postingdata() ****************************************/
/*
 * Description ::
 * - used to post the data using AT commands
 */
void Http_postfor_Postingdata()
{
  while (1)
  {
    char buf_1[DataSize];
    BG95_Serial_Clearbuf();
    sprintf(buf_1, "AT+QHTTPPOST=%d,%d,%d", DataSize, 80, 80);                                                                     /* creating the command using sprintf function as the datasize is dynamic in nature */
    Serial.println(buf_1);                                                                                                                   
    BG95_Serial1.println(buf_1);                                                                                                   /* Writing the AT Command from ESP32 to BG95 */                                                                                                                                                          
    BG95_Serial_Read( );                                                                                                           /* Reading the response from BG95 and storing that into an array (Response) */                                                                                                                          
    if (strstr(cellular_Res, "CONNECT") ) {                                                                                        /* Until and unless the "OK" response is appeared, it will repeat the command */                        
      BG95_Serial1.println(requestBody);                                                                                           /* After getting CONNECT response from BG95, Then you have to push your JSON_PACKET which is stored in requestBody */                       
      delay(80);                                                                                                                   /* NOTE : you can decrease the delay(1000),upto delay(80) and check */
      break;
    }

  }
}


/*
   HTTP_AT commands for Posting JSON to server
*/
void Http_commands()
{
 // If you are getting +CME ERROR: 707 while posting the JSON_PACKET then ask hemasri once
 // since test4db may get updated from server side, and after this updation, it needs a restart ( restartDB), So after confirming from her proceed
  Contenttypefor_Postingdata();
  //Request_headerfor_Postingdata();
  //Response_headerfor_Postingdata();
  Http_URLfor_Postingdata();
  Http_postfor_Postingdata();
  Http_readfor_Postingdata();


}
/*********************************************/




/*
   Task3 will take SPIFFS file and send to the server and wait for 200 response code for each SPIFFS file send
*/

void task3(void*arg)
{
 
  while (1)
  {
   
    //QueueActivate = false ; // Making QueueActivate = false means task1 back to running state
    //isDatainQ = true; 
    //loop1 = true;  
    
    if ( enableTask3 == true )
    {
      printf("TASK3 started");
     // if(spiffsFileCounter >= 2)
      //while(spiffsFileCounter <= 2 && spiffsFileCounter >= 1 )
      while(spiffsFileCounter == 1 )
      {
       QueueActivate = true; 
     
      Serial.println("Entry into TASK3");

      File Fname = SPIFFS.open("/cache");
       File acName = SPIFFS.open("/accelerometer");  // file opening for acclerometeer data . we are taking the data of max (x ,y, z)
      File file = Fname.openNextFile();
      file_name = file.name();
      Serial.print("The file name is ");
      Serial.println(file_name);
      Readfile(file_name);
     
      /* Priority switch */
      if(spiffsFileCounter == 1)
      {
      //PriorityCheck();
      Cell_modem_Priority();
      PriorityCheck();
      }
      /*******************/
      Http_commands();
      spiffsFileCounter = vLoadSPIFFsDirectory();
      printf("SpiffsCount => %d", spiffsFileCounter);
      if( spiffsFileCounter == 0 )
      {
        Serial.println("***********All the SPIFFS Files are deleted************ \n !!!!!Activating GPS_MODE!!!!!!");
        Serial.print("QueueActivate boolean value is => "); 
        Serial.println(QueueActivate);
        GNSSPriority();                                                                 /* Giving priority to GNSS*/
        PriorityCheck();
        GPS_ON();
        Mode_For_location();
        QueueActivate = false;
        enableTask3 = false;
       }
      }
    }
    
    else
    {
     vTaskDelay(5000);
  
    }
  }
}
/*
   Creating SPIFFS file
*/

void createfilename()
{
  yield();
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  else {
    Serial.println("SUCCESS");
  }

  Filename_with_ext = "/cache/" + String(gps_time[EpochTimeCounter]) + ".DAT";  //  /cache/1092638321.DAT
  File SPIFFS_file = SPIFFS.open(Filename_with_ext, "w");

  /* writing to the SPIFFS files */

  if (!SPIFFS_file)
  {

    Serial.println("SPIFFs file open failure");
    return;
  }
  else
  {
    Serial.println("Writing " + Filename_with_ext + " to SPIFFS");
    SPIFFS_file.print(requestBody);
    SPIFFS_file.close();
  }

}

/*
   Creating JSON using required strings
*/

void JsonCreate()
{
  uint8_t ulog ;
  DynamicJsonDocument doc(2048);
  JsonObject root    = doc.to<JsonObject>();
  root["v"]          = 1;
  root["pw"]         = "abc";
  root["ts"]         = gps_time[0];
  root["smac"]       = smac;//smac;
  //JsonObject erl     = root.createNestedObject("erl");
  JsonArray erl     = root.createNestedArray("erl");
  JsonObject tmp_data = erl.createNestedObject();
  tmp_data["ts"]          = gps_time[0];
  tmp_data["kt"]          = C_Temp;
  tmp_data["sq"]          = 5;

  //Json object Creating for acclerometer sensor

  JsonArray aclero     = root.createNestedArray("Acl");
  JsonObject acm_data = aclero.createNestedObject();

   acm_data["Xmin"]         = minXStr;
   acm_data["Xmax"]         = maxXStr;
   acm_data["Xavg"]         = avgXStr;
   acm_data["Ymin"]         = minXStr;
   acm_data["Ymax"]         = maxXStr;
   acm_data["Yavg"]         = avgXStr;
   acm_data["Zmin"]         = minXStr;
   acm_data["Zmax"]         = maxXStr;
   acm_data["Zavg"]         = avgXStr;
   acm_data["smac"]         = smac;      
  acm_data ["ts"]           = gps_time[0];
   // Json For GPS
  JsonArray gps      = root.createNestedArray("gps");
  Serial.print("In Jsoncreate func Taskcount is=>"); Serial.println(u8LogSeq);
  for (ulog  = 0; ulog < u8LogSeq; ulog++)                                             // This for loop is for taking each string from the array of string
  {
    Serial.print("destarr is => "); Serial.println(dest_arr[ulog]);
    char loc_buff[dest_arr[ulog].length() + 1];
    memset(loc_buff, 0, sizeof(loc_buff));
    dest_arr[ulog].toCharArray(loc_buff, dest_arr[ulog].length() + 1);
    char *token2 = strtok(loc_buff, ",");
    JsonObject gps_data = gps.createNestedObject();
    gps_data["ts"] = gps_time[ulog];
    //17.427811,N,78.432030,E,03,548.5,GNVTG,0.0
   // Serial.println("Token2 loop in JSON_create");
    //Serial.println("token2 value is=>");Serial.println( token2 );
    while ( token2 != NULL )
    {
      for (uint8_t i = 0; i < 9; i++) //This Forloop is for taking required parameter by taking each string
      {
        if(i == 0 )
        {
         gps_data["utc"] = token2;
        }

        else if (i == 1) {
          gps_data["lat"]  = atof(token2);


        }
        else if (i == 2) {
          gps_data["d1"]  = token2;


        }
        else if (i == 3) {
          gps_data["lng"] = atof(token2);

        }
        else if (i == 4) {
          gps_data["d2"] = token2;

        }
        else if ( i == 5) {
          gps_data["sat"] = atoi(token2);

        }
        else if ( i == 6) {
          gps_data["alt"] = atof(token2);

        }
        else if ( i == 7)
        {
          gps_data["cog"] = atof(token2);
        }
        else if (i == 8)
        {
          gps_data["spd"] = atof(token2);
        }

        token2 = strtok(NULL, ",");

      }
    }
    //ulog--;

  }
  
  dest_arr[u8LogSeq].clear();
  u8LogSeq = 0;
  Serial.print("After Clearing the dest array value is ");Serial.println(dest_arr[u8LogSeq]);
  Serial.println();
  Serial.println("***********************************************");
  //serializeJson(doc, Serial);
  EpochTimeCounter = 0;
  memset(requestBody, '\0', sizeof(requestBody));
  DataSize = serializeJsonPretty(doc, requestBody);
  Serial.print("Request Body\t:");
  Serial.println(requestBody);
  Serial.println("************************************************");
  Serial.print("\t the sizeof JSON is:\t"); Serial.println(DataSize);

  createfilename();
  createAccelerometerFile();


    QueueActivate = false ; // Making QueueActivate = false means task1 back to running state
    isDatainQ = true; 
    loop1 = true;  
 

   spiffsFileCounter = vLoadSPIFFsDirectory();
    vSortDirectory();
    if( spiffsFileCounter == 1)
    {
    QueueActivate = true; // Making QueueActivate = false means task1 back to running state
    isDatainQ = true; 
    loop1 = true;
    enableTask3 = true;
    }

}


String createReqLatLong(double token)
{
  float lReal;
  //Serial.println("The token value is "); Serial.println(token);
  int l = round((int)token / 100);
  //printf("Now 1700 is %d\t", l);

  token = token - l * 100;
  //printf("%f\n", token);

  double val1 = token - (int)(token);
  //printf("%f\n", val1);

  lReal = l + (token / 60) + (val1 / 3600);
  //printf("The Required Latitude value is %f\n", lReal);
  L_STR = String(lReal, 6);

  return L_STR;//String(lReal);

}

/*
   Here we fetch variables from NMEA_STRINGS and create  strings which contains required variables for JSON
   that is converting
   GGA_STR :: GNGGA,060926.00,1725.650622,N,07825.905661,E,1,02,18.1,568.8,M,-73.3,M,,*6B
   VTG_STR :: GNVTG,,T,,M,0.0,N,0.0,K,A*3D
   to
   Desired GPS_STR Format
   17.427691,N,78.432014,E,02,568.8,GNVTG,0.0,
*/
// creating Acclerometere data file for storing the value
void createAccelerometerFile()
{
  yield();
  
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  else {
    Serial.println("SUCCESS");
  }

  String Filename_with_ext2 = "/accelerometer/" + String(gps_time[EpochTimeCounter]) + ".DAT"; // Change the directory name
  
  File spiffsFile = SPIFFS.open(Filename_with_ext2, "w"); 

  if (!spiffsFile)
  {
    Serial.println("SPIFFS file open failure");
    return;
  }
  else
  {
    Serial.println("Writing " + Filename_with_ext2 + " to SPIFFS");

    // Assuming accelerometerData is a string or formatted data
    String accelerometerData = "Xmax: " + String(maxXStr) + " Ymax: " + String(maxYStr) + " Zmax: " + String(maxZStr);
    
    spiffsFile.print(accelerometerData);
    spiffsFile.close();

  }
}

void creatReqStrings(String GPVTG_STR, String GNGGA_STR)
{
  char  *terminator3, *terminator4;
  const char code2[] = ",";
  const char code3[] = "\r\n";
  int index2 = 0;
  Serial.println("___________Each New GGA_VTG STRINGS______________");
  Serial.println(GNGGA_STR);
  Serial.println(GPVTG_STR);
  uint16_t l1 = GPVTG_STR.length(); 
  char sGpsDummyBuff1[l1 + 1];
  strcpy(sGpsDummyBuff1 , GPVTG_STR.c_str()); // convert from string to character array
  uint16_t l2 =  GNGGA_STR.length();
  char sGpsDummyBuff2[l2 + 1];
  strcpy(sGpsDummyBuff2, GNGGA_STR.c_str());

  /*if((strstr(sGpsDummyBuff2,"GNGGA") != NULL ) && (strstr(sGpsDummyBuff1,"GNVTG")) != NULL)
  {
     Fetch_REQ_NMEASTR();
  }*/

  //GGA_STRING parsing
  terminator2 = strtok(sGpsDummyBuff1, code2);
  dest_arr[u8LogSeq].clear();
  if (loop1 == true)
  {
    while (terminator2 != NULL )
    {
      /* Adding only to check all the gps working properly */
      if(index2 == 1 )
      {
        String utc = terminator2;
        dest_arr[u8LogSeq].concat(utc );
        dest_arr[u8LogSeq].concat(",");
      }
      else if (   index2 == 2 ||  index2 == 4  )  // modifying lat and long
      {
        
        String t = createReqLatLong(atof(terminator2));
        Serial.println("modified Lat and long values "); Serial.println(t);
        dest_arr[u8LogSeq].concat(t );
        dest_arr[u8LogSeq].concat(",");
      }
      else if (index2 == 3 || index2 == 5 || index2 == 7 || index2 == 9  ) // Getting all other required variables (Number of satellites, DIR1,DIR2,Altitude)
      {
        dest_arr[u8LogSeq].concat( terminator2 );
        dest_arr[u8LogSeq].concat(",");
      }
      terminator2 = strtok(NULL, code2);
      index2++;
    }
    // VTG_STRING parsing
    //GPVTG,126.0,T,126.0,M,4.8,N,8.9,K,A*2E

    //GPVTG,,T,,M,,N,,K,A*23
    // GPVTG,,T,,M,0.0,N,0.0,K,A*23


    terminator3 = strtok(sGpsDummyBuff2, code2);
    index2 = 0;
    while (terminator3 != NULL)
    {
      if (   index2 == 1 ||  index2 == 7 )  //VTG string parsing getting the values of COG and speed in km/hr
      {
        if(strstr(terminator3," ") != NULL)   //This if statement is added since sometimes
        {                                     //the GPVTG is => GPVTG,,T,,M,0.0,N,0.0,K,A*23
           dest_arr[u8LogSeq].concat( "0.0" );// and sometimes its => GPVTG,,T,,M,,N,,K,A*23 and we are accessing the element before 'K' which is speed
        }                                     // whenever it is 0.0 everything is normal, But if there is no value..its giving Guru Meditation and the ESP32 is resetting to avoid that this IF statement is added
        else
        {
        dest_arr[u8LogSeq].concat( terminator3 );
        dest_arr[u8LogSeq].concat(",");
        //Serial.print("terminator3..VTG string is=>"); Serial.println(terminator3 );
        }
      }
      terminator3 = strtok(NULL, code2);
      index2++;
    }
   
    Serial.print("&&&&&&&&&&\nThe Desired Dest_Arr is=> ");Serial.println( dest_arr[u8LogSeq] );
    
    char gps_info[dest_arr[u8LogSeq].length() + 1];
    dest_arr[u8LogSeq].toCharArray(gps_info, dest_arr[u8LogSeq].length() + 1);
    
    terminator4 = strtok(gps_info, code3);


    // 17.427811,N,78.432030,E,03,548.5,GNVTG,0.0,\r\n
    Trimm_data.clear();
    while (terminator4 != NULL ) { //&& limitorcount <= 8
      Trimm_data.concat(terminator4);
      terminator4 = strtok(NULL, code3);
    }
    Trimm_data.concat('\0');
    Serial.print("Trimm data is "); Serial.println(Trimm_data);
    loop1 = false;
  }
}

/*
   Description: here we take raw_strings from queue and form the  strings which contains the required parameters inorder to create JSON
   The RAW_GPS_STRINGS are 
   VTG :  GPVTG,126.0,T,126.0,M,4.8,N,8.9,K,A*2E
   GGA :  GNGGA,060926.00,1725.650622,N,07825.905661,E,1,02,18.1,568.8,M,-73.3,M,,*6B
   These strings after sending to creatReqStrings(receive_buff1[strInQ], receive_buff2[strInQ]);
   will be get converted to
   REQUIRED_GPS_STRING :
   17.427691,N,78.432014,E,02,568.8,126.0,8.9,
   which contains
   Latitude, Dir1, Longitude, Dir2, Number of satellites, Altitude, Cog(Course Over Ground), Speed (Km/hr)

*/
void task2(void*z)
{
 String GGA_STR;
 String VTG_STR, value;
 uint16_t strInQ=1;
 
 while (1)
  {
     //JsonCount =  TaskCount ; 
    if(TaskCount >= 1) // && QueueActivate == true 
     {
       if (!intQueue.isEmpty()) 
       {
          printf("The Array count before the Queue value addition is %d\n", u8LogSeq);
          dest_arr[u8LogSeq] = intQueue.dequeue();
          dest_arr[u8LogSeq].concat('\0');
          printf("Destination_Array value is %s\n", dest_arr[u8LogSeq] );
          u8LogSeq++;
          printf("\nThe Array Count value is %d\n",u8LogSeq);
          QueueActivate = false ;  //To activate Task1 as we need to fill upto 2 strings
          isDatainQ = true; // To Activate Req_NMEA_STR function to get the GGA and VTG
           loop1 = true;    // To activate FORMAT_STR to making the GGA+VTG into One Required Format
          if(u8LogSeq == 1)//2 strings we need
           {
             //GPS_OFF(); after making GPS_OFF...once again making GPS_OFF will give you +CME ERROR: 505
            if( intQueue.isEmpty() )
            {
              Serial.print("Queue is empty now!!!");
            }
            else
            {
              Serial.println("Data is present in the Queue");
            }
            QueueActivate = true ;  // Making QueueActivate = true such that TASK1 is not in running state
            JsonCreate();
            }
        }
        
     }
     else
     {
      vTaskDelay(800);
     }

  }
}

void setup()
{
  Serial.begin(9600);
  //SPIFFS.format();
  MC3672_acc.start();  // this funtion for start the acclerometr sensor readings 
  pinMode(RXD2, OUTPUT);
  pinMode(TXD2, OUTPUT);

  digitalWrite(TXD2, LOW);
  digitalWrite(RXD2, LOW);

  Serial.println("Receiving pin reading=>");
  Serial.println(digitalRead(RXD2));

  Serial.println("Transmitting pin reading=>");
  Serial.println(digitalRead(TXD2));
  
  delay(50);
  
  pinMode(C_MODEM_ENABLE, OUTPUT);
  digitalWrite(C_MODEM_ENABLE, LOW);
  delay(30);
  digitalWrite(C_MODEM_ENABLE, HIGH );
  delay(500);
  digitalWrite(C_MODEM_ENABLE, LOW );

  pinMode(TXD2, OUTPUT);
  pinMode(RXD2, INPUT);

  digitalWrite(TXD2, LOW);
  digitalWrite(RXD2, LOW);

  pinMode(TXD1, OUTPUT);
  pinMode(RXD1, INPUT);
  digitalWrite(TXD1, LOW);
  digitalWrite(RXD1, LOW);

  BG95_Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1); //GPS
  while (!BG95_Serial1)
  {
    Serial.println("BG95 is not initialized");
    Serial.println("Please Check Connections");
  }
  BG95_Serial1.flush();

  BG95_Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);//Cell_modem
  while (!BG95_Serial2)
  {
    Serial.println("BG95 is not initialized");
    Serial.println("Please Check Connections");
  }
  BG95_Serial2.flush();

  delay(1000);
  xTaskCreate(task1, "Task1", 5000, NULL, 2, &Task1);
  xTaskCreate(task2, "Task2", 90000, NULL, 1, &Task2);//2
  xTaskCreate(task3, "Task3", 5000, NULL, 1, &Task3);
  //xTaskCreate(task4, "Task4" ,2500, NULL, 1, &Task4);

}


void loop()
{

}
