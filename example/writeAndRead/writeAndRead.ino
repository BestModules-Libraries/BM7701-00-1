/*************************************************
  File:             write.ino
  Description:      The module is connected to the mobile phone APP "BLEDemo", press the key on the APP, 
                    the module receives the key and prints it to the serial port monitor, 
                    and then sends the corresponding data to the APP, making the LED on the APP light up
  Note:
  Operation:
**************************************************/

#include <BM7701-00-1.h>
//BM7701_00_1       BC7701(2, 3); //rxPin,txPin,Please comment out this line of code if you don't use SW Serial
BM7701_00_1     BC7701(&Serial1); //Please uncomment out this line of code if you use HW Serial1 on BMduino
//BM7701_00_1     BC7701(&Serial2); //Please uncomment out this line of code if you use HW Serial2 on BMduino
//BM7701_00_1     BC7701(&Serial3); //Please uncomment out this line of code if you use HW Serial3 on BMduino
//BM7701_00_1     BC7701(&Serial4); //Please uncomment out this line of code if you use HW Serial4 on BMduino

//Define BLE
#define TX_POWER     0x0F                   //TX Power
#define XTAL_CLOAD   0x04                   //16MHz Crystal load
#define ADV_MIN      100                    //AdvIntv min=100ms
#define ADV_MAX      100                    //AdvIntv max=100ms
#define CON_MIN      30                     //ConIntv min=30ms
#define CON_MAX      30                     //ConIntv max=30ms
#define CON_LATENCY  00                     //ConIntv latency=00
#define CON_TIMEOUT  300                    //ConIntv timeout=300 
uint8_t BDAddress[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};                            //Device address
uint8_t BDName[] = {'B', 'M', 'C', '7', '7', 'M', '0', '0', '1'};                       //Device name
uint8_t Adata[] = {0x02, 0x01, 0x06}; //advertising data
uint8_t Sdata[] = {0x03, 0x02, 0x0f, 0x18};     //scan response data


/////////////////////////////////////////////////////////////////////////////////////////

