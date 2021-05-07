#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>
#include <CheapStepper.h>
#include <Arduino.h>
#include <sstream>
#include <iostream>
#include <string>
using namespace std;
#define max 100
// for DHT11,
//      VCC: 5V or 3V
//      GND: GND
//      DATA: D4

//连接的wifi名称
const char *ssid = "wifi";
//连接的WiFi密码
const char *password = "123123123";
//MQTT服务器地址
const char *mqtt_broker = "broker.emqx.io";
//MQTT服务器端口
const int mqtt_port = 1883;
//MQTT订阅的主题
const char *topic = "zjh";
//温度，Byte类型
byte temperature = 0;
//湿度，Byte类型
byte humidity = 0;
//运动标记
bool MoveFlag = false;
//MQTT服务器发来的字符
String InputPayLoad;
//发送给服务器的通知
String OutputPayload;
//定义温度输入引脚
int pinDHT11 = D4;
//定义光强输入引脚
int PinTEMT6000 = A0;

int PinMoveOutput = D8;
//创建SimpleDHT11类，通过调用这个类来读取DHT11
SimpleDHT11 dht11(pinDHT11);

//创建CheapStepper步进电机驱动类，定义输出引脚为D0, D1, D2, D3
CheapStepper stepper(D0, D1, D2, D3);

WiFiClient espClient;
PubSubClient client(espClient);
//int类型转string类型，typedef basic_string<char>
string to_String(int n)
{
  int m = n;
  char s[max];
  char ss[max];
  int i = 0, j = 0;
  if (n < 0) // 处理负数
  {
    m = 0 - m;
    j = 1;
    ss[0] = '-';
  }
  while (m > 0)
  {
    s[i++] = m % 10 + '0';
    m /= 10;
  }
  s[i] = '\0';
  i = i - 1;
  while (i >= 0)
  {
    ss[j++] = s[i--];
  }
  ss[j] = '\0';
  return ss;
}
//温度模块错误处理
void DHT11_err()
{
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
  {
    Serial.print("Read DHT11 failed, err=");
    Serial.println(err);
    delay(1000);
    return;
  }
}
//步进电机驱动模块封装
void move(bool MoveFlag)
{
  if (MoveFlag)
  {
    for (int s = 0; s < 2048; s++)
    {
      stepper.step(true);
    }
  }
  else
  {
    stepper.stop();
  }
}
//bool转String
String bool_to_String(bool flag)

{
  if (flag)
  {
    return "true";
  }
  else
  {
    return "false";
  }
}
//消息回调函数，需要注意：函数名称任意但是形参固定
void callback(char *topic, byte *payload, unsigned int length)
{
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    InputPayLoad = (char)payload[i];
  }
  Serial.println();
  Serial.println("-----------------------");
}
void sc()
{
  dht11.read(&temperature, &humidity, NULL);
  //读取光强
  int LightIntensity = analogRead(PinTEMT6000);
  //
  int msgLen = 17 + 15 + 13 + 17; //

  msgLen += bool_to_String(PinMoveOutput).length();
  msgLen += to_String((int)temperature).length();
  msgLen += to_String((int)humidity).length();
  msgLen += to_String(LightIntensity).length();

  client.beginPublish(topic, msgLen, true);
  client.print(" 光照强度：");
  client.print(LightIntensity);
  client.print(" LUX 温度：");
  client.print(to_String((int)temperature).c_str());
  client.print("*C   湿度 ");
  client.print((int)humidity);
  client.print("%  运动状态是: ");
  client.print(bool_to_String(PinMoveOutput));
  client.endPublish();
}

void setup()
{
  void DHT11_err();
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);

    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //设置连接服务器与端口
  client.setServer(mqtt_broker, mqtt_port);

  client.setCallback(callback);
  while (!client.connected())
  {
    Serial.println("Connecting to public emqx mqtt broker.....");
    if (client.connect("esp8266-client"))
    {
      Serial.println("Public emqx mqtt broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  // publish and subscribe
  client.publish(topic, "hello emqx");
  client.subscribe(topic);
  sc();
  // while (true)
  // {
  //   sc();
  //   delay(1000);
  // }
}

void loop()
{

  //心跳消息
  client.loop();
  //读取温湿度
  dht11.read(&temperature, &humidity, NULL);
  //读取光强
  int LightIntensity = analogRead(A0);
  sc();
  delay(500);
  if ((int)temperature >= 30 || LightIntensity > 500)
  {
    //MoveFlag = true;
    PinMoveOutput = 1;
  }
  else
  {
    PinMoveOutput = 0;
    //MoveFlag = false;
  }
  if (InputPayLoad == "1")
  {
    PinMoveOutput = 1;
    //MoveFlag = true;
  }
  if (InputPayLoad == "0")
  {
    PinMoveOutput = 0;
    //MoveFlag = false;
  }
  InputPayLoad = "";
  Serial.println(PinMoveOutput);
  move(MoveFlag);
  digitalWrite(D7,HIGH);
}