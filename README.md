# 7" TFT Forecasting Weather Station

Welcome to the 7" TFT Forecasting Weather Station

Within this build I've taken coding from various other projects online and incorporating it into my own. At present time during this publication all sensor data had been running for three days without a hickup. Currently weather forecast coding taken from G6EJD/ESP32-Weather-Forecaster repository has been running for 4hrs with out a lockup. Once everything has run for a few days i'm going to try and get the moon phase coding to properly show the moon picture that goes with the calculated phase.

Required Parts:

Arduino Mega 2560

Sainsmart 7" TFT display with adapter

DS1332 RTC

BME280

DHT22

Logic Level converter



Version 1.0a


Uses basic pressure readings from BMP280 without compensating for altitude and temperature.


<a data-flickr-embed="true"  href="https://www.flickr.com/photos/164087731@N07/40141880503/in/album-72157678697572758/" title="V1.0a 7&quot; TFT Weather Station/Forecaster"><img src="https://farm8.staticflickr.com/7923/40141880503_76a490502c_c.jpg" width="800" height="450" alt="V1.0a 7&quot; TFT Weather Station/Forecaster"></a>


Version 1.0c

Added - Temperature and Altitude compensation for pressure readings

Changed - Layout of bottom information screen to include more weather information as well as future weather icons.

Still in the air is the Moonphase Pictures switching as per the coding of the current phase.

<a data-flickr-embed="true"  href="https://www.flickr.com/photos/164087731@N07/32164694337/in/album-72157678697572758/" title="V1.0c Weather Station/Forecaster"><img src="https://farm8.staticflickr.com/7906/32164694337_f5d84ef878_c.jpg" width="800" height="450" alt="V1.0c Weather Station/Forecaster"></a>


Version 1.0d

Removed Bar graph

Added Zambretti Forecast calculations for predictions

Still in the air is the Moonphase Pictures switching as per the coding of the current phase.

Possible layout redesign with the addition of esp8266 to get current weather data from OpenWeather Maps and actual weather data from your sensors , addition of SunRise/SunSet times based on GPS location.

Zambretti Source code was taken from [G6EJD's ESP32 Weather Forecaster](https://github.com/G6EJD/ESP32_Weather_Forecaster_TN061) and tweaked to work with saving data to an SD card.

<a data-flickr-embed="true"  href="https://www.flickr.com/photos/164087731@N07/47179511951/in/dateposted-public/" title="Version.1.0d"><img src="https://farm8.staticflickr.com/7924/47179511951_6e93ec107d_c.jpg" width="800" height="450" alt="Version.1.0d"></a>


Update 2/26/2019

After playing hell with the WEMOS Mega w/ESP8266 I finally got the Open Weather Maps API finally working for my location. Currently it's just the Current Forecast but I'll be working more and more with the code to include more than one future forecast I currently get. 

<a data-flickr-embed="true"  href="https://www.flickr.com/photos/164087731@N07/46310268435/in/album-72157678697572758/" title="Open Weather Map finally streaming current weather information on the WEMOS Mega w/esp8266"><img src="https://farm8.staticflickr.com/7867/46310268435_4b10acf12d_c.jpg" width="800" height="450" alt="Open Weather Map finally streaming current weather information on the WEMOS Mega w/esp8266"></a>
