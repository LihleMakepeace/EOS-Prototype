#include <Encoder.h>
Encoder Enc(2, 3);

const int commandMaxLength = 10;
char command[commandMaxLength];
int commandReceivedLength = 0;
int commandAsIntegers[3];
long Enc_Ubound;
long Enc_Lbound;
long Enc_Midpoint;
long Enc_reading;
long Motor_Goal;
long Total_Diff;
long Reduction_Step;
long Reduction_Step1;
long joystick_Reduction_Step = 11000;
unsigned long lastDebounceTime = 0;
unsigned long DebounceDelay = 20;
int limit_switch = 0;
int Encoder_boundrySet = 0;
int limit_switchState;
int lastlimit_switchState = HIGH;
int joystick;
int joystick_diff;
int joystick_mid = 511;
int joystick_Dzone = 10;
int command_stetup;
int PWM;
int Controlled_Motor;
float PWM_Percent;

void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);
    pinMode(4, INPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, INPUT);
    pinMode(8, INPUT);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
    analogWrite(5, 65);
    digitalWrite(6, LOW);
    Encoder_setup();
}
void loop()
{
    /*
      Based on the input the Arduino receives from the momentary switch
      the following code tells the Arduino to either use the Joystick or
      the Serial monitor commands
    */
    command_stetup = digitalRead(7);
    if (command_stetup == HIGH)
    {
        Read_Joystick_Command();
        Move_Motor();
    }
    if (command_stetup == LOW)
    {
        Read_Serial_Commamd();
        if (commandAsIntegers[0] == 2)
        {
            Move_Motor();
        }
    }
}

void Read_Joystick_Command()
{
    joystick = analogRead(A8);
    if (joystick < (joystick_mid - joystick_Dzone))
    {
        Motor_Goal = Enc_Lbound;
        joystick_diff = (joystick_mid + joystick_Dzone) - joystick;
        Serial.print("joystick_diff: ");
        Serial.println(joystick_diff);
        Joy_Diff_to_PWM();
    }
    else if (joystick > (joystick_mid + joystick_Dzone))
    {
        Motor_Goal = Enc_Ubound;
        joystick_diff = joystick - (joystick_mid + joystick_Dzone);
        Serial.print("joystick_diff: ");
        Serial.println(joystick_diff);
        Joy_Diff_to_PWM();
    }
    else
    {
        PWM = 0;
    }
}
void Joy_Diff_to_PWM()
{
    Enc_reading = Enc.read();
    long Difference = abs(Motor_Goal - Enc_reading);
    if (Difference > joystick_Reduction_Step)
    {
        PWM = map(joystick_diff, 0, 250, 0, 255);
        PWM = constrain(PWM, 0, 255);
        Serial.print("PWM: ");
        Serial.println(PWM);
    }
    else if (Difference < joystick_Reduction_Step)
    {
        PWM = 55;
        Reduction_Step1 = joystick_Reduction_Step;
        Serial.print("PWM: ");
        Serial.println(PWM);
    }
}

