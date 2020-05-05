void setup() {
  pinMode(32, OUTPUT);
  Serial.begin(9600);
}
// the loop function runs over and over again forever
void loop() {
  digitalWrite(32, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(100);  // wait for a second
  digitalWrite(32, LOW);  // turn the LED off by making the voltage LOW
  delay(100);  // wait for a second



  float temperature = 0.0;   // stores the calculated temperature
  int sample;                // counts through ADC samples
  float ten_samples = 0.0;   // stores sum of 10 samples
  // take 10 samples from the MCP9700
  for (sample = 0; sample < 10; sample++) {
    // convert A0 value to temperature
    temperature = (float)analogRead(12);
    temperature = temperature / 0.01;
    // sample every 0.1 seconds
    delay(1000);
    // sum of all samples
    ten_samples = ten_samples + temperature;
  }
  // get the average value of 10 temperatures
  temperature = ten_samples / 10.0;
  // send temperature out of serial port
  Serial.print(temperature);
  Serial.println(" deg. C");
  ten_samples = 0.0;
}
