#include <Wire.h>
#include <DHT.h>
#include <DHT_U.h>
#include <BH1750.h>

#define DHTPIN 13
#define DHTTYPE DHT22

// Objek DHT
DHT dht(DHTPIN, DHTTYPE);

// Variabel global untuk Kecepatan Angin
volatile byte rpmcount;
volatile unsigned long last_micros;
unsigned long timeold;
unsigned long timemeasure = 10.00;
int countThing = 0;
int GPIO_pulse = 15;
float rpm, rotasi_per_detik;
float kecepatan_kilometer_per_jam;
float kecepatan_meter_per_detik;
volatile boolean flag_anemometer = false;

// Variabel global untuk Arah Angin
#define RX2 16 
#define TX2 17
String data, arah_angin, s_angin;
int a, b;

// Variabel global untuk Curah Hujan
const int pin_interrupt = 14;
long int jumlah_tip = 0;
long int temp_jumlah_tip = 0;
float curah_hujan = 0.00;
float milimeter_per_tip = 0.70;
volatile boolean flag_hujan = false;

// Objek BH1750
BH1750 lightMeter;

// ISR Kecepatan Angin
void ICACHE_RAM_ATTR rpm_anemometer() {
  flag_anemometer = true;
}

// ISR Curah Hujan
void ICACHE_RAM_ATTR hitung_curah_hujan() {
  flag_hujan = true;
}

void setup() {
  Serial.begin(9600);

  Wire.begin(32, 33);

  // Setup Kecepatan Angin
  pinMode(GPIO_pulse, INPUT_PULLUP);
  digitalWrite(GPIO_pulse, LOW);
  attachInterrupt(digitalPinToInterrupt(GPIO_pulse), rpm_anemometer, RISING);
  rpmcount = 0;
  rpm = 0;
  timeold = 0;

  // Setup Arah Angin
  Serial2.begin(9600, SERIAL_8N1, RX2, TX2);

  // Setup Curah Hujan
  pinMode(pin_interrupt, INPUT);
  attachInterrupt(digitalPinToInterrupt(pin_interrupt), hitung_curah_hujan, FALLING);

  // Setup DHT
  dht.begin();

  // Setup BH1750
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 Error!");
    while (1);
  }

  Serial.println("System Initialized!");
}

void loop() {
  // ================== Kecepatan Angin ==================
  if (flag_anemometer) {
    if (long(micros() - last_micros) >= 5000) {
      rpmcount++;
      last_micros = micros();
    }
    flag_anemometer = false;
  }

  if ((millis() - timeold) >= timemeasure * 1000) {
    countThing++;
    detachInterrupt(digitalPinToInterrupt(GPIO_pulse));
    rotasi_per_detik = float(rpmcount) / float(timemeasure);
    kecepatan_meter_per_detik = ((-0.0181 * (rotasi_per_detik * rotasi_per_detik)) + 
                                (1.3859 * rotasi_per_detik) + 1.4055);
    if (kecepatan_meter_per_detik <= 1.5) {
      kecepatan_meter_per_detik = 0.0;
    }
    kecepatan_kilometer_per_jam = kecepatan_meter_per_detik * 3.6;
    timeold = millis();
    rpmcount = 0;
    attachInterrupt(digitalPinToInterrupt(GPIO_pulse), rpm_anemometer, RISING);
  }

  // ================== Arah Angin ==================
  if (Serial2.available()) {
    data = Serial2.readString(); 
    a = data.indexOf("*"); 
    b = data.indexOf("#"); 
    s_angin = data.substring(a + 1, b);
    if (s_angin.equals("1")) arah_angin = "utara";
    if (s_angin.equals("2")) arah_angin = "timur laut";
    if (s_angin.equals("3")) arah_angin = "timur";
    if (s_angin.equals("4")) arah_angin = "tenggara";
    if (s_angin.equals("5")) arah_angin = "selatan";
    if (s_angin.equals("6")) arah_angin = "barat daya";
    if (s_angin.equals("7")) arah_angin = "barat";
    if (s_angin.equals("8")) arah_angin = "barat laut";
  }

  // ================== Curah Hujan ==================
  if (flag_hujan) {
    curah_hujan += milimeter_per_tip;
    jumlah_tip++;
    delay(500);
    flag_hujan = false;
  }
  curah_hujan = jumlah_tip * milimeter_per_tip;

  // ================== Pembacaan DHT ==================
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  // ================== Pembacaan BH1750 ==================
  float lux = lightMeter.readLightLevel();

  // ================== Menampilkan Data ke Serial ==================
  Serial.println("=======================================");
  Serial.print("Kecepatan Angin: ");
  Serial.print(kecepatan_meter_per_detik, 1);
  Serial.println(" m/s");

  Serial.print("Arah Angin: ");
  Serial.println(arah_angin);

  Serial.print("Curah Hujan: ");
  Serial.print(curah_hujan, 1);
  Serial.println(" mm");

  Serial.print("Suhu: ");
  Serial.print(suhu, 1);
  Serial.println(" C");

  Serial.print("Kelembapan: ");
  Serial.print(kelembapan, 1);
  Serial.println(" %");

  Serial.print("Intensitas Cahaya: ");
  Serial.print(lux, 1);
  Serial.println(" lx");
  Serial.println("=======================================");

  delay(2000); // Tunggu untuk update berikutnya
}
