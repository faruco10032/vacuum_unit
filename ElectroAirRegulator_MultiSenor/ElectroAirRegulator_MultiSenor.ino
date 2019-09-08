/* 
2019/07/14

ESP32
pin parts
32  valve01:suction valve
33  valve02:rerease valve
36  air pressure sensor 01
39  air pressuer sensor 02
*/

#define PULSE_SUCTION_WIDTH 3000 //吸引の間隔
#define PULSE_RELEACE_WIDTH 2000 //排気の間隔
#define RANGE 5 //目標気圧との誤差許容範囲

#define SUCTION_POINT_NUM 2 //吸引点の数
//#define SENSOR_PIN 36 //気圧センサ
#define VALVE_NUM 2 //バルブの数
int VALVE_PIN[VALVE_NUM] = {32,33};
int SENSOR_PIN[] = {36,39};

double val; //
double raw_pres; //raw air pressure value
double adraw_pres; //生データを平滑化するための一時的な加算場所，平滑化したあとのデータ
#define LOOP 10 // 生データを時間平滑化するためのループ回数
int loop_time; //ループ回数
double loop_raw_pres[LOOP]; //時間平滑化のためのデータ保存場所 　
int aim_pres = -300; //目標気圧

bool suction_flag = false; //目標気圧より気圧が高いときに吸引を行う
bool timer_flag=false; //タイマー割り込みを行うフラグ

//Timer関連セットアップ
hw_timer_t * timer = NULL;
//portMUX_TYPE型の変数で、メインループとISRで共用変数を変更する時の同期処理に使用
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
//メインループとISRで共用するためのカウンターです。コンパイラーの最適化によって消去されないように、volatile宣言をします
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

//気圧センサの値を読み込む関数
void read_sensor_value(int sensor_num){
    val = analogRead(SENSOR_PIN[sensor_num]);
  raw_pres = val/4095*3.3/2.7*(-1000); //単位 mbar (1mbar = 1hPa)
  
  //計測値の平均化
  loop_time=(loop_time+1)%LOOP;
  loop_raw_pres[loop_time]=raw_pres;
  
  int i;
  adraw_pres = 0;
  for(i=0;i<LOOP;i++){
    adraw_pres += loop_raw_pres[i];
  }
  adraw_pres = adraw_pres/LOOP;
}

//
void change_valve(){
  if(!suction_flag){
    if(adraw_pres>=aim_pres+RANGE){//目標気圧+RANGE以上なら吸う
      digitalWrite(VALVE_PIN[0] , LOW);
      digitalWrite(VALVE_PIN[1] , LOW);
  //    Serial.print("SUCTION");Serial.print("\t");
    }else if(adraw_pres>=aim_pres-RANGE){//目標気圧±RANGE以内なら停止
      digitalWrite(VALVE_PIN[0] , HIGH);
      digitalWrite(VALVE_PIN[1] , LOW);
//------------------------------------------------------------------------------
      suction_flag = true;//バルブを止めて気圧調整するときはコメントアウトを解除
//      Serial.print("STOP");Serial.print("\t");
//------------------------------------------------------------------------------
    }else{//目標気圧-RANGE以下なら排気
      digitalWrite(VALVE_PIN[0] , HIGH);
      digitalWrite(VALVE_PIN[1] , HIGH);
  //    Serial.print("OUT");Serial.print("\t");
    }
  }
}

//バルブを開放して気圧を開放．
void releace(){
  digitalWrite(VALVE_PIN[0] , HIGH);
  digitalWrite(VALVE_PIN[1] , HIGH);
  suction_flag = false;
}

//割り込み処理関数
void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
//  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  
  //以下割り込み処理
  read_sensor_value(1);
//  val = analogRead(SENSOR_PIN);
//  raw_pres = val/4095*3.3/2.7*(-1000); //単位 mbar (1mbar = 1hPa)
//  
//  //計測値の平均化
//  loop_time=(loop_time+1)%LOOP;
//  loop_raw_pres[loop_time]=raw_pres;
//  
//  int i;
//  adraw_pres = 0;
//  for(i=0;i<LOOP;i++){
//    adraw_pres += loop_raw_pres[i];
//  }
//  adraw_pres = adraw_pres/LOOP;
  
  //排気パルス実行
  if((isrCounter%(PULSE_SUCTION_WIDTH+PULSE_RELEACE_WIDTH)) < PULSE_SUCTION_WIDTH){
    change_valve();
  }else{
    releace();
  }
}



void setup() {
  //change pin mode
  Serial.begin(9600);
  for(int i=0;i<VALVE_NUM;i++){
    pinMode(VALVE_PIN[i] , OUTPUT);
  }

  //timer set up
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more　info).
  timer = timerBegin(0, 80, true);
  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  // timer 1ms
  timerAlarmWrite(timer, 1000, true);
  // Start an alarm
  timerAlarmEnable(timer);
  
}

void loop() {

  int inByte;
  if ( Serial.available() ) {
    inByte = Serial.read();
    if(inByte == 'A'){
//      Serial.print(aim_pres);
//      Serial.print(',');
//      Serial.print(adraw_pres);
//      Serial.print(',');
      Serial.println(raw_pres);
    }else{
    
      switch (inByte) {
//        case 'A' : 
//          Serial.print(adraw_pres);
//          Serial.print(',');
//          Serial.println(raw_pres);
//          break;
        case 'j' : 
          aim_pres += 25;
          break;
        case 'k' : 
          aim_pres -= 25;
          break;
        case 'l' : 
          aim_pres += 5;
          break;
        case 'm' : 
          aim_pres -= 5;
          break;
      }
    }
  }
  Serial.print(aim_pres);
  Serial.print("\t");
  Serial.println(raw_pres);
  
}
