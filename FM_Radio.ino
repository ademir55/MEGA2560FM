/*MCP4261       ARDUİNO MEGA
CS    1          SS   53
SCK   2          SCK  52
MOSI  3          MOSI 51
MISO 13          MISO 50
SHDN 12         SHDN  7*/

    #include <UTFT.h>
    #include <URTouch.h>
    #include <Adafruit_GFX.h>
    #include <DS3231.h>
    #include <SPI.h>
    #include <TEA5767.h>
    #include <Wire.h>
    #include <Button.h>
    #include <EEPROM.h>
    #include <UTFT_Geometry.h>

    TEA5767 Radio;   //Pinout SLC ve SDA - Arduino pinleri 20 ve 21

    UTFT     myGLCD(SSD1289, 38, 39, 40, 41); //Parametreler Display / Schield modelinize göre ayarlanmalıdır
    URTouch  myTouch( 6, 5, 4, 3, 2);
    DS3231  rtc(SDA, SCL);

    //==== YAZI FONTLARI==================================================
   
    extern uint8_t SmallFont[];
    extern uint8_t BigFont[];
    extern uint8_t SevenSegNumFont[];
    extern uint8_t GroteskBold24x48[]; 

    
    extern unsigned int AlarmButton[0x1040];
    extern unsigned int PreviousButton[0x9C4];
    extern unsigned int NextButton[0x9C4];
    extern unsigned int PreviousButton1[0x9C4];
    extern unsigned int NextButton1[0x9C4];
    extern unsigned int VolumeDown[0x170];
    extern unsigned int VolumeUp[0x3B8];

    double old_frequency;
    double frequency;
    int search_mode = 0;
    int search_direction;
    unsigned long last_pressed;

    int x, y; // Ekranın basıldığı koordinatlar için değişkenler
    char currentPage, playStatus;

    int aHours = 0;
    int aMinutes = 0;

    boolean fm = false;
    boolean blueON = false;
    boolean alarmNotSet = true;
    String alarmString = "";

    float currentTemperature, temperature;
    static word totalTime, elapsedTime, playback, minutes, seconds, lastSeconds, minutesR, secondsR;
    String currentClock, currentHours, currentMinutes, currentSeconds, currentDate;
    String timeString, hoursString, minutesString, secondsString, hoursS, minutesS, secondsS, dateS;

                                                     // Frekans Seçme
    //                           1    2   3     4   5     6    7   8   9  10  11   12   13   14   15  16    17   18   19    20   21   22    23    24    25   26                           
    double    ai_Stations[26]={88.8,89.3,90.8,91.2,91.8,92.2,92.4,93.2,94,95.8,96,96.8,98.1,98.4,99.3,99.5,100,100.5,104.6,105,105.4,105.6,106,106.5,106.8,107.9};
    int    i_sidx=0;        // Maksimum İstasyon Endeksi sıfırdan sayılan istasyon sayısı)
    int    i_smax=25;       // Maks İstasyon Dizini - 0..22 = 23 :) (maksimum istasyon sayısı - sıfırdan sayılır)
    int    i_smin=1;

     byte f1, f2, f3, f4;  // her frekans bölgesi için numara
     double frekans;
     int frekans10;      // frekans x 10 
//----------------------------------------------------------------
    int PotWiperVoltage0 = 0;
    int RawVoltage0 = 0;
    float Voltage0 = 0;
    int PotWiperVoltage1 = 1;
    int RawVoltage1 = 0;
    float Voltage1 = 0;
    const int wiper0writeAddr = B00000000;
    const int wiper1writeAddr = B00010000;
    const int tconwriteAddr = B01000000;
    const int tcon_0off_1on = B11110000;
    const int tcon_0on_1off = B00001111;
    const int tcon_0off_1off = B00000000;
    const int tcon_0on_1on = B11111111;
    const int slaveSelectPin = 53;   //cs
    const int shutdownPin = 7;
