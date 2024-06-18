#include "Settings.h"
#include "Configure.h"

Ucglib_SSD1351_18x128x128_SWSPI ucg(/*sclk=*/ 13, /*data=*/ 14, /*cd=*/ 5 , /*cs=*/ 12, /*reset=*/ 4);

String days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, timeZone * 3600);

int netStatus = 0;
int _netStatus = 0;
int secsSinceBoot = 0;
int minsSinceBoot = 0;
int hrsSinceBoot = 0;
int daysSinceBoot = 0;
int d, h, m, s = 0;

void log(int type, String text)
{
  switch (type) {
    case 1:
      Serial.print("\n[INFO] ");
      Serial.print(text);
      break;
    case 2:
      Serial.print("\n[NOTE] ");
      Serial.print(text);
      break;
    case 3:
      Serial.print("\n[WARN] ");
      Serial.print(text);
      break;
    case 4:
      Serial.print("\n[ERR] ");
      Serial.print(text);
      break;
    case 5:
      Serial.print("\n[FATAL] ");
      Serial.print(text);
      break;
    case 6:
      Serial.print("\n[PANIC] ");
      Serial.print(text);

      ucg.setPrintPos(0, 0);
      ucg.setColor(255, 0, 0);
      ucg.printf("Kernel panic\nCheck Serial\nmonitor"); // Print Kernel panic message to screen
      break;
    default:
      Serial.print("\n");
      Serial.print(text);
      break;
  }
}

int getNetworkStatus()
{
  if (WiFi.status() == WL_DISCONNECTED) 
    return 0;
  
  if (WiFi.status() == WL_CONNECTED) 
    return 1;

  if (WiFi.status() == WL_CONNECT_FAILED) 
    return 2;

  if (WiFi.status() == WL_CONNECTION_LOST)
    return 3;
  
  if (WiFi.status() == WL_NO_SSID_AVAIL) 
    return -1;
  
  if (WiFi.status() == WL_WRONG_PASSWORD)
    return -2;
}

void parseCommand() // Basic command parser
{
  String command = Serial.readString();
  String parsedCommand[128];

  int i = 0;
  int base = 0;
  int len = command.length();
  int isString = 0;
  int parse_idx = 0;
  char char_i[1];

  for (i=0; i<len; i++)
  {
    char_i[0] = command.charAt(i);

    if (char_i[0] == '"') 
    {
      if (isString == 0) {
        i++;
        isString = 1;
      } 
      else {
        isString = 0;
      }
    }
    if (char_i[0] == ' ')
    {
      if (isString == 0 && command.charAt(i-1) == '"') {
        parsedCommand[parse_idx] = command.substring(base, i);
        base = i+1;
        parse_idx++;
      } else if (isString == 0) {
        parsedCommand[parse_idx] = command.substring(base, i+1);
        base = i+1;
        parse_idx++;
      }
    }
  }

  if (parsedCommand[0] == "screen")
  {
    if (parsedCommand[1] == "1" || parsedCommand[1] == "true" || parsedCommand[1] == "enable") {
      ucg.powerUp();
      Serial.printf("\nEnabled Screen");
    }
      
    else if (parsedCommand[1] == "0" || parsedCommand[1] == "false" || parsedCommand[1] == "disable") {
      ucg.powerDown();
      Serial.printf("\nDisabled Screen");
    }
  }

  if (parsedCommand[0] == "time")
    Serial.printf("\n%s", timeClient.getFormattedTime());

  if (parsedCommand[0] == "day")
    if (parsedCommand[1] == "raw" || parsedCommand[1] == "int")
      Serial.printf("\n%s", timeClient.getDay());
    else
      Serial.printf("\n%s", days[ timeClient.getDay() ]);
  if (parsedCommand[0] == "echo")
    Serial.printf("\n%s", parsedCommand[1]);
}

void drawBMP(const int *image, int ix, int iy)
{
  int x = *image;
  int y = *(image+1);
  int color = *(image+2);
  
  int i, j;
  const int *image_ = image + 3;

  for (i=0; i<y; i++)
  {
    for (j=0; j<x; j++)
    {
      ucg.setColor(*(image_+(i*x + j)*3), *(image_+(i*x + j)*3+1), *(image_+(i*x + j)*3+2) );
      ucg.drawPixel(j+ix, i+iy);
    }
  }
}

