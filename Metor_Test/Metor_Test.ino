#define METOR_FL 12
#define METOR_BL 13
#define METOR_FR 15
#define METOR_BR 14

void setup() {

  pinMode(METOR_FL, OUTPUT) ;
  pinMode(METOR_BL, OUTPUT) ;
  pinMode(METOR_FR, OUTPUT) ;
  pinMode(METOR_BR, OUTPUT) ;

}



void loop() {

  digitalWrite(METOR_FR, LOW);
  digitalWrite(METOR_BR, HIGH);

  delay(1000);

  digitalWrite(METOR_BR, LOW);
  digitalWrite(METOR_FR, HIGH);

  delay(1000);

}
