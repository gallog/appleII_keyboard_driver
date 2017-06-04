/*
 * Driver per tastiera Apple II - v1 - 02/09/2016
 * Arduino mini 16mhz
 * 
 * Connettore tastiera:
 *  +--+ 
 *  |  | 25 Shift
 *  |  | 24 Shift
 *  |  | 23 Gnd
 *  |  | 22 Reset (+)
 *  |  | 21 Reset (-)
 *  |  | 20 Rpt (+)
 *  |  | 19 Rpt (-)
 *  |  | 18 R9
 *  |  | 17 C0
 *  |  | 16 R8
 *  |  | 15 R7
 *  |  | 14 C1
 *  |  | 13 C2
 *  |  | 12 R6
 *  |  | 11 C3
 *  |  | 10 R5
 *  |  |  9 R4
 *  |  |  8 R3
 *  |  |  7 R2
 *  |  |  6 R1
 *  |  |  5 R0
 *  |  |  4 Bulb
 *  |  |  3 Cntrl
 *  |  |  2 C4
 *  |  |  1 NC
 *  +--+
 *  
 * 
 * Conettore uscita scheda driver:
 *         +---| |----+ 
 *    +5v  | 1     12 | Data 0
 * Strobe  | 2     11 | Data 1
 *  Reset  | 3     10 | Data 2
 *   Bulb  | 4      9 | Data 3 
 * Data 6  | 5      8 | Data 4 
 *    Gnd  | 6      7 | Data 5 
 *         +----------+ 
 * Strobe alto per OUTPUT_STROBE_DURATION_US=12 quando il dato e' da campionare
 * Output arduino tramite OUT_SERIAL_D, OUT_SERIAL_CLK passati ad uno shift register 74164
 * 
 */
#define KEY_SHIFT 5 
#define KEY_CTRL 2
#define R0 A0 
#define R1 A1
#define R2 12 
#define R3 11
#define R4 10
#define R5 A5
#define R6 A4
#define R7 9
#define R8 8
#define R9 7
#define RPT 6
#define STROBE     A3
#define COL_SERIAL_D   4
#define COL_SERIAL_CLK 3
#define OUT_SERIAL_D   13
#define OUT_SERIAL_CLK A2

#define SHIFT_CLOCK_DELAY_US 1
#define OUTPUT_STROBE_DURATION_US 12
#define doColumnClock()                      \
   digitalWrite(COL_SERIAL_CLK, HIGH);       \
   delayMicroseconds(SHIFT_CLOCK_DELAY_US);  \
   digitalWrite(COL_SERIAL_CLK, LOW);        \
   delayMicroseconds(SHIFT_CLOCK_DELAY_US);
#define doOutClock()                         \
   digitalWrite(OUT_SERIAL_CLK, HIGH);       \
   delayMicroseconds(SHIFT_CLOCK_DELAY_US);  \
   digitalWrite(OUT_SERIAL_CLK, LOW);        \
   delayMicroseconds(SHIFT_CLOCK_DELAY_US);

void sendDataToShiftRegister(byte columData);
void ouputKeyboardData(byte data);
byte decodeRow();

#define COL 5
#define ROW 10

byte keyMap[ROW][COL]      = {{'3','q','d','z','s'},     
                              {'4','w','f','x','2'},     
                              {'5','e','g','c','1'},
                              {'6','r','h','v',27}, //27 = esc
                              {'7','t','j','b','a'},
                              {'8','y','k','n',' '},
                              {'9','u','l','m',0},
                              {'0','i',';',',',0},   
                              {':','o',5,'.',0}, //5 = <-
                              {'-','p',6,'/',13}}; //6 = ->, 13 = return
byte keyMapShift[ROW][COL] = {{'#','Q','D','Z','S'},     
                              {'$','W','F','X','"'},     
                              {'%','E','G','C','!'},
                              {'&','R','H','V',27},  //27 = esc
                              {'\'','T','J','B','A'},
                              {'(','Y','K','M',' '},
                              {')','U','L','M',0},
                              {'0','I','+','<',0},   
                              {'*','O',5,'>',0}, //5 = <-
                              {'=','P',6,'?',13}}; //6 = ->, 13 = return                              