void kpanic(String reason)
{
  log(6, reason);
  ESP.restart();
}

void drawConnectionStatus(int type)
{
  switch (type)
  {
    case 1: // Draw black box to erase connection status message
      ucg.setColor(0, 0, 0);
      ucg.drawBox(0, 0, 127, 37);
      break;
    case 2:
      ucg.setColor(255, 255, 255);
      ucg.setFont(ucg_font_6x13_tf);
      ucg.setPrintPos(8, 18);
      ucg.print("Connecting to WiFi");
      break;
    case 3:
      ucg.setColor(255, 255, 255);
      ucg.setFont(ucg_font_6x13_tf);
      ucg.setPrintPos(8, 18);
      ucg.print("Failed to connect");
      break;
    case 4:
      ucg.setColor(255, 255, 255);
      ucg.setFont(ucg_font_6x13_tf);
      ucg.setPrintPos(8, 18);
      ucg.print("Connected to WiFi");
      break;
    default:
      log(4, "Invalid argument has been passed to drawConnectionStatus");
      break;
  }
}

void drawLoadingIcon(int num, int x, int y)
{
  int i = 0;

  if (num == -1)
  {
    for (i=0; i<8; i++)
      drawBMP(blankDot, loadingDot_PosX[i] + x, loadingDot_PosY[i] + y);
  } else {
    for (i=0; i<8; i++)
      if (i != loadingDot_skipDraw[num % 8])
        drawBMP(loadingDot, loadingDot_PosX[i] + x, loadingDot_PosY[i] + y);
      else
        drawBMP(blankDot, loadingDot_PosX[i] + x, loadingDot_PosY[i] + y);
  }
}

void drawNumberDigit(int num, int x, int y, int fontNo)
{
  switch (num)
  {
    case 48: {
      if (fontNo == 0) {
        drawBMP(arial12_0, x, y);
      }
      break;
    }
    case 49: {
      if (fontNo == 0) {
        drawBMP(arial12_1, x, y);
      }
      break;
    }
    case 50: {
      if (fontNo == 0) {
        drawBMP(arial12_2, x, y);
      }
      break;
    }
    case 51: {
      if (fontNo == 0) {
        drawBMP(arial12_3, x, y);
      }
      break;
    }
    case 52: {
      if (fontNo == 0) {
        drawBMP(arial12_4, x, y);
      }
      break;
    }
    case 53: {
      if (fontNo == 0) {
        drawBMP(arial12_5, x, y);
      }
      break;
    }
    case 54: {
      if (fontNo == 0) {
        drawBMP(arial12_6, x, y);
      }
      break;
    }
    case 55: {
      if (fontNo == 0) {
        drawBMP(arial12_7, x, y);
      }
      break;
    }
    case 56: {
      if (fontNo == 0) {
        drawBMP(arial12_8, x, y);
      }
      break;
    }
    case 57: {
      if (fontNo == 0) {
        drawBMP(arial12_9, x, y);
      }
      break;
    }
    case 58: {
      if (fontNo == 0) {
        drawBMP(arial12_colon, x, y);
      }
      break;
    }
  }
}

void drawNumber(String numstr, int x, int y)
{
  int i, ascii;
  int len = numstr.length();

  for (i=0; i<len; i++) {
    ascii = (int) numstr.charAt(i);
    drawNumberDigit(ascii, i*8+1, y, 0);
  }
}

void drawOverlay(bool drawClock, bool drawNetworkStat)
{
  log(1, "Rendering information bar");
  ucg.setColor(0, 0, 0);
  ucg.drawBox(0, 111, 127, 127); // Erase previous status

  ucg.setColor(255, 255, 255);
  ucg.drawHLine(0, 109, 127); // Draw horizontal bar
  ucg.drawHLine(0, 110, 127);

  //ucg.setPrintPos(1, 126);
  // ucg.print(timeClient.getFormattedTime().substring(0, 5)); // Print current time
  drawNumber(timeClient.getFormattedTime().substring(0, 5), 1, 115);
  if (drawNetworkStat) {
    if (WiFi.status() == WL_CONNECTED) {
      drawBMP(WiFiIcon, 112, 114); // Wifi Icon
    } else {
      // Commented due to icon being unavailable

      // drawBMP(NoWiFiIcon, 112, 114); // No connection icon
    }
  }
}

