#include "DFPlayerMini_Fast.h"
#include <MFRC522.h>
#include <SPI.h>
#include <NfcAdapter.h>
#include <Regexp.h>
#include <ButtonDebounce.h>

#define RXD2 16
#define TXD2 17
#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16
#define SS_PIN 21
#define RST_PIN 22
#define NDEF_USE_SERIAL true

int RFID_SDA = 21;
int RFID_RST = 22;
int RFID_SCK = 18;
int RFID_MOSI = 23;
int RFID_MISO = 19;

int bd = 115200;
int serial2_baud = 9600;
int playPauseButtonPin = 2;
int volumeButtonPin = 4;
int isPaused = 0;
int volume = 10;

MFRC522 mfrc522(RFID_SDA, RFID_RST);
NfcAdapter nfc = NfcAdapter(&mfrc522);
DFPlayerMini_Fast myMP3;
MatchState ms;
ButtonDebounce playPauseButton(playPauseButtonPin, 250);
ButtonDebounce volumeButton(volumeButtonPin, 250);

void setup() {
  Serial.begin(bd);
  while(!Serial);
  initButtons();
  initMp3Player();
  initNfcReader();
}

void loop() {
  buttonsLoop();
  nfcLoop();
  delay(100);
}

void buttonsLoop() {
  playPauseButton.update();
  volumeButton.update();
}

void handlePlayPauseButton(const int state) {
  if (state == HIGH) {
    if (isPaused) {
      myMP3.resume();
      isPaused = 0;
    } else {
      myMP3.pause();
      isPaused = 1;
    }
  }
}

void handleVolumeButton(const int state) {
  if (state == HIGH) {
    if (volume == 30) {
      volume = 10;
    } else {
      volume = volume + 10;
    }
    myMP3.volume(volume);
  }
}

void initButtons() {
  pinMode(playPauseButtonPin, INPUT_PULLUP);
  pinMode(volumeButtonPin, INPUT);
  playPauseButton.setCallback(handlePlayPauseButton);
  volumeButton.setCallback(handleVolumeButton);
}

void initNfcReader() {
  SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SDA);
  mfrc522.PCD_Init();
  nfc.begin(true);
  delay(4);
  Serial.println("RFID Ready");
}

void initMp3Player() {
  Serial2.begin(serial2_baud, SERIAL_8N1, RXD2, TXD2);
  while(!Serial2);
  myMP3.begin(Serial2);
  myMP3.volume(volume);
  Serial.println("Mp3 ready");
  delay(100);
  myMP3.playFromMP3Folder(1);
}

void nfcLoop() {

  if (nfc.tagPresent()) {

    // extract the NDEF text
    NfcTag tag = nfc.read();
    NdefMessage msg = tag.getNdefMessage();
    NdefRecord record = msg.getRecord(0);
    char* ndefText = (char*)record.getPayload();

    // extract the track number from the raw ndef text
    ms.Target(ndefText);
    char result = ms.Match("(%d+)");
    char buf [100];
    if (result == REGEXP_MATCHED) {
      String trackNumber = ms.GetMatch(buf);
      Serial.println("Playing: " + trackNumber);
      myMP3.playFromMP3Folder(trackNumber.toInt());
    }
  }
}