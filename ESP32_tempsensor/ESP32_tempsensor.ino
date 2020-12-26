/* Temperature sensing on ESP32 board
 * Returns temperature in celsius
 * using MCP9700AE 3 pin temp sensor
 */
const int pin = A0;  // Using pin 0 on ESP32 board
float voltage = 3.26; // as measured with multimeter from 3V out pin
float temp = 0; // initialize temperature

void setup() {
  // initialization:
  Serial.begin(115200); 
  Serial.println("setup complete");
}

void loop() {
  // read the analog in value:
  temp = getTempC();

  // print the results to the serial monitor:
  Serial.print("Temperature (deg C): ");                       
  Serial.println(temp);   
  delay(1000);                     
}

float getTempC()
{
  // vout = tc*ta + v0
  // (vout-v0)/tc = ta
  // based on data sheet
  // https://www.sparkfun.com/datasheets/DevTools/LilyPad/MCP9700.pdf

  // read sensor pin
  float temp = analogRead(pin);

  // adjust for 12-bit ADC
  temp = temp*voltage/4095;

  // return adjusted temp reading
  return (temp-.500)*100;
}