//-----------------------------------------------------------------

    // transform lin to log
    float a = 0.2;     // for x = 0 to 63% => y = a*x;
    float x1 = 63;
    float y1 = 12.6;   // 
    float b1 = 2.33;     // for x = 63 to 100% => y = a1 + b1*x;
    float a1 = -133.33;

    int level, level1, level2, stepi;

    int lung = 300;    // buton için duraklat
    int scurt = 25;   // tekrar döngüsü için duraklat
//---------------------
    byte sunet = 0;    
    byte volmax = 15;
//-----------------------
    UTFT_Geometry geo(&myGLCD);

    void setup() {
//--------------------------------------------------- 
     // slaveSelectPin'i bir çıkış olarak ayarlayın:
     pinMode (slaveSelectPin, OUTPUT);
     //shutdownPin'i bir çıkış olarak ayarlayın:
     pinMode (shutdownPin, OUTPUT);
     // tüm kapların kapanmasıyla başla
     digitalWrite(shutdownPin,LOW);
     // SPI'yı başlat:
      SPI.begin(); 
 
    digitalPotWrite(wiper0writeAddr, 0); // Sileceği ayarla 0 
    digitalPotWrite(wiper1writeAddr, 0); // Sileceği ayarla 0 
 
//----------------------------------------------------- 
    // son frekansın okuma değeri
 
  f1 = EEPROM.read(101);
  f2 = EEPROM.read(102);
  f3 = EEPROM.read(103);
  f4 = EEPROM.read(104);
  stepi = EEPROM.read(105);

  // numarayı kurtar
  frekans = 100*f1 + 10*f2 + f3 + 0.1*f4;
 
  Wire.begin();
  Radio.init();
  Radio.set_frequency(frekans); 
  i_sidx = 7; // İstasyonda indeks = 8 ile başlayın (Eski - istasyon 8'den başlıyoruz, ancak sıfırdan sayıyoruz)
  Radio.set_frequency(ai_Stations[i_sidx]); 
  // Initiate display
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  rtc.begin();
  
  // FM RADİO
  Serial.begin(9600);  
  pinMode(8, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(8,HIGH);
  digitalWrite(13,HIGH);
  
  drawHomeScreen();  // Ana Ekranı Çizer
  currentPage = '0'; // Ana Ekranda olduğumuzu gösterir
  playStatus = '0';  // Fm sayfasını 
  currentTemperature = rtc.getTemp();
  currentDate = rtc.getDateStr();
  currentClock = rtc.getTimeStr();
  timeString = rtc.getTimeStr();
  currentHours = timeString.substring(0, 2);
  currentMinutes = timeString.substring(3, 5);
  currentSeconds = timeString.substring(6, 8);
 //================================================================ 
  int procent1 = stepi * 6.66;
    if (procent1 < 63)  // for x = 0 to 63% => y = a*x;
    {
    level = a * procent1;
    }
    else               // for x = 63 to 100% => y = y1 + b*x;
    {
    level = a1 + b1 * procent1;
    }  
level1 = map(level, 0, 100, 0, 255);      // 256 adımda% 100 dönüştürün 
digitalPotWrite(wiper0writeAddr, level1); // Sileceği 0 ile 255 arasında ayarlayın
digitalPotWrite(wiper1writeAddr, level1); // Sileceği 0 ile 255 arasında ayarlayın
digitalWrite(shutdownPin,HIGH); //Turn off shutdown
//==========================================================================
 }

   void loop() {
   
    unsigned char buf[5];
    int stereo;
    int signal_level;
    double current_freq;
    unsigned long current_millis = millis();
    
     if (Radio.read_status(buf) == 1) {
    current_freq =  floor (Radio.frequency_available (buf) / 100000 + .5) / 10;
    stereo = Radio.stereo(buf);
    signal_level = Radio.signal_level(buf);
      if (search_mode == 1) {
      if (Radio.process_search (buf, search_direction) == 1) {
          search_mode = 0;
      }
      if (Radio.read_status(buf) == 1) {  
      frekans =  floor (Radio.frequency_available (buf) / 100000 + .5) / 10;
      frekans10 = 10*frekans;
    f1 = frekans10/1000;
    frekans10 = frekans10 - 1000*f1;
    f2 = frekans10/100;
    frekans10 = frekans10 - 100*f2;
    f3 = frekans10/10;
    f4 = frekans10 - 10*f3;
    EEPROM.write(101,f1);
    EEPROM.write(102,f2);
    EEPROM.write(103,f3);
    EEPROM.write(104,f4);
    frekans = 100*f1 + 10*f2 + f3 + 0.1*f4;
    Radio.set_frequency(frekans);
      }
    }
  }
    if (sunet == 1) {
    if (stepi <= 0) stepi = 0;
    if (stepi >= volmax) stepi = volmax;
     EEPROM.write(105,stepi);
    int procent1 = stepi * 6.66;
    if (procent1 < 63)  // for x = 0 to 63% => y = a*x;
    {
    level = a * procent1;
    }
    else               // for x = 63 to 100% => y = y1 + b*x;
    {
    level = a1 + b1 * procent1;
    }  
    level1 = map(level, 0, 100, 0, 255);      // convert 100% in 256 steps  

    if (level1 >255) level1 = 255;
    digitalPotWrite(wiper0writeAddr, level1); // Set wiper 0 to 255
    digitalPotWrite(wiper1writeAddr, level1); // Set wiper 0 to 255
    sunet = 0;
  }
 //******************************************************************** 
  // Homes Screen
  if (currentPage == '0') {
    // Saatin değişip değişmediğini kontrol eder
    if ( currentClock != rtc.getTimeStr()) {
      timeString = rtc.getTimeStr();
      hoursS = timeString.substring(0, 2);
      minutesS = timeString.substring(3, 5);
      secondsS = timeString.substring(6, 8);
      myGLCD.setFont(SevenSegNumFont);
      myGLCD.setColor(0, 255, 0);
      myGLCD.print(secondsS, 224, 50);
      if ( currentMinutes != minutesS ) {
      myGLCD.print(minutesS, 128, 50);
      currentMinutes = minutesS;
      }
      if ( currentHours != hoursS ) {
        myGLCD.print(hoursS, 32, 50);
        currentHours = hoursS;
      }
     // Tarih değişikliğini denetler
      dateS = rtc.getDateStr();
      delay(10);
      if ( currentDate != dateS){
          myGLCD.setColor(255, 255, 255); // Rengi beyaza ayarlar
          myGLCD.setFont(BigFont); // Yazı tipini büyük yapar
          myGLCD.print(rtc.getDateStr(), 153, 7);
        }
      // Sıcaklık değişimini kontrol eder
      temperature = rtc.getTemp();
      delay(10);
      if ( currentTemperature != temperature ){
        myGLCD.setColor(255, 255, 255); // Rengi beyaza ayarlar
        myGLCD.setFont(BigFont); // Yazı tipini büyük yapar
        myGLCD.printNumI(temperature, 39, 7);
        currentTemperature = temperature;
      }
      delay(10);
      currentClock = rtc.getTimeStr();
    }
   //**********************************BLUETOOTH ÇİZİM***************
    if (blueON == true){
  myGLCD.setColor(255, 255, 255); // Beyaz  
  geo.drawTriangle(280, 155, 280, 140, 290, 148);   
  geo.drawTriangle(280, 155, 280, 170, 290, 162);  
  myGLCD.drawLine(280, 155, 270, 148);  
  myGLCD.drawLine(280, 155, 270, 162); 
  myGLCD.setColor(0, 0, 255);  // mavi
  myGLCD.drawCircle(280, 155, 28);  
  myGLCD.drawCircle(280, 155, 27);  
  myGLCD.drawCircle(280, 155, 26); 
  myGLCD.drawCircle(280, 155, 25);
  }
    else{
  myGLCD.setColor(255, 255, 255); // Beyaz  
  geo.drawTriangle(280, 155, 280, 140, 290, 148);   
  geo.drawTriangle(280, 155, 280, 170, 290, 162);  
  myGLCD.drawLine(280, 155, 270, 148);  
  myGLCD.drawLine(280, 155, 270, 162); 
  myGLCD.drawCircle(280, 155, 28);  
  myGLCD.drawCircle(280, 155, 27);  
  myGLCD.drawCircle(280, 155, 26);  
  }
  //**********************************BLUETOOTH ÇİZİM***************  
   //**********************************radyo ÇİZİM***************
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(5, 148, 40, 138); 
  myGLCD.drawRoundRect(52,174,10,148); 
  myGLCD.drawRoundRect(48,152,30,158);
 
  myGLCD.drawLine(15, 158, 15, 170);
  myGLCD.drawLine(18, 158, 18, 170);
  myGLCD.drawLine(21, 158, 21, 170);
 
  myGLCD.drawCircle(42, 166, 4);
  myGLCD.drawCircle(32, 158, 28);  
  myGLCD.drawCircle(32, 158, 27);  
  myGLCD.drawCircle(32, 158, 26);  
  
  //**********************************radyo ÇİZİM***************  
       
       // Ekrana dokunulup dokunulmadığını kontrol eder
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX(); // Ekranın basıldığı X koordinatı
      y = myTouch.getY(); // Ekranın basıldığı Y koordinatı
      //FM Düğmesine basarsak
      if ((x >= 0) && (x <= 80) && (y >= 125) && (y <= 190)) {
          drawFrame(32, 157, 33);
          currentPage = '1';
          fm = false; 
          digitalWrite(8,0); // Set low 
           myGLCD.clrScr();
           blueON = false;
          
          delay(100);
          drawMusicPlayerScreen();
          delay(100);
        }
     
      // ALARM BUTONU
      if ((x >= 140) && (x <= 180) && (y >= 125) && (y <= 190)) {
          drawFrame(150, 160, 29);
          currentPage = '2';
          myGLCD.clrScr();
        }
       
        // Bluetooth Buton    blueON
     if ((x >= 280) && (x <= 300) && (y >= 125) && (y <= 190)) {
      drawFrame(280, 160, 29);
      if(blueON == true) {
           blueON =false ;
          digitalWrite(13,HIGH); // Set HIGH       
           fm = false;
           delay(250);
     }
       else {
            blueON = true; 
          digitalWrite(13,LOW); // Set LOW
           delay(250);
        }
      }
    }
  }
  
   // FM Screen
  if (currentPage == '1') {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX(); 
      y = myTouch.getY(); 
    
    // istasyon ara yukarı
      if ((x >= 5) && (x <= 55) && (y >= 110) && (y <= 147)) {
         drawFrame(30, 135, 26);
        last_pressed = current_millis;
        search_mode = 1;
        search_direction = TEA5767_SEARCH_DIR_UP;
        Radio.search_up(buf);
        delay(lung/5);
       }
      if ((x >= 27) && (x <= 87) && (y >= 190) && (y <= 220)) {
        drawFrame(30, 210, 26);
        last_pressed = current_millis;
        search_mode = 1;
        search_direction = TEA5767_SEARCH_DIR_DOWN;
        Radio.search_down(buf);
        delay(lung/5);
        }
         // istasyon hafıza yukarı
     if ((x >= 240) && (x <= 290) && (y >= 97) && (y <= 147)) {
         drawFrame(282, 135, 26);
         i_sidx++;
        if (i_sidx>i_smax){i_sidx=0;
        }
          Radio.set_frequency(ai_Stations[i_sidx]);
      delay(lung/5);
       }
          if ((x >= 250) && (x <= 290) && (y >= 190) && (y <= 220)) {
         drawFrame(282, 210, 26);
         i_sidx--;
         if (i_sidx<i_smin){i_sidx=25;
      }
         Radio.set_frequency(ai_Stations[i_sidx]);
          delay(lung/5);
       }

        // **********SES******------------------------------------------------------
      if ((x >= 110) && (x <= 130) && (y >= 190) && (y <= 220)) {
        drawFrame(120, 210, 13);
           stepi = stepi -1;
           if (stepi <= 0) stepi == 0;
           sunet = 1;
          delay(lung/5);
      }
      // **********SES******+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      if ((x >= 185) && (x <= 230) && (y >= 200) && (y <= 230)) {
        drawFrame(190, 220, 16);
       stepi = stepi +1;
         if (stepi > volmax ) stepi == volmax;
         sunet = 1;
        delay(lung/5);
        }
      
      // ÇIKIŞ MENUSU
      if ((x >= 0) && (x <= 75) && (y >= 0) && (y <= 30)) {
        myGLCD.clrScr();
        drawHomeScreen();  // Draws the Home Screen
        fm = true; 
        digitalWrite(8,1); // Set high
         blueON =false ;
        digitalWrite(13,HIGH);
        currentPage = '0';
         return;
      }
    }
      
     //===============sinyal değerini çiz=======================
      
         myGLCD.setFont(SmallFont);
         myGLCD.setColor(255, 255, 255);
          myGLCD.print("Signal",LEFT,30);
          if (current_freq < 10) {
         myGLCD.printNumF(signal_level,2,LEFT,55);
         }
       else
      {
        myGLCD.setFont(SmallFont);
         myGLCD.setColor(255, 255, 255);
         myGLCD.printNumF(signal_level,1,LEFT,55);
      }
     //=====================sinyal değerini çiz==================    
      myGLCD.setFont(GroteskBold24x48);
      myGLCD.setColor(255, 0, 0);
      if (current_freq < 100) {
        myGLCD.printNumF(current_freq,3, CENTER, 50);
      }
       else
      {
       myGLCD.setFont(GroteskBold24x48);
      myGLCD.setColor(255, 0, 0);
       myGLCD.printNumF(current_freq,2, CENTER, 50);
       }
       myGLCD.setColor(255, 255, 255);
       myGLCD.setFont(BigFont);
       myGLCD.print("MHz",235,50);

      if (stereo){
        myGLCD.setColor(255, 255, 255);
       myGLCD.setFont(BigFont);
       myGLCD.print("STEREO",CENTER,110);
      }
      else
      {
        myGLCD.setColor(255, 0, 0);
       myGLCD.setFont(BigFont);
       myGLCD.print("MONO  ",CENTER,110); 
      }
       myGLCD.setFont(SmallFont);
       myGLCD.setColor(255, 255, 255);
       myGLCD.print("  ARA",LEFT,80);
       
      //-----------------------------------
       if (stepi <=0) stepi = 0;
       if (stepi >=volmax) stepi = volmax; 
      if (stepi<10)  {
         myGLCD.setFont(SmallFont);
         myGLCD.setColor(255, 255, 255);
         myGLCD.printNumF(stepi,2,CENTER,210);
          }
       else
      {
         myGLCD.setFont(SmallFont);
         myGLCD.setColor(255, 255, 255);     
         myGLCD.printNumF(stepi,1,CENTER,210);
       }
                
       //------------------------------------     
            printpost(current_freq);
  
   // Saati sağ üst köşeye yazdırmak
    myGLCD.setFont(BigFont);
    myGLCD.setColor(255, 255, 255);
    printClock(187, 5);
  }
    // Alarm SAAT EKRANI
    if (currentPage == '2') {
    myGLCD.setFont(BigFont);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("CIKIS", 5, 5);
    myGLCD.print("Set Alarm", CENTER, 20);
    
    //Saatler ve dakikalar arasında iki nokta üst üste çizer
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillCircle (112, 65, 4);
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillCircle (112, 85, 4);
    myGLCD.setFont(SevenSegNumFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.printNumI(aHours, 32, 50, 2, '0');
    myGLCD.printNumI(aMinutes, 128, 50, 2, '0');
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (42, 115, 82, 145);
    myGLCD.drawRoundRect (138, 115, 178, 145);
    myGLCD.setFont(BigFont);    
    myGLCD.print("H", 54, 122);
    myGLCD.print("M", 150, 122);
    myGLCD.drawRoundRect (215, 60, 303, 90);
    myGLCD.print("SET", 236, 67);
    myGLCD.drawRoundRect (215, 115, 303, 145);
    myGLCD.print("SIL", 236, 122);
    alarmNotSet = true;
      while (alarmNotSet){
      if (myTouch.dataAvailable()) {
        myTouch.read();
        x = myTouch.getX(); 
        y = myTouch.getY(); 
        //Set SAAT button
        if ((x >= 42) && (x <= 82) && (y >= 115) && (y <= 145)) {
          drawRectFrame(42, 115, 82, 145);
          aHours++;
          if(aHours >=24){
            aHours = 0;
          }
          myGLCD.setFont(SevenSegNumFont);
          myGLCD.setColor(0, 255, 0);
          myGLCD.printNumI(aHours, 32, 50, 2, '0');
        }
        // Set DAKIKA buttonu
        if ((x >= 138) && (x <= 178) && (y >= 115) && (y <= 145)) {
          drawRectFrame(138, 115, 178, 145);
          aMinutes++;
          if(aMinutes >=60){
            aMinutes = 0;
          }
          myGLCD.setFont(SevenSegNumFont);
          myGLCD.setColor(0, 255, 0);
          myGLCD.printNumI(aMinutes, 128, 50, 2, '0');
      }
      // Set alarm buttonu
      if ((x >= 215) && (x <= 303) && (y >= 60) && (y <= 80)) {
        drawRectFrame(215, 60, 303, 90);
        if (aHours < 10 && aMinutes < 10){
          alarmString = "0"+(String)aHours + ":" + "0"+ (String)aMinutes + ":" + "00";
        }
        else if (aHours < 10 && aMinutes > 9){
          alarmString = "0"+(String)aHours + ":" + (String)aMinutes + ":" + "00";
        }
        else if (aHours > 9 && aMinutes < 10){
          alarmString = (String)aHours + ":" + "0"+ (String)aMinutes + ":" + "00";
        }
        else {
          alarmString = (String)aHours + ":" + (String)aMinutes + ":" + "00";
        }
        myGLCD.setFont(BigFont);
        myGLCD.print("Alarm set for:", CENTER, 165);
        myGLCD.print(alarmString, CENTER, 191);
      
      }
      // ALARM BUTON SİL
      if ((x >= 215) && (x <= 303) && (y >= 115) && (y <= 145)) {
        drawRectFrame(215, 115, 303, 145);
        alarmString="";
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(45, 165, 275, 210); 
      }
      // ÇIKIŞ MENUSU
      if ((x >= 0) && (x <= 75) && (y >= 0) && (y <= 30)) {
        alarmNotSet = false;
        currentPage = '0';
        myGLCD.clrScr();
        drawHomeScreen();  // Draws the Home Screen
      }    
     }
    }
   }
    // Alarm AKTİF     
    if (alarmNotSet == false) {
      if (alarmString == rtc.getTimeStr()){
        myGLCD.clrScr();
        fm = false; 
        digitalWrite(8,0); // Set high
        delay(100);
        myGLCD.setFont(BigFont);
        myGLCD.setColor(255, 255, 255);
        myGLCD.print("ALARM", CENTER, 90);
        myGLCD.drawBitmap (127, 10, 65, 64, AlarmButton);
        myGLCD.print(alarmString, CENTER, 114);
        myGLCD.drawRoundRect (94, 146, 226, 170);
        myGLCD.print("KAPAT", CENTER, 150);
        boolean alarmOn = true;
        while (alarmOn){
          if (myTouch.dataAvailable()) {
          myTouch.read();
          x = myTouch.getX(); 
          y = myTouch.getY(); 
          
          // ALARM STOP
          if ((x >= 94) && (x <= 226) && (y >= 146) && (y <= 170)) {
          drawRectFrame(94, 146, 226, 170);
          alarmOn = false;
          alarmString="";
          myGLCD.clrScr();
          fm = true; 
          digitalWrite(8,1); // Set high
          delay(100);
          currentPage = '0';
          drawHomeScreen();
          }
         }
        }
       }
      }
     }
   
    // Bu işlev, pota SPI verilerinin gönderilmesini sağlar.
     void digitalPotWrite(int address, int value) {
  // çipi seçmek için SS pinini düşük tutun:
     digitalWrite(slaveSelectPin,LOW);
 
  //  SPI aracılığıyla adresi ve değeri gönder:
     SPI.transfer(address);
     SPI.transfer(value);
  //çipin seçimini kaldırmak için SS pinini yükseğe alın:
     digitalWrite(slaveSelectPin,HIGH); 
    }
   void drawHomeScreen() {
  myGLCD.setBackColor(0, 0, 0); // Metnin siyah olarak yazdırılacağı alanın arka plan rengini ayarlar
  myGLCD.setColor(255, 255, 255); //Rengi beyaza ayarlar
  myGLCD.setFont(BigFont); // Yazı tipini büyük yapar
  myGLCD.print(rtc.getDateStr(), 153, 7);
  myGLCD.print("T:", 7, 7);
  myGLCD.printNumI(rtc.getTemp(), 39, 7); 
  myGLCD.print("C", 82, 7); // 82,7
  myGLCD.setFont(SmallFont);
  myGLCD.print("o", 74, 5); // 74,5
  if (alarmString == "" ) {
    myGLCD.setColor(255, 255, 255);
   
    myGLCD.print("AYDIN DEMIR", CENTER, 215);
    
  }
  else {
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("Alarm Ayar Saati: ", 60, 215);
    myGLCD.print(alarmString, 188, 215);
  }
  
  
  drawAlarmButton();
  drawHomeClock();
}

  void drawMusicPlayerScreen() {
  // Title
  myGLCD.setBackColor(0, 0, 0); // Metnin yazdırılacağı alanın arka plan rengini siyah olarak ayarlar 0,0,0
  myGLCD.setColor(255, 255, 255); // Rengi beyaz olarak ayarlar // 255,255,255
  myGLCD.setFont(BigFont); // Yazı tipini büyük yapar
  myGLCD.print("CIKIS", 5, 5); // Dizeyi ekrana yazdırır
  myGLCD.setColor(255, 0, 0); // Rengi kırmızı olarak ayarlar
  myGLCD.drawLine(0, 26, 319, 26); // Kırmızı çizgiyi çizer

 
  drawPreviousButton();
  drawNextButton();
  drawPreviousButton1();
  drawNextButton1();
  drawVolumeDown();
  drawVolumeUp();
  }
  void drawAlarmButton() {
  myGLCD.drawBitmap (120, 125, 65, 64, AlarmButton);
  }
  void drawNextButton() {
  myGLCD.drawBitmap (260, 110, 50, 50, NextButton);
  }
  void drawPreviousButton() {
  myGLCD.drawBitmap (5, 185, 50, 50, PreviousButton);    
  }
  void drawNextButton1() {
  myGLCD.drawBitmap (5, 110, 50, 50, NextButton1);  
  }
  void drawPreviousButton1() {
   myGLCD.drawBitmap (260, 185, 50, 50, PreviousButton1);
   }
   void drawVolumeDown() {
   myGLCD.drawBitmap (110, 200, 16, 23, VolumeDown);
  }
  void drawVolumeUp() {
  myGLCD.drawBitmap (181, 198, 34, 28, VolumeUp);
  }

  // Basıldığında düğmeyi vurgular
  void drawFrame(int x, int y, int r) {
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawCircle (x, y, r);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(0, 0, 0);
  myGLCD.drawCircle (x, y, r);
  }
  void drawRectFrame(int x1, int y1, int x2, int y2) {
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  }
  void drawUnderline(int x1, int y1, int x2, int y2) {
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(0, 0, 0);
  myGLCD.drawLine (x1, y1, x2, y2);
  }
    void printClock(int x, int y) {
    if ( currentClock != rtc.getTimeStr()) {
    myGLCD.print(rtc.getTimeStr(), x, y);
    currentClock = rtc.getTimeStr();
   }
  }
  void drawColon() {
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillCircle (112, 65, 4);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillCircle (112, 85, 4);

  myGLCD.setColor(0, 255, 0);
  myGLCD.fillCircle (208, 65, 4);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillCircle (208, 85, 4);
  }
  void drawHomeClock() {
  timeString = rtc.getTimeStr();
  currentHours = timeString.substring(0, 2);
  currentMinutes = timeString.substring(3, 5);
  currentSeconds = timeString.substring(6, 8);
  myGLCD.setFont(SevenSegNumFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print(currentSeconds, 224, 50);
  myGLCD.print(currentMinutes, 128, 50);
  myGLCD.print(currentHours, 32, 50);
  drawColon();
  }

      // istasyon adlarını görüntüleme 
  void printpost(double current_freq)
  {
     // istasyon adlarını görüntüleme
  {    
   if (current_freq==88.8) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("Show Radyo   ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 1",RIGHT,80);
   }
    if (current_freq==89.3) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("ALEM FM      ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 2",RIGHT,80);
   }
    if (current_freq==90.8) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("SUPER FM     ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 3",RIGHT,80);
   }
    if (current_freq==91.2) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("DENIZLI NET  ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 4",RIGHT,80);
   }
  if (current_freq==91.8) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("Radyo 20     ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 5",RIGHT,80);
   }
  if (current_freq==92.2) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TRT FM       ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 6",RIGHT,80);
    }
  if (current_freq==92.4) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("RADYO SES    ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 7",RIGHT,80);
   }
  if (current_freq==93.2) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TRT FM       ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 8",RIGHT,80);
  }
  if (current_freq==94.0) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TRT TURkU    ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA: 9",RIGHT,80);
  }
  if (current_freq==95.8) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TRT NaGme    ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:10",RIGHT,80);
  }
  if (current_freq==96.0) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("METRO FM     ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:11",RIGHT,80);
  }
  if (current_freq==96.8) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("AKRA FM      ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:12",RIGHT,80);
  }
  if (current_freq==98.1) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TATLISES     ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:13",RIGHT,80);
  }
  if (current_freq==98.4) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("BEST FM      ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:14",RIGHT,80);
  }
  if (current_freq==99.3) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("Pal Nostalji ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:15",RIGHT,80);
  }
  if (current_freq==99.5) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("Radyo EGE    ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:16",RIGHT,80);
  }
  if (current_freq==100.0) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("POWER FM     ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:17",RIGHT,80);
  }
  if (current_freq==100.5) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("MUJDE FM     ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:18",RIGHT,80);
  }
  if (current_freq==104.6) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("POLIS RADYOSU",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:19",RIGHT,80);
  }
  if (current_freq==105.0) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("RADYO HOROZ  ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:20",RIGHT,80);
  }
  if (current_freq==105.4) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("GURBETCI     ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:21",RIGHT,80);
  }
  if (current_freq==105.6) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HABERTURK    ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:22",RIGHT,80);
  }
  if (current_freq==106.0) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("Dogus FM     ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:23",RIGHT,80);
  }
  if (current_freq==106.5) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("JOYTURK FM   ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:24",RIGHT,80);
  }
  if (current_freq==106.8) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("Dostlar FM   ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:25",RIGHT,80);
  }
  if (current_freq==107.9) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TRT HABER    ",CENTER,150);
   myGLCD.setFont(SmallFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("HAFIZA:26",RIGHT,80);
  }
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx 
  if (current_freq==89.6) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("Kafa Radyo   ",CENTER,150);
   }
  if (current_freq==90.0) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TRT RADYO 3  ",CENTER,150);
   }
  if (current_freq==90.2) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TURKUVAZ     ",CENTER,150);
   }
  if (current_freq==94.2) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TGRT FM      ",CENTER,150);
   }
  if (current_freq==95.2) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TRT RADYO 1  ",CENTER,150);
   }
  if (current_freq==97.8) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("TRT NAGME    ",CENTER,150);
   }
  if (current_freq==99.6) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("RADYO SEYMEN ",CENTER,150);
   }
  if (current_freq==99.8) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("KRAL POP     ",CENTER,150);
   }
  if (current_freq==102.2) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("ART FM       ",CENTER,150);
   }
  if (current_freq==103.5) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("DEHA         ",CENTER,150);
   }
  if (current_freq==104.2) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("RADYOSPOR    ",CENTER,150);
   }
  if (current_freq==107.7) { 
   myGLCD.setFont(BigFont);
   myGLCD.setColor(0, 255, 0);
   myGLCD.print("RADYO 7      ",CENTER,150);
   }
  } 
 }
