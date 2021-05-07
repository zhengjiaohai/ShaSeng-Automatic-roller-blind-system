#include <SimpleDHT.h>
#include <CheapStepper.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// for DHT11,
//      VCC: 5V or 3V
//      GND: GND
//      DATA: D4
byte temperature = 0;
byte humidity = 0;
int pinDHT11 = D4;
bool isMove = false;
//创建SimpleDHT11类，通过调用这个类来读取DHT11
SimpleDHT11 dht11(pinDHT11);

CheapStepper stepper(D0, D1, D2, D3);
boolean moveClockwise = true;

const char *ssid = "wifi";                  // wifi名称
const char *password = "123123123";         // wifi密码
const char *mqtt_server = "broker.emqx.io"; // mqtt服务器地址，如本地ip192.168.31.22

WiFiClient espClient;
PubSubClient client(espClient);

const byte OutPin = D5; // 需要控制的接口

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++)
  {
    char receivedChar = (char)payload[i];
    Serial.print(receivedChar); // 打印mqtt接收到消息

    if (receivedChar == '1')
    { // 收到消息是 '1' 设置D5为高电平
      digitalWrite(OutPin, HIGH);
      isMove = true;
    }
    if (receivedChar == '0') // 收到消息是 '0' 设置D5为低电平
    {
      digitalWrite(OutPin, LOW);
      isMove = false;
    }
  }
  Serial.println();
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client"))
    {
      Serial.println("connected");
      client.subscribe("ledStatus"); // 订阅 'ledStatus' 这个topic
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
void setup()
{
  stepper.setRpm(20);
  Serial.begin(115200);

  // Serial.print(stepper.getRpm()); // get the RPM of the stepper
  // Serial.print(" rpm = delay of ");
  // Serial.print(stepper.getDelay()); // get delay between steps for set RPM
  Serial.print(" microseconds between steps");
  Serial.println();
  client.setServer(mqtt_server, 1883); // 连接mqtt
  client.setCallback(callback);        // 设置回调，控制电机

  pinMode(OutPin, OUTPUT);
}

void loop()
{
  //   int err = SimpleDHTErrSuccess;
  //   if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
  //   {
  //     Serial.print("Read DHT11 failed, err=");
  //     Serial.println(err);
  //     delay(1000);
  //     return;
  //   }
  //   dht11.read(&temperature, &humidity, NULL);

  // if ((int)temperature >= 25)
  // {
  //   stepper.moveCCW(102400);
  // }
  // delay(1000);
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  delay(1000);
}