/*
 * AutoMixer.ino
 *
 * Created: 04.12.2018 13:27:23
 * Author: kosty
 */ 

#include <TFT.h>
#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <GyverEncoder.h>

//-------------------------------------
// PINS CONF
#if defined(__LGT8FX8P48__)
// CONF	__LGT8FX8P48__
#else
// pins TFT display
#define DISP_CS   D22 // (7)SELECT CHIP
#define DISP_DC  D16 // (A2) //D10 // RS(6) -> MOSI (D0)
#define DISP_RST  D17 // (A3) //D9 // D9 -> DISP RESET

// pins SD card
#define SD_CS E2 // select chip SD

// key board
#define BUTTON_MENU D14
#define BUTTON_PAUSE D15
//#define  BUTTON_CW C2
//#define  BUTTON_CCW C3

// encoder
#define ENC_1 D3
#define ENC_2 D19 // A5
#define ENC_B D5

// thermo sensor
#define  DS_D A4 // PC4 -> A4

// motor
#define M_R D7 // PE2
#define M_L D8 // PE0
#define M_PWM D10//D6
#define M_EN1 D2
#define M_EN2 D4

// FAN (������)
#define FAN_PWM D9
#define FAN_ON D6//D10

// END __LGT8FX8__
#endif
//*****

/* CONF ATMega328p
//#if defined(__AVR_ATmega328P__ || __AVR_ATmega328p__)
#define DISP_CS   10 //
#define DISP_DC  9 //
#define DISP_RST  8 //

// pins SD card
#define SD_CS D7

// key board
#define BUTTON_MENU A0
#define BUTTON_PRG A1
#define  BUTTON_CW A2
#define  BUTTON_CCW A3

// encoder
#define ENC_1 D3
#define ENC_2 A5
#define ENC_B D5

// thermo sensor
#define  DS_D A4

// motor
#define M_R D0
#define M_L D1
#define M_PWM D6
#define M_EN1 D2
#define M_EN2 D4

// END CONF ATMega328p
//#endif
//*****
*/
// END PINS CONF
//-------------------------------------
//*************************************


// INITS ------------------------------

// display
TFT TFTscreen = TFT(DISP_CS, DISP_DC, DISP_RST);

// ds18b20
OneWire ds(DS_D);

// Encoder
Encoder encoder(ENC_1, ENC_2, ENC_B);

// ������ ������ ������� �����������
#define  LAST 5000
unsigned long currentTime = 0;

// STRUCT
struct Menu {
	bool selected = false, cursor = false;
	String name;
};

// END STRUCT

//////////////////////////////////////////////////////////////////////////
// COLORE

const uint16_t colorLine = TFTscreen.Color565(255, 255, 255), // default white
		colorText = TFTscreen.Color565(255, 0, 0), // blue
		colorRPM = TFTscreen.Color565(0, 255, 0), // green
		colorTemp = TFTscreen.Color565(0, 0, 255), // red
		colorState = TFTscreen.Color565(240, 0, 255),
		colorBackground = TFTscreen.Color565(0, 0, 0);

// END COLORE

Menu menu[6];

const uint8_t margin = 4,
			sector = TFTscreen.width()/2,
			sectorH = 16,
			sectorM = sector+margin;

volatile uint8_t size = 0; // ���������� �������� ������������ ���������� 7 �������� ����������� ����������
volatile signed char select = 0; // �������� �� ������� ������

volatile bool up = false; // ���� ���������� ������
volatile float temp = 0.0; // ������� ����������� ���������

// ����������� ���������
const float REDUCTOR_RATION = 21.3;

// ������� ����������
const char *dir = "/prog";

File file;

unsigned long timePrg, // ����� ������� ����. ����������
			timePID; // ����� ������������ ���
double rpmProg, // �������� ���. ��������
	rpmPrev, // ���������� ���������� ��������
	rpmD, // ������� �������
	rpmErr, // ������ ��������������� ������
	rpmErrP; // ������ ��������������� ����������

int manualSpeed = 0, // �������� ������� ������������ � manual ������
	manualMaxSpeed = 410; // ������������ ���������� �������� ������� ������������ � manual ������
bool manualStopedFlag = false; // ���� ��������� ���������� manual �����

//////////////////////////////////////////////////////////////////////////
/************************************************************************/
/*                       ��������� ��� ����������                       */
/************************************************************************/

// ����. ��� ����������
const double Kp = 0.5,
			Ki = 0.2,	
			Kd = -0.2;

// �������� ��������� ��� ����������
byte maxOut = 255,
	minOut = 40;

//////////////////////////////////////////////////////////////////////////