byte keyMapCtrl[ROW][COL] = {{'3','q','d','z','s'},     
                              {'4','w','f','x','2'},     
                              {'5','e',7,'c','1'}, //7=bell
                              {'6','r','h','v',27}, //27 = esc
                              {'7','t','j','b','a'},
                              {'8','y','k','^',' '},
                              {'9','u','l','m',0},
                              {'0','i',';',',',0},   
                              {':','o',5,'.',0}, //5 = <-
                              {'-','@',6,'/',13}}; //6 = ->, 13 = return        

unsigned long lastSentSameCharTimeMs = 0;
unsigned long currentPressTimeMs;
byte rowIndex[ROW] = {0,0,0,0,0,0,0,0,0,0};
byte colIndex = 0;
byte colData  = 1;
byte oldKeyboardStatus[ROW*COL];
byte currentKeyboardStatus[ROW*COL];
int timeToWaitBeforeMultiplePress;
byte charPressedIndex = 255;
byte totalPressed = 0;
byte totalReleased = 0;
byte totalNewPressed = 0;
byte oldCharPressedIndex = 255;
#define minTimeBetweenSameKeyPressMsFirstTime  550  //solo per tasto tenuto premuto la prima volta
#define minTimeBetweenSameKeyPressMsOtherTimes 40   //le battute successive vanno piu' veloci

void setup() {
  pinMode(RPT, INPUT);
  pinMode(KEY_SHIFT, INPUT);
  pinMode(KEY_CTRL, INPUT);
  pinMode(R0, INPUT);
  pinMode(R1, INPUT);
  pinMode(R2, INPUT);
  pinMode(R3, INPUT);
  pinMode(R4, INPUT);
  pinMode(R5, INPUT);
  pinMode(R6, INPUT);
  pinMode(R7, INPUT);
  pinMode(R8, INPUT);
  pinMode(R9, INPUT);
  pinMode(STROBE,     OUTPUT);
  pinMode(OUT_SERIAL_D,   OUTPUT);
  pinMode(OUT_SERIAL_CLK, OUTPUT);
  pinMode(COL_SERIAL_D,   OUTPUT);
  pinMode(COL_SERIAL_CLK, OUTPUT);
  
  timeToWaitBeforeMultiplePress = minTimeBetweenSameKeyPressMsFirstTime;
  memset(oldKeyboardStatus,0,sizeof(oldKeyboardStatus));
  memset(currentKeyboardStatus,0,sizeof(currentKeyboardStatus));
    
  Serial.begin(9600);
}

void loop() {
  //decodifica
  sendDataToShiftRegister(colData);
  if (decodeRow()) {
    for (byte i = 0; i < ROW; i++) {
      if (rowIndex[i]) {
        currentKeyboardStatus[i*COL+colIndex] = 1;
      }
    }
  }
  //identificazione tasti
  if (colIndex == COL-1) {  //fine scansione colonna  
    currentPressTimeMs = millis();
    for (byte i = 0; i < ROW*COL; ++i) {
      if (currentKeyboardStatus[i] == 1) {
          ++totalPressed;
          if (charPressedIndex == 255) {
            charPressedIndex = i;
          }
          if (oldKeyboardStatus[i] == 0) { //solo un tasto premuto alla volta
            ++totalNewPressed;
            charPressedIndex = i;
         }
      } else if (oldKeyboardStatus[i] == 1) {
        ++totalReleased;
      }
    }
    //invio
    if (charPressedIndex != oldCharPressedIndex) { //nuovo
      ouputKeyboardData(charPressedIndex);
      lastSentSameCharTimeMs = currentPressTimeMs;
      timeToWaitBeforeMultiplePress = minTimeBetweenSameKeyPressMsFirstTime; 
    } else if (currentPressTimeMs - lastSentSameCharTimeMs > timeToWaitBeforeMultiplePress) { 
      ouputKeyboardData(charPressedIndex);
      lastSentSameCharTimeMs = currentPressTimeMs;    
      timeToWaitBeforeMultiplePress = minTimeBetweenSameKeyPressMsOtherTimes; //prossima volta piu' veloce    
    }
    
    //reset
    oldCharPressedIndex = charPressedIndex;
    if (totalReleased) {      
      charPressedIndex = 255;
      timeToWaitBeforeMultiplePress = minTimeBetweenSameKeyPressMsFirstTime; 
    }
    if (totalPressed == 0 || totalNewPressed == 0) { //con totalNewPressed si fa tornare totalReleased a 0 il prossimo giro in caso di pressione multiple
      totalReleased = 0;
    }
    totalPressed = 0;
    totalNewPressed = 0;
    memcpy(oldKeyboardStatus,currentKeyboardStatus,ROW*COL);
    memset(currentKeyboardStatus,0,ROW*COL);
  }
  colData  = colData < 16 ? colData << 1 : 1; //incrementa colonna per prossima iterazione (16=2^(COL-1))
  colIndex = colIndex+1 >= COL ? 0 : colIndex+1;
}