void Read_Serial_Commamd()
{
    if (Serial1.available())
    {
        char characters = Serial1.read();
        if (characters == '<')
        {
            commandReceivedLength = 0; // The begining of a new coomand
        }
        else if (characters == '>')
        {

            Serial.print("Command Received: "); // The end of the command
            for (int i = 0; i <= commandReceivedLength; i++)
            {
                Serial.print(command[i]);
            }
            Serial.println();
            commandtoIntegers();
        }
        else
        {
            command[commandReceivedLength] = characters;
            commandReceivedLength++;
        }
    }
}
void commandtoIntegers()
{
    /*
    This command will split the array of characters on commas
    each sub-array of characters will be converted to integers
    those integers are stored in an array (commandAsIntegers)
    */

    int j = 0;
    char tempo[commandMaxLength];
    int k = 0;
    for (int i = 0; i <= commandReceivedLength; i++)
    {
        if (command[i] == ',' || i == commandReceivedLength)
        {
            commandAsIntegers[j] = atoi(tempo);
            j++;
            k = 0;
            for (int X = 0; X <= commandMaxLength; X++)
            {
                tempo[X] = " ";
            }
        }
        else
        {
            tempo[k] = command[i];
            k++;
        }
    }
    if (!((commandAsIntegers[0] == 1) || (commandAsIntegers[0] == 2)))
    {
        Serial.println("WRONG MOTOR CHOICE");
        return;
    }
    if ((commandAsIntegers[1] < 0) || (commandAsIntegers[1] > 60))
    {
        Serial.println("SPEED OUT OF RANGE");
        return;
    }
    if ((commandAsIntegers[2] < -180) || (commandAsIntegers[2] > 180))
    {
        Serial.println("BEARING OUT OF RANGE");
        return;
    }

    Controlled_Motor = commandAsIntegers[0];
    PWM = map(commandAsIntegers[1], 0, 60, 0, 255);
    PWM = constrain(PWM, 30, 255);
    Motor_Goal = map(commandAsIntegers[2], -180, 180, Enc_Lbound, Enc_Ubound);
    Total_Diff = Motor_Goal - Enc_reading;
    Total_Diff = abs(Total_Diff);
    Reduction_Step = map(commandAsIntegers[1], 0, 60, 0, 10000);
    Reduction_Step1 = Reduction_Step;

    Serial.print("Controlled Motor: ");
    Serial.println(Controlled_Motor);
    Serial.print("PWM: ");
    Serial.println(PWM);
    Serial.print("Motor Goal Position: ");
    Serial.println(Motor_Goal);
    Serial.print("Total Differnce: ");
    Serial.println(Total_Diff);
    Serial.print("Reduction_Step: ");
    Serial.println(Reduction_Step);

    if (Total_Diff < Reduction_Step)
    {
        PWM = 50;
        Serial.print("PWM: ");
        Serial.println(PWM);
    }
    memset(command, 0, sizeof(command));
}

void Move_Motor()
{
    if (Enc_reading != Motor_Goal)
    {
        Enc_reading = Enc.read();
        if (Enc_reading <= Motor_Goal)
        {
            analogWrite(6, PWM);
            digitalWrite(5, LOW);
        }
        if (Motor_Goal <= Enc_reading)
        {
            analogWrite(5, PWM);
            digitalWrite(6, LOW);
        }
        if (Enc_reading == Motor_Goal)
        {
            digitalWrite(5, LOW);
            digitalWrite(6, LOW);
        }
        Slow_Down();
    }
}
void Slow_Down()
{
    long Difference = abs(Motor_Goal - Enc_reading);
    if (Difference <= Reduction_Step)
    {
        if (Difference == Reduction_Step1)
        {
            PWM_Percent = (((Difference * 100) / Reduction_Step));
            PWM = map(PWM_Percent, 30, 100, 0, PWM);
            PWM = constrain(PWM, 30, 255);
            Reduction_Step1 = Reduction_Step1 - (Reduction_Step1 / 20);
            Serial.print("PWM_Percent: ");
            Serial.println(PWM_Percent);
            Serial.print("PWM: ");
            Serial.println(PWM);
            Serial.print("Reduction_Step1: ");
            Serial.println(Reduction_Step1);
        }
    }
}

void Encoder_setup()
{
    while (Encoder_boundrySet <= 2)
    {
        limit_switch = digitalRead(4);
        if (limit_switch != lastlimit_switchState)
        {
            lastDebounceTime = millis();
        }

        if ((millis() - lastDebounceTime) > DebounceDelay)
        {
            if (limit_switch != limit_switchState)
            {
                limit_switchState = limit_switch;
                if (limit_switchState == LOW)
                {
                    Encoder_boundrySet++;
                    if (Encoder_boundrySet == 1)
                    {
                        digitalWrite(5, LOW);
                        analogWrite(6, 65);
                        Enc_Lbound = Enc.read();
                        Enc_Ubound = Enc_Lbound + pow(2, 16);
                        Enc_Midpoint = (Enc_Lbound + (Enc_Ubound - Enc_Lbound) / 2);
                        Serial.print("Enc_Lbound: ");
                        Serial.println(Enc_Lbound);
                        Serial.print("Enc_Midpoint: ");
                        Serial.println(Enc_Midpoint);
                        Serial.print("Enc_Ubound: ");
                        Serial.println(Enc_Ubound);
                        Encoder_boundrySet = 2;
                    }
                }
            }
        }
        lastlimit_switchState = limit_switch;
        Enc_reading = Enc.read();

        if (Encoder_boundrySet == 2)
        {
            if (Enc_reading == Enc_Midpoint)
            {
                digitalWrite(5, LOW);
                digitalWrite(6, LOW);
                Encoder_boundrySet++;
            }
        }
    }
}