#define BUTTON_CONSISTENCY_DURATION    6
#define BUTTON_REPEAT1_DURATION       (600 / BUTTON_CONSISTENCY_DURATION)
#define BUTTON_REPEAT2_DURATION       (150 / BUTTON_CONSISTENCY_DURATION)
#define INVERT_TIME                   500
bool board_connect = false;
bool board_receive = false;
bool board_conIntv = false;
uint8_t Status;         //BLE status
uint8_t flag=0;
uint8_t count=0;
uint8_t sel = 1;
uint8_t receiveBuf[256] = {0};
KEY_MESSAGE Keymessage;
void setup() {
  delay(60);//delay 60ms after power on reset,can not send CMD in this peried
  Serial.begin(9600);
  BC7701.begin(BAUD_115200);                
  while (sel != 10)
  {
    switch (sel)
    {
      case 1: if (BC7701.setAddress(BDAddress) == true) sel++;                        //Set address
        else sel = 0xFF;  break;
      case 2: if (BC7701.setName(sizeof(BDName), BDName) == true) sel++;              //Set Bluetooth Name  (max:31)
        else sel = 0xFF; break;
      case 3: if (BC7701.setAdvIntv(ADV_MIN / 0.625, ADV_MAX / 0.625, 7) == true) sel++; //Set AdvIntv
        else sel = 0xFF; break;
      case 4: if (BC7701.setAdvData(APPEND_NAME, sizeof(Adata), Adata) == true) sel++; //Set AdvData
        else sel = 0xFF; break;
      case 5: if (BC7701.setScanData(sizeof(Sdata), Sdata) == true) sel++;            //Set ScanData
        else sel = 0xFF; break;
      case 6: if (BC7701.setTXpower(TX_POWER) == true) sel++;                         //Set TX power
        else sel = 0xFF; break;
      case 7: if (BC7701.setCrystalOffset(XTAL_CLOAD) == true) sel++;                     //Set Xtal Cload
        else sel = 0xFF; break;
      case 8: if (BC7701.setFeature(FEATURE_DIR, AUTO_SEND_SATUS) == true) sel++;     //Auto Send Satus
        else sel = 0xFF; break;
      case 9: if (BC7701.setAdvCtrl(ENABLE) == true) sel++;                  //Turn on broadcast
        else sel = 0xFF;
        break;
      case 0xFF: digitalWrite(13,HIGH);
        break;//Configure fail
    }
  }
  delay(650);                                            //delay 650ms after Turn on broadcast,can not send CMD in this peried
}
void loop()
{
  count=0; 
  
  Status = bleProcess();                                   //Read BLE status
  if (Status)
  {
    switch (Status)
    {
      case API_CONNECTED:
        if (board_connect == false)
        {
          board_connect = true;
          board_receive = false;
        }
        break;
        
      case API_DISCONNECTED:
        board_connect = false;
        board_receive = false;
        board_conIntv = false;
        break;
      case DATA_RECEIVED:
        if (board_connect == true)
        {
          digitalWrite(13,LOW);
          board_receive = true;
        }
        break;
      case API_ERROR:
        break;
    }
  }
  if (board_connect == true)
  {
    if (board_conIntv == false)
    {
      BC7701.wakeUp();
      delay(30);
      if (BC7701.setConnIntv(CON_MIN / 1.25, CON_MAX / 1.25, CON_LATENCY, CON_TIMEOUT) == true) //Set ConIntv
      board_conIntv = true;
    }
    if (board_receive == true)
    {
      board_receive = false;
      if (receiveBuf[3] == 0xB0)
      {
        switch (receiveBuf[4])
        {
          case 0x11:
            count=1;
            Serial.println("KEY1 icon Pushed");
            break;
          case 0x10:
            count=2; 
            Serial.println("KEY1 icon Released");
            break;
          case 0x22:
            count=1;
            Serial.println("KEY2 icon Pushed");
            break;
          case 0x20:
            count=2;
            Serial.println("KEY2 icon Released");
            break;
          case 0x44:
            count=1;
            Serial.println("KEY3 icon Pushed");
            break;
          case 0x40:
            count=2; 
            Serial.println("KEY3 icon Released");
            break;
        }
          if (receiveBuf[4]!=0 && count==2&& flag==0)
          {   
              Keymessage.key = receiveBuf[4]>>4;
              Keymessage.key += receiveBuf[4];
              Keymessage.serial ++;
              Keymessage.checksum = 0xB1 ^ Keymessage.key ^ Keymessage.serial;
              BC7701.writeData((uint8_t*)&Keymessage,3);
              flag=1;
              receiveBuf[4] = 0;
          }
          if (receiveBuf[4]!=0&&count==2&&flag==1)
          {
              Keymessage.key = receiveBuf[4];
              Keymessage.serial ++;
              Keymessage.checksum = 0xB1 ^ Keymessage.key ^ Keymessage.serial;
              BC7701.writeData((uint8_t*)&Keymessage,3);   
              flag=0;  
              receiveBuf[4] = 0;         
          }
    }
  }
}
}
/**********************************************************
  Description:    Judge the BLE status
  Parameters:
  Return:         API_CONNECTED
                API_DISCONNECTED
                DATA_RECEIVED
                API_ERROR
  Others:
**********************************************************/
uint8_t bleProcess()
{
  uint8_t st = 0x00;
  uint8_t lenth = 0;
  if (BC7701.readData(receiveBuf, lenth))
  {
    switch (receiveBuf[1])
    {
      case 0x00:
        if (receiveBuf[0] == 0x00)
        {
          if ((receiveBuf[3] & 0x01) == 0x01)
          {
            st = API_CONNECTED;

          }
          else st = API_DISCONNECTED;

        }
        break;
      case 0xF2:
        if ((receiveBuf[0] == 0x00) && (receiveBuf[2] == 0xFF))
        {
          st = DATA_RECEIVED;
        }
        break;
      default: break;
    }
  }
  else st = API_ERROR;
  return st;
}