/*
 * Colonne attive basse, in uscita dallo shift register 
 * solo uno 0 ruotante
 * 00000001
 * 00000010
 * 00000100
 * 00001000
 * 00010000
 */
void sendDataToShiftRegister(byte data) {  
  byte mask = 0b10000000;
  for (byte i = 0; i < 8; i++) {
     if (data & mask) { digitalWrite(COL_SERIAL_D, LOW);  }
     else             { digitalWrite(COL_SERIAL_D, HIGH); }
     doColumnClock();
     mask >>= 1;
  }
}

byte decodeRow() {
  byte rowActived = 0; 
  memset(rowIndex,0,ROW); 
  if (digitalRead(R0) == LOW) { rowIndex[0] = 1; ++rowActived; }
  if (digitalRead(R1) == LOW) { rowIndex[1] = 1; ++rowActived; }
  if (digitalRead(R2) == LOW) { rowIndex[2] = 1; ++rowActived; }
  if (digitalRead(R3) == LOW) { rowIndex[3] = 1; ++rowActived; }
  if (digitalRead(R4) == LOW) { rowIndex[4] = 1; ++rowActived; }
  if (digitalRead(R5) == LOW) { rowIndex[5] = 1; ++rowActived; }
  if (digitalRead(R6) == LOW) { rowIndex[6] = 1; ++rowActived; }
  if (digitalRead(R7) == LOW) { rowIndex[7] = 1; ++rowActived; } 
  if (digitalRead(R8) == LOW) { rowIndex[8] = 1; ++rowActived; } 
  if (digitalRead(R9) == LOW) { rowIndex[9] = 1; ++rowActived; } 
  return rowActived;
}

void ouputKeyboardData(byte index) {
  if (index == 255) { return; }
  byte data;
  if (digitalRead(KEY_CTRL) == LOW) {
    data = *(keyMapCtrl[0]+index);
  } else if (digitalRead(KEY_SHIFT) == LOW) {
    data = *(keyMapShift[0]+index);
  } else {
    data = *(keyMap[0]+index);
  }

  //debug
  Serial.print((char)data);

  digitalWrite(OUT_SERIAL_D, LOW); 
  doOutClock();
  //shift register
  if (data & 0b01000000) { digitalWrite(OUT_SERIAL_D, HIGH); } else { digitalWrite(OUT_SERIAL_D, LOW); }
  doOutClock();
  if (data & 0b00100000) { digitalWrite(OUT_SERIAL_D, HIGH); } else { digitalWrite(OUT_SERIAL_D, LOW); }
  doOutClock();
  if (data & 0b00010000) { digitalWrite(OUT_SERIAL_D, HIGH); } else { digitalWrite(OUT_SERIAL_D, LOW); }
  doOutClock();
  if (data & 0b00001000) { digitalWrite(OUT_SERIAL_D, HIGH); } else { digitalWrite(OUT_SERIAL_D, LOW); }
  doOutClock();
  if (data & 0b00000100) { digitalWrite(OUT_SERIAL_D, HIGH); } else { digitalWrite(OUT_SERIAL_D, LOW); }
  doOutClock();
  if (data & 0b00000010) { digitalWrite(OUT_SERIAL_D, HIGH); } else { digitalWrite(OUT_SERIAL_D, LOW); }
  doOutClock();
  if (data & 0b00000001) { digitalWrite(OUT_SERIAL_D, HIGH); } else { digitalWrite(OUT_SERIAL_D, LOW); }
  doOutClock();

  //strobe
  digitalWrite(STROBE, HIGH);
  delayMicroseconds(OUTPUT_STROBE_DURATION_US);
  digitalWrite(STROBE, LOW);  
}

