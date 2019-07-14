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

# 参考文献
* 電空レギュレーター原理
  * https://www.ckd.co.jp/company/giho/pdf/Vol04/CKDgh_Vol4_02.pdf
* ESP32
  * Timer 
    * http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc202/doc21105.html
    * https://garretlab.web.fc2.com/arduino/lab/esp32_timer/index.html#timerBegin