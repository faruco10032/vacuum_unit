# vacuum unit
吸引デバイスの空気圧調整

# 環境
* マイコン
  * ESP32 DevKit
* valve
  * SC 3701 PML, SHENZHEN SKOOCOM ELECTRONIC
  * SC415GF, SC0526GF, SHENZHEN SKOOCOM ELECTRONIC
* エアーポンプ
  * MIS-2503-015V
* FET
  * k2232
* 気圧センサ
  * MIS-2503

# 概要
目標気圧値に達したらバルブを閉めて気圧を維持

気圧を開放する際に外気バルブを開放

## SimpleVacuumについて
目標気圧をこえたときにバルブを開放すると急激に気圧が下がるため解放と吸引を繰り返してしまい振動が発生する．

そのため目標気圧に達したらバルブを閉めて停止させる．

また開放したいときは目標気圧が変化した時なのでシリアル入力を受け付けたら開放できるようにする．

## 各コードの概要
* AirRegulateByValve
  * このコードに最終的に集約する
* ElectroAirRegulator
  * 基本的な気圧調整を行うコード
* ElectroAirRegulator_MultiSenor
  * 複数のセンサと吸引点に対応したコード
* ElectroAirRegulator_MultiSenorForDemo
  * 複数吸引点でUnityとの通信を実装したコード
  * Unityとの高速な通信のために吸引点と気圧を3byteで受信
* ElectroAirRegulator_SinglePumpforMultiSuctionPoint
  * 一つのポンプで多数の吸引点に対応するためのテスト
* ElectroAirRegulator_SinglePumpforMultiSuctionPoint02
  * 一つのポンプで多数の吸引点に対応するコードをUnityとの通信に対応させたもの
* SimpleVacuum
  * 基本的な気圧調整コード
  * Processingと対応して一箇所の吸引を行う実験のためのコード


# ESP32のピン番号割当

| IO  | Name      | discription            |
| --- | --------- | ---------------------- |
| 36  | SENSOR_VP | air pressure sensor 01 |
| 39  | SENSOR_VN | air pressuer sensor 02 |
| 34  | IO04      | air pressure sensor 03 |
| 35  | IO25      | air pressuer sensor 04 |
| 32  | IO26      | air pressure sensor 05 |
| 33  | IO27      | air pressuer sensor 06 |
| 25  | IO25      | suction valve 01       |
| 26  | IO26      | rerease valve 01       |
| 27  | IO27      | suction valve 02       |
| 14  | IO14      | rerease valve 02       |
| 13  | IO13      | suction valve 03       |
| 23  | IO23      | rerease valve 03       |
| 22  | IO22      | suction valve 04       |
| 21  | IO21      | rerease valve 04       |
| 19  | IO19      | suction valve 05       |
| 18  | IO18      | rerease valve 05       |
| 17  | IO17      | suction valve 06       |
| 16  | IO16      | rerease valve 06       |

# 参考文献
* 電空レギュレーター原理
  * https://www.ckd.co.jp/company/giho/pdf/Vol04/CKDgh_Vol4_02.pdf
* ESP32
  * Timer 
    * http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc202/doc21105.html
    * https://garretlab.web.fc2.com/arduino/lab/esp32_timer/index.html#timerBegin