/// ������������ ����
void showMenu(Menu *mn = NULL, uint8_t s = 0) {
	TFTscreen.textSize(1);

	for(uint8_t i = 0; i <= s; i++) {
		TFTscreen.setCursor(margin, (i>0) ? sectorH*(i+1)+margin : sectorH+margin);
		if (mn[i].cursor == true)
		{
			if (menu[i].selected) {
				TFTscreen.fillRect(margin-1, (i>0) ? sectorH*(i+1) : sectorH+1, sector-margin, (i>0) ? 14 : 14 , colorLine);
				TFTscreen.setTextColor(colorText, colorLine);
			} else {
				TFTscreen.fillRect(margin-1, (i>0) ? sectorH*(i+1) : sectorH+1, sector-margin, (i>0) ? 14 : 14 , colorText);
				TFTscreen.setTextColor(colorLine, colorText);
			}
		} else {
			TFTscreen.fillRect(margin-1, (i>0) ? sectorH*(i+1) : sectorH+1, sector-margin, (i>0) ? 14 : 14 , colorBackground);
			TFTscreen.setTextColor(colorLine, colorBackground);
		}
		TFTscreen.print(mn[i].name.c_str());
	}
}

/// �������� ������ ������
void initPrograms(File dir) {
	
	File entry =  dir.openNextFile();

	// ���������� �� 7-� ������
	for (uint8_t i = 0; i <= 6; i++)
	{
		if (!entry)
		{
			break;
		}
		menu[i].name = entry.name();
		menu[i].cursor = (i == 0) ? true : false;
		entry =  dir.openNextFile();
		size++;
	}

	entry.close();
}

/// ������������� �����
void motorSTOPED() {
	//menu[select].selected = !(menu[select].selected);
	up = true;
	
	showRPM(0.0);
	showStateText("Mot. stop!");
	digitalWrite(M_L, LOW);
	digitalWrite(M_R, LOW);
	analogWrite(M_PWM, 0);
}

void fanOn() {
	digitalWrite(FAN_ON, HIGH);
	analogWrite(FAN_PWM, 255);
}

void fanOff() {
	digitalWrite(FAN_ON, LOW);
	analogWrite(FAN_PWM, 0);
}

/// ��������� ��������� �� �����
void startProgram(const char *pathProg) {
	showMenu(menu, size);
	String path = (String)dir+"/"+String(menu[select].name).c_str();
	file = SD.open(path);

	char pch[15];
	String buff;
	bool pause = false;

	if (file) {
		while(file.available()) {
			
			if (!pause)
			{
				analogWrite(M_PWM, 0);
				delay(500);

				buff = file.readStringUntil('\n');
				buff.toCharArray(pch, 15);

				// �������� ��������� �� �����
				rpmProg = atof(strtok(pch, " "));
				timePrg = atoi(strtok(NULL, " "));

				showStateMot(timePrg, rpmProg);
				timePrg = millis() + timePrg*1000;

				rpmProg = (rpmProg < 0)? ((rpmProg*(-1) > 220)? rpmProg-150 : rpmProg): ((rpmProg > 220)? rpmProg+150 : rpmProg);
			}

			if (rpmProg > 0)
			{
				digitalWrite(M_L, LOW);
				digitalWrite(M_R, HIGH);
			} else {
				digitalWrite(M_L, HIGH);
				digitalWrite(M_R, LOW);
			}

			if (!digitalRead(BUTTON_PAUSE))
			{
				timePrg += millis();
				pause = false;
			}

			// ��������� ���������
			while ((millis() < timePrg) && (pause != true))
			{
				encoder.tick();
				showTemp();

				if (encoder.isPress())
				{
					menu[select].selected = !(menu[select].selected);
					motorSTOPED();
					file.close();
					return;
				}

				if (!digitalRead(BUTTON_PAUSE))
				{
					timePrg -= millis();
					analogWrite(M_PWM, 0);
					pause = true;
					break;
				}

				if (temp >= 50)
				{
					fanOn();
				} else {
					fanOff();
				}

				rpmD = getRPM();
				analogWrite(M_PWM, regulPID(maxOut, minOut, Kp, Ki, Kd));
				showRPM(rpmD);
			}
		}
	} else {
		showStateText("FileErr");
	}
	
	menu[select].selected = !(menu[select].selected);
	motorSTOPED();
	file.close();
}