void setup(void)
{
  int n = 0;

  Serial.begin(115200); // Init serial interface
  Serial.print("\nKernel init");

  if (!ENABLE_WDT) {  
    ESP.wdtDisable();
    *((volatile uint32_t*) 0x60000900) &= ~(1); // Disable Watchdog
  } else {
    log(2, "WDT is enabled. It is suggested to disable WDT as it can cause the system to crash.");
  }

  ucg.begin(UCG_FONT_MODE_SOLID);
  ucg.setRotate270();
  ucg.clearScreen(); // Init display

  if (FORCE_PANIC)
    kpanic("Kernel panic triggered by FORCE_PANIC option in Settings.h");
 
  drawConnectionStatus(2); // Display WiFi Connection status

  Serial.print("\n");

  log(1, "WiFi Initialized");
  WiFi.begin(ssid, password); // Init WiFi

  log(1, "Connecting to WiFi");
  while ( WiFi.status() != WL_CONNECTED ) {
    n++;
    delay(200);

    drawLoadingIcon((n % 8) + 1, 57, 48);

    if (FORCE_FAIL_WIFI) {
      WiFi.disconnect(true);
      n = 300; // Set n to 300 to cause WiFi connection to fail
    }

    if (WiFi.status() == WL_NO_SSID_AVAIL
    || WiFi.status() == WL_WRONG_PASSWORD) {
      log(4, "WiFi connection has failed either due to wrong SSID or password. Please check WiFi credentials on Settings.h");
    }

    if (ENABLE_WDT) yield(); // Execute yield() to prevent crashing if wdt is enabled

    Serial.print(".");
    if (n >= 75) {
      log(2, "15 seconds has elapsed, but WiFi connection is still not estabilished. Considering that as error, WiFi connection process will be forcibly ended");
      log(4, "Failed to connect to WiFi network [" + String(ssid) + "]");

      drawConnectionStatus(1); // Erase previous status text
      drawConnectionStatus(3); // Print "Failed to connect"

      break;
    }
  }

  drawLoadingIcon(-1, 57, 48);

  if (WiFi.status() != WL_CONNECTED)
  {
    drawConnectionStatus(1); // Erase previous status text
    drawConnectionStatus(3); // Print "Failed to connect"
  } else if (WiFi.status() == WL_CONNECTED) {
    log(1, "Successfully connected to WiFi network [" + String(ssid) + "]");
    drawConnectionStatus(1);
    drawConnectionStatus(4);
  }

  timeClient.begin(); // Init NTP Client
  timeClient.update(); // Update time

  drawOverlay(true, true); // Boot completed; draw the overlay
} 

void loop(void)
{
  delay(50);
  s++;

  if (PRINT_INTERNAL_TIMER)
    Serial.printf("\n%dd %d:%d:%d [%dh] [%dm] [%ds]", d, h, m, s, hrsSinceBoot, minsSinceBoot, secsSinceBoot);

  secsSinceBoot++;

  if (secsSinceBoot == 2)
    drawConnectionStatus(1);

  if (minsSinceBoot % NTPUpdateInterval == 0)
    timeClient.update();

  parseCommand();

  _netStatus = netStatus;
  netStatus = WiFi.status();

  if (_netStatus != netStatus)
    drawOverlay(false, true);

  if (s >= 60) {
    s = 0;
    drawOverlay(true, false);

    m++;
    minsSinceBoot++;
    Serial.printf("\n%s", timeClient.getFormattedTime());
  }

  if (m >= 60) {
    m = 0;
    h++;
    hrsSinceBoot++;
  }

  if (h >= 24) {
    h = 0;
    d++;
    daysSinceBoot++;
  }
}
