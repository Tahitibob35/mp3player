#include <BY8001.h>
#include <LiquidCrystal.h>
#include "SoftwareSerial.h"

byte playChar[8] = {
  0b10000,
  0b11000,
  0b11100,
  0b11110,
  0b11100,
  0b11000,
  0b10000,
  0b00000
};

byte stopChar[8] = {
  0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b00000,
  0b00000
};

byte pauseChar[8] = {
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b00000
};

byte soundChar[8] = {
  0b00001,
  0b00011,
  0b00111,
  0b11111,
  0b11111,
  0b00111,
  0b00011,
  0b00001
};

byte sleepChar[8] = {
  0b01110,
  0b10101,
  0b10101,
  0b10101,
  0b10101,
  0b10011,
  0b10001,
  0b01110
};

//LiquidCrystal lcd( 2 , 3 , 4 , 5 , 6 , 7 );
LiquidCrystal lcd(8, 7, 6, 5, 4, 3);

#define RXPIN 9
#define TXPIN 10
#define PLAYPIN A5
#define STOPPIN A2
#define PREVPIN A3
#define NEXTPIN A1
#define VOLINCPIN A0
#define VOLDECPIN 13
#define BACKLIGHTPIN 2
#define SLEEPPIN 1

SoftwareSerial swSerial( RXPIN , TXPIN );
//BY8X0116P audioController( swSerial );
BY8001 mp3;

byte volume = 15;
byte mp3status = 0;
unsigned long backlightpreviousMillis = 0;
const long backlightinterval = 30000;
byte generalmode = 0;         // 0->ON; 1->LCDOFF; 2->SLEEP
unsigned long generalmodepreviousMillis = 0;
const long generalmodeinterval = 60000;
int previouscurrentelapsedtime = 0;
unsigned long sleep = 0;
unsigned long timeout = 0;
unsigned long now = 0;
byte sleepmode = false;
unsigned long previousmillis = 0;
byte rollover = 1;
  
void setup() {
  // Serial setup
  //Serial.begin( 9600 );
  //Serial.println("Setup...");
  swSerial.begin( 9600 );
  mp3.setup( swSerial );
  delay( 500 );
  
  // LCD setup
  lcd.begin( 16 , 2 );
  lcd.createChar( 0 , stopChar );
  lcd.createChar( 1 , playChar );
  lcd.createChar( 2 , pauseChar );  
  lcd.createChar( 3 , sleepChar );
  lcd.createChar( 4 , soundChar );  
  
  lcd.setCursor( 9 , 1 );
  lcd.write( byte ( 4 ) );
  lcd.setCursor( 13 , 1 );
  lcd.write( byte ( 3 ) );

  //BY8001 setup
  mp3.setVolume( volume );
  updatevolume( );
  mp3.stopPlayback( );
  updatestatus( );
  mp3.switchDevice( 1 );
  updatetrackname( );
  mp3.setLoopPlaybackMode( 0 );

  pinMode( BACKLIGHTPIN , OUTPUT );
  digitalWrite( BACKLIGHTPIN , HIGH );
}