/// ���������� �������������
void setup()
{
// init
	TFTscreen.begin();
	TFTscreen.background(colorBackground);

	if (!SD.begin(SD_CS))
	{
		showStateText("Not SD");
	} else {
		// ��������� ���������� "prog"
		file = SD.open(dir);
		// ��������� ���������� �� �����
		initPrograms(file);
	}
	
	encoder.setType(false);
	
	// ������� ������
	pinMode(M_EN1, INPUT);
	pinMode(M_EN2, INPUT);
	// ������������ ����� ��� ���������� �������
	pinMode(M_R, OUTPUT);
	pinMode(M_L, OUTPUT);
	// ��� ������
	pinMode(M_PWM, OUTPUT);

	// ��� ���������� �� ��������� ������
	pinMode(FAN_ON, OUTPUT);
	// ��� ������
	pinMode(FAN_PWM, OUTPUT);

	// ������ �������
	pinMode(BUTTON_PAUSE, INPUT);
	// ������ ����
	pinMode(BUTTON_MENU, INPUT);

	up = true; // �������� �����
}

void manualMode() {
	
	encoder.tick();
	
	if (encoder.isFastR())
	{
		manualSpeed-=10;
		manualSpeed = (manualSpeed>0)? ((manualSpeed+manualMaxSpeed)%manualMaxSpeed) : ((manualSpeed-manualMaxSpeed)%(-manualMaxSpeed));
		up = true;
	}

	if (encoder.isFastL())
	{
		manualSpeed+=10;
		manualSpeed = (manualSpeed>0)? ((manualSpeed+manualMaxSpeed)%manualMaxSpeed) : ((manualSpeed-manualMaxSpeed)%(-manualMaxSpeed));
		up = true;
	}

	if (encoder.isPress())
	{
		if (manualSpeed > 0)
		{
			digitalWrite(M_L, LOW);
			digitalWrite(M_R, HIGH);
		} else {
			digitalWrite(M_L, HIGH);
			digitalWrite(M_R, LOW);
		}

		rpmProg = manualSpeed;

		while(1) {
      showTemp();
			encoder.tick();
			rpmD = getRPM();
			analogWrite(M_PWM, regulPID(maxOut, minOut, Kp, Ki, Kd));
			showRPM(rpmD);

			if (encoder.isPress())
			{
				motorSTOPED();
				break;
			}
		}
	}

	if(up) {
		showStateMot(0, manualSpeed);
		up = false;
	}

	if (!digitalRead(BUTTON_PAUSE))
	{
		up = true;
		manualStopedFlag = false;
		showStateText("Man. close");
	}
}

/// ���������� ���������� �������� ��������� (��/���)
double getRPM() { 
	byte en1 = M_EN1,
		en2 = M_EN2;
	byte prevState = digitalRead(en1);
	prevState += digitalRead(en2) << 1;
	byte state;

	long rpm = 0;
	unsigned long delta = millis() + 500;

	while(millis() < delta) {
		state = digitalRead(en1);
		state += digitalRead(en2) << 1;
		if (prevState != state)
		{
			if (state == 0b11) {
				if (prevState == 0b10) {
					rpm--;
				} else if (prevState == 0b01) {
					rpm++;
				}
			}
		}
		prevState = state;
	}

	return (rpm*120)/(REDUCTOR_RATION*7);
}

/// ��� ���������
double regRPMInt;
byte regulPID(byte maxOut, // ������������� �������� ��������
			byte minOut, // ����������� �������� ��������
			double Kp, // ����. ���������������� ������������
			double Ki, // ����. ������������ ������������
			double Kd) // ����. ��������������� ������������
{
	rpmErr = ((rpmProg < 0) ? rpmProg*(-1) : rpmProg) - ((rpmD < 0) ? rpmD*(-1) : rpmD);

	double regRPMPr, regRPMDif, resultPID;
	
	regRPMInt = regRPMInt + rpmErr * Ki; // ������������ ����� ����������
	if (regRPMInt > maxOut)
	{
		regRPMInt = maxOut;
	}
	if (regRPMInt < minOut)
	{
		regRPMInt = minOut;
	}

	regRPMPr = rpmErr * Kp; // ����������������� ����� ����������
	regRPMDif = (rpmErr - rpmErrP) * Kd; // ���������������� ���� ����������

	rpmErrP = rpmErr; // ���������� ���������� ������
	
	resultPID = regRPMInt + regRPMPr + regRPMDif;

	if (resultPID > maxOut)
	{
		resultPID = maxOut;
	}

	if (resultPID < minOut)
	{
		resultPID = minOut;
	}

	return resultPID;
}

void loop()
{
	encoder.tick();

	digitalWrite(FAN_ON, HIGH);
	analogWrite(FAN_PWM, 255);

	if (encoder.isPress())
	{		
		menu[select].selected = !(menu[select].selected);

		if (menu[select].selected)
		{
			startProgram(menu[select].name.c_str());
		}
		showMenu(menu, size);
		up = true;
	}
		
	if (encoder.isFastR() && !menu[select].selected)
	{
		menu[select].cursor = false;
		--select;
		select = (select+size)%size;
		menu[select].cursor = true;
		up = true;
	}

	if (encoder.isFastL() && !menu[select].selected)
	{
		menu[select].cursor = false;
		++select;
		select = (select+size)%size;
		menu[select].cursor = true;
		up = true;
	} 

	if (!digitalRead(BUTTON_MENU))
	{
		up = true;
		manualStopedFlag = true;
		while(manualStopedFlag) {
			manualMode();
		}
	}

	// ��������� ���������
	if (up)
	{
		up = false;
		mainWindow();
		showMenu(menu, size);
	}

	if (millis() > currentTime)
	{
		up = true;
		showTemp();
		currentTime = millis() + LAST;
	}
}

