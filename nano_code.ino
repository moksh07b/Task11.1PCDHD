#include <Servo.h>  // Include the Servo library for servo motor control

Servo myServo1; // Servo motor for control gate 1
Servo myServo2; // Servo motor for control gate 2
Servo myServo3; // Servo motor for sorting gate
Servo myServo4; // Servo motor for ripe category gate
Servo myServo5; // Servo motor for reject category gate

const int irPin = 4;    // IR sensor to detect the tomato at the first position
const int irPin2 = 10;  // IR sensor to detect the tomato passing through the ripe gate
const int irPin3 = 11;  // IR sensor to detect the tomato passing through the unripe gate
const int irPin4 = 12;  // IR sensor to detect the tomato passing through the reject gate

const int OPEN_POS = 0;    // Angle for the open position of the servo
const int CLOSE_POS = 90;  // Angle for the closed position of the servo

#define DIR_PIN 7   // Direction pin for stepper motor
#define STEP_PIN 8  // Step (pulse) pin for stepper motor

void setup() {
  // Set pin modes for direction and stepper pins, as well as IR sensors
  pinMode(DIR_PIN, OUTPUT);
  pinMode(irPin, INPUT);
  pinMode(irPin2, INPUT);
  pinMode(irPin3, INPUT);
  pinMode(irPin4, INPUT);
  pinMode(STEP_PIN, OUTPUT);

  Serial.begin(9600);  // Initialize serial communication

  // Attach servos to pins
  myServo1.attach(2);
  myServo2.attach(3);
  myServo3.attach(5);
  myServo4.attach(6);
  myServo5.attach(9);

  // Set initial positions for servos
  myServo1.write(OPEN_POS);
  myServo2.write(OPEN_POS);
  myServo3.write(CLOSE_POS);
  myServo4.write(OPEN_POS);  // Default ripe gate open
  myServo5.write(CLOSE_POS); // Default reject gate closed
}

int irValue = HIGH;    // Initial IR sensor value
String command;        // Command received from serial input

void loop() {
  irValue = digitalRead(irPin);  // Read value from IR sensor

  // If IR sensor detects a tomato
  if (irValue == LOW) {
    myServo1.write(CLOSE_POS); // Close the entry gate
    myServo2.write(CLOSE_POS); // Close another gate for control
    int commandCount = 0;      // Counter for rotate commands
    Serial.println("CLICK!");  //Command sent for pi purpose

    // Wait for three "rotate" commands before rotating
    while (commandCount < 3) {
      if (Serial.available() > 0) {
        // Read the incoming command from Serial
        command = Serial.readStringUntil('\n');
        
        // Check if the command is "rotate"
        if (command == "rotate") {
          commandCount++;  // Increment count if "rotate" command is received
          Serial.print("Rotate command count: ");
          Serial.println(commandCount);
          rotateMotor(135, HIGH);  // Rotate the motor by 135 degrees
          Serial.println("Rotation complete.");
          delay(1000);  // Delay to allow motor to rotate
        }
        Serial.println("CLICK!");  
      }
    }

    delay(500);
    rotateMotor(380, LOW);  // Return motor to initial position
    
    // Wait for command to classify the tomato
    while (Serial.available() <= 0) {}
    command = Serial.readStringUntil('\n');

    // Command to classify the tomato
    if (command == "1") {
      myServo4.write(CLOSE_POS);  // Close ripe gate
    }
    if (command == "2") {
      // Do nothing if tomato is unripe
    }
    if (command == "3") {
      myServo5.write(OPEN_POS);   // Open reject gate
    }

    myServo3.write(OPEN_POS);  // Open sorting gate for the tomato to pass

    // Wait for the tomato to pass through a gate
    while (true) {
      int val1 = digitalRead(irPin2);
      int val2 = digitalRead(irPin3);
      int val3 = digitalRead(irPin4);

      // Break loop if the tomato passes any of the leeway IR sensors
      if (val1 == LOW || val2 == LOW || val3 == LOW) {
        break;
      }
    }

    delay(500);

    // Reset servos to initial positions
    myServo1.write(OPEN_POS);
    myServo3.write(CLOSE_POS);
    myServo2.write(OPEN_POS);
    myServo4.write(OPEN_POS);
    myServo5.write(CLOSE_POS);
  }
}

// Function to rotate the stepper motor by a given degree in a specified direction
void rotateMotor(int degrees, int dir) {
  int stepsPerRevolution = 200;  // Number of steps for a 360-degree rotation
  int steps = (stepsPerRevolution * degrees) / 360;  // Steps needed for given degrees

  digitalWrite(DIR_PIN, dir);  // Set motor direction

  // Generate pulses on STEP pin t