void loop() {
  //Serial.println("Loop...");

  if ( generalmode == 0) {    
    updatesleep( );

    //Backlight
    unsigned long backlightcurrentMillis = millis( );
    if ( backlightcurrentMillis - backlightpreviousMillis >= backlightinterval ) {
      // save the last time you blinked the LED
      backlightpreviousMillis = backlightcurrentMillis;
      digitalWrite( BACKLIGHTPIN , LOW );
    }
  
    if ( mp3status == 0 ) {
      unsigned long generalmodecurrentMillis = millis( );
      if ( generalmodecurrentMillis - generalmodepreviousMillis >= generalmodeinterval ) {
        // save the last time you blinked the LED
        generalmodepreviousMillis = generalmodecurrentMillis;
        lcd.clear( );
        //Serial.println("Sleep....");
        mp3.toggleStandbyMode();
        generalmode = 2;
      }
    }
  
    //digitalWrite( 13,  !digitalRead( 13 ) );
  
    // Update elapsed time
    if ( mp3status == 1 ) {
  
      int currentelapsedtime = mp3.getElapsedTrackPlaybackTime( );
      lcd.setCursor( 2 , 1 );
      lcd.print( currentelapsedtime );  
      if ( currentelapsedtime < previouscurrentelapsedtime ) {     // track changed
        updatetrackname( );
      }
      previouscurrentelapsedtime = currentelapsedtime;
    }
  }

  // Check Play button
  
  if ( digitalRead( PLAYPIN ) == HIGH ) {
    mp3wakeup( );
    powerbacklight( );
    switch ( mp3status ) {
      case 0:
        mp3.play( );
        mp3status = 1;
        previouscurrentelapsedtime = 0;
        break;      
      case 1:
        mp3.pause( );
        mp3status = 2;
        break;
      case 2:
        mp3.play( );
        mp3status = 1;
        break;      
    }
    updatestatus( );
    delay( 300 );
    while (digitalRead( PLAYPIN ) == HIGH );
  }

  if ( digitalRead( STOPPIN ) == HIGH ) {
    mp3wakeup( );
    powerbacklight( );
    //Serial.println( "Stop" );
    mp3.stopPlayback( );
    lcd.setCursor( 2 , 1 );
    lcd.print( "       " );  
    mp3status = 0;
    updatestatus( );
    delay( 300 );
    while (digitalRead( STOPPIN ) == HIGH );
    generalmodepreviousMillis = millis();
  }

  if ( digitalRead( VOLDECPIN ) == HIGH ) {    
    mp3wakeup( );
    mp3.setVolume( --volume );
    updatevolume( );
    delay( 300 );
    while (digitalRead( VOLDECPIN ) == HIGH );
  }

  if ( digitalRead( VOLINCPIN ) == HIGH ) {    
    mp3wakeup( );
    mp3.setVolume( ++volume );
    updatevolume( );
    delay( 300 );
    while (digitalRead( VOLINCPIN ) == HIGH );
  }

  if ( digitalRead( NEXTPIN ) == HIGH ) {   
    mp3wakeup( );
    powerbacklight( );
    mp3.nextTrack( );
    updatetrackname( );
    while (digitalRead( NEXTPIN ) == HIGH );
    mp3status = 1;
    updatestatus( );
    clearcurrenttime( );
    generalmodepreviousMillis = millis();
  }

  if ( digitalRead( PREVPIN ) == HIGH ) {    
    mp3wakeup( );
    powerbacklight( );
    mp3.previousTrack( );
    updatetrackname( );
    while (digitalRead( PREVPIN ) == HIGH );
    mp3status = 1;
    updatestatus( );
    clearcurrenttime( );
    generalmodepreviousMillis = millis();
  }



  if ( digitalRead( SLEEPPIN ) == HIGH ) {  
    mp3wakeup( );
    powerbacklight( );
    sleep = sleep + 30 * 1000.0 * 60 ;
    //Serial.print( "New sleep :");
    //Serial.println( sleep ) ;
    
    now = millis( );
    timeout = now + sleep;
    
    if ( sleep > ( 90 * 60 * 1000.0 ) ) {
      //Serial.println( "Retour à 0" );
      sleep = 0;
      sleepmode = false;
    }
    else {  
      //Test overflow
      if ( timeout < now ) {
        //One rollover is needed
        rollover = 0;
        //Serial.println( "Rollover enabled" );
      }
      else {
        //No rollover is needed
        rollover = 1;
        //Serial.println( "Rollover disabled" );
      }       
      sleepmode = true;      
    }
  
    //Serial.print( "timeout = ");
    //Serial.println( timeout );
    generalmodepreviousMillis = millis();
    updatesleep( );
    while (digitalRead( SLEEPPIN ) == HIGH );
  }

  now = millis( );
  if ( sleepmode ) {
    if ( ( now > timeout ) && rollover ) {
      mp3.stopPlayback( );
      mp3status = 0;
      updatestatus( );
      generalmodepreviousMillis = millis();
      //Serial.println( "STOP" );
      sleepmode = false;
      //delay( 2000 );
      sleep = 0;
    }
  }
  if ( sleepmode ) sleep = (timeout - now);
  unsigned long currentMillis = millis();
  if ( currentMillis < previousmillis ) {     //Millis rollover
    rollover = 1;
  }
  previousmillis = currentMillis;
  
}

void updatevolume( ) {  
  lcd.setCursor( 10 , 1 );
  lcd.print( volume );
}

void updatestatus( ) {  
  lcd.setCursor( 0 , 1 );
  lcd.write( mp3status );
}

void clearcurrenttime( ) {
  lcd.setCursor( 1 , 1 );
  lcd.print( "       " );  
}

void updatetrackname( ) {  
  lcd.setCursor( 0 , 0 );
  lcd.print( mp3.getFileNameCurrentTrack( ) );  
}

void powerbacklight( ) {
  digitalWrite( BACKLIGHTPIN , HIGH );
  backlightpreviousMillis = millis();
  if ( generalmode == 1) {
    lcd.setCursor( 9 , 1 );
    lcd.write( byte ( 4 ) );
    lcd.setCursor( 13 , 1 );
    lcd.write( byte ( 3 ) );
    updatevolume( );
    updatetrackname( );
  }
  generalmode = 0;
}

void updatesleep( ) {
  lcd.setCursor( 14 , 1 );
  lcd.print( ( sleep + 59000 ) / 60 / 1000 );
}

void mp3wakeup( ) {
  if ( generalmode == 2 ) {    
    //Serial.println("Wakeup !!!!");
    mp3.toggleStandbyMode();
    delay( 800 );
    generalmode = 1;
  }
}