/// ������������ ������� ����
void mainWindow() {
	// main line
	TFTscreen.drawLine(sector, margin, sector, TFTscreen.height()-margin, colorLine);
	
	// title line
	TFTscreen.drawLine(margin, sectorH, sector-margin, sectorH, colorLine);
	TFTscreen.drawLine(sectorM, sectorH, (TFTscreen.width()-margin), sectorH, colorLine);

	// table line
	TFTscreen.drawLine(sectorM, sectorH*3+7, (TFTscreen.width()-margin), sectorH*3+7, colorLine);
	TFTscreen.drawLine(sectorM, sectorH*6, (TFTscreen.width()-margin), sectorH*6, colorLine);
	//TFTscreen.stroke(colorText);
	TFTscreen.textSize(2);
	// Title table
	TFTscreen.setTextColor(colorText, colorBackground);
	TFTscreen.setCursor(margin, 0);
	TFTscreen.print(F("Select"));
	TFTscreen.setCursor(sectorM, 0);
	TFTscreen.print(F("RPM"));
}

/// ���������� ���������� �������� � ���.
void showRPM(double rpm) {
	TFTscreen.fillRect(sectorM, sectorH+8, sector, sectorH*2-1, colorBackground);
	TFTscreen.setCursor(sectorM, sectorH+8);
	TFTscreen.setTextColor(colorRPM, colorBackground);
	TFTscreen.setTextSize(1);
	TFTscreen.print(rpm);
}

/// ��������� � ���������� �����������
void showTemp(){
	temp = getTemp(); // ��������� �����������
	TFTscreen.fillRect(sectorM, sectorH*6+12, sector, sectorH*2, colorBackground);
	TFTscreen.setCursor(sectorM, sectorH*6+12);
	TFTscreen.setTextColor(colorTemp, colorBackground);
	TFTscreen.setTextSize(1);
	TFTscreen.print(temp);
	TFTscreen.print("t");
}

/// ���������� ����� ������
void showStateMot(unsigned int timePrg, double rpm){
	//TFTscreen.stroke(colorState);
	TFTscreen.fillRect(sectorM, sectorH*margin, sector, sectorH*2, colorBackground);
	TFTscreen.setTextColor(colorState, colorBackground);
	TFTscreen.setCursor(sector+8, sectorH*margin);
	TFTscreen.setTextSize(1);
	TFTscreen.print("s: ");
	TFTscreen.print(timePrg);
	TFTscreen.setCursor(sector+8, sectorH*margin+10);
	TFTscreen.print("rpm: ");
	TFTscreen.print(rpm);
}

/// ���������� ��������� � ���� ������
void showStateText(const char* state){
	TFTscreen.fillRect(sectorM, sectorH*margin, sector, sectorH*2, colorBackground);
	TFTscreen.setTextColor(colorState, colorBackground);
	TFTscreen.setCursor(sector+8, sectorH*margin);
	TFTscreen.setTextSize(1);
	TFTscreen.print(state);
}

/// �������� ������ � ������� �����������
float getTemp() {
	
	// ���������� ����������� �� ������� DS18b20
	byte data[1]; // ����� ��� �������� �����������

	ds.reset();
	ds.write(0xCC); // ���� ������� DS18b20 ������� ���������� ����� �� ������. � ����� ������ ������ ���� ����������
	ds.write(0x44); // ���� ������� DS18b20 ������� �������� �����������. ���� �������� ����������� �� ��� �� �������� - ������ ��� ������� �� ���������� ������

	delay(500);

	ds.reset(); // ������ ��������� �������� �������� ���������� �����������
	ds.write(0xCC);
	ds.write(0xBE); // ������ �������� ��� �������� ��������� �� ��������� �����������

	// �������� � ��������� �����
	data[0] = ds.read(); // ������ ������� ���� �������� �����������
	data[1] = ds.read(); // � ������ �������

	// ��������� �������� ��������:
	//    - ������ "���������" ��������,
	//    - ����� �������� ��� �� �����������, �������������� ����������� ����������� (��� 12 ��� �� ��������� - ��� 0,0625)
	return ((data[1] << 8) | data[0]) * 0.0625;
}
