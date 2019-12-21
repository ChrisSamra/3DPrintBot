//Ben Weese Pearlman, Asha Mills Emmet, Christopher Samra, Jacob Dupius

//Christopher Samra worked on task main() as well as moveToPoint()

#include "PC_FileIO.c" //library created by Univeristy of Waterloo Proffessor Carol Hulls

const float TICKSTOMM = 22.5*PI/180;
tMotor extMotor = motorA;
tMotor XMotor = motorB;
tMotor YMotor = motorC;
tMotor ZMotor = motorD;

void moveToPoint(float xmove,float ymove, bool& eStop);
void readNumLayers(int numLayers, TFileHandle data);
void readPoint(float& x, float& y, int& extrude, int& moveToNextLayer, TFileHandle data);
void home();
void shutdown();
void goToNextLayer();
void printLayer(TFileHandle inFile);

task main()
{
	SensorType[S1] = sensorEV3_Touch;
	wait1Msec(50);
	SensorType[S2] = sensorEV3_Touch;
	wait1Msec(50);
	SensorType[S3] = sensorEV3_Touch;
	wait1Msec(50);

	home();

	displayString(2,"PRESS THE DOWN BUTTON FOR CUBE OR THE UP FOR PHONE CASE");

	TFileHandle cubeFile;
	bool cubeFileOK = openReadPC(cubeFile, "cubePrint.txt");
	TFileHandle phoneCaseFile;
	bool phoneCaseFileOK = openReadPC(cubeFile, "phoneCaseFilePrint.txt");

	while(!getButtonPress(buttonAny)){}

	int numLayers = 0;

	if(getButtonPress(buttonDown))
	{
		readNumLayers(numLayers, cubeFile);
		for(int currentLayer = 1; currentLayer <= numLayers; currentLayer++)
		{
			printLayer(cubeFile);
		}
		shutdown();
	}

	else if (getButtonPress(buttonUp))
	{
		readNumLayers(numLayers, phoneCaseFile);
		for(int currentLayer = 1; currentLayer <= numLayers; currentLayer++)
		{
			printLayer(phoneCaseFile);
		}
		shutdown();
	}

	else
	{
		displayString(3, "Invalid button pressed");
		wait1Msec(5000);
	}
	//printLayer( parameters );

}

//reads the number of layers in the file, only called once at the begining of printing
void readNumLayers(int& numLayers, TFileHandle data)
{
	readIntPC(data, numLayers);
}

//reads the next coordinate to move to, extrude(bool
void readPoint(float& x, float& y, int& extrude, int& moveToNextLayer, TFileHandle data)
{
	readFloatPC(data, x);
	readFloatPC(data, y);
	readIntPC(data, extrude);
	readIntPC(data, moveToNextLayer);
}


void moveToPoint(float xmove,float ymove, bool& eStop)
//depending on the x and y coordinates provided, the function moves the baseplate x and y motors diagonally or in vertical/horizontal straight line
{

	nMotorEncoder[XMotor] = 0;//x
	nMotorEncoder[YMotor] = 0;//y
	const int ENC_LIMX = (xmove*360/(2*PI*2.75))/10; // divide by 10 gets mm value
	const int ENC_LIMY = (ymove*360/(2*PI*2.75))/10; //2.75 placeholder, whatever the gear radius is can be subbed in

	int baseMPower =15;

	if(ENC_LIMX ==0 || ENC_LIMY ==0)
	{

		motor[XMotor] = baseMPower;
		while(nMotorEncoder[XMotor] < ENC_LIMX){}
		motor[XMotor] = 0;

		motor[YMotor] = baseMPower;
		while(nMotorEncoder[YMotor] < ENC_LIMY){}
		motor[YMotor] = 0;


	}
	else
	{

		float para = ENC_LIMY/ENC_LIMX;
		float theta = atan(para);
		float xPower = baseMPower*cos(theta);
		float yPower = baseMPower*sin(theta);

		motor[XMotor] = xPower;
		motor[YMotor] = yPower;

		while(nMotorEncoder[XMotor] < ENC_LIMX && nMotorEncoder[YMotor] < ENC_LIMY && !getButtonPress(buttonAny))
		{}
		if(getButtonPress(buttonAny))
		{
			while(getButtonPress(buttonAny))
			{}
			eStop = 1;
			shutdown();
		}

		motor[XMotor] = motor[YMotor] = 0;
	}

}

void homePoint(tMotor myMotor, tSensors mySensor, int motorPower)
{
	motor[myMotor] = motorPower; //assume motor a is the x direction motor
	while(!SensorValue(mySensor)) // check for proper use of sensor
	{}
	motor[myMotor] = -motorPower;
	wait1Msec(1000); // test for best time of wait
	motor[myMotor] = motorPower;
	while(!SensorValue(mySensor))
	{}
	motor[myMotor] = 0;
}


void home()
{
	int power = 15; // need proper number and could be const
	motor[XMotor] = motor[YMotor] = motor[ZMotor] = 0;
	homePoint(XMotor, S1, power); // for x
	homePoint(YMotor, S2, power);// for y
	homePoint(ZMotor, S3, power); // for z

	displayString(3, "Press Enter to start print");

	while(!getButtonPress(buttonEnter)) //proper button input needed
	{}
	while(getButtonPress(buttonEnter))
	{}
	eraseDisplay();
}

void shutdown()
{
	motor[ZMotor] = -15; //speed to raise z motor
	while(nMotorEncoder[ZMotor] < 50) // replace 50 with the encoder value for raising the motor
	{}
	motor[ZMotor] = 0;
	displayString(6, "Press down when the glue gun is cooled"); // might be too long

	while(!getButtonPress(buttonDown))
	{}
	while(getButtonPress(buttonDown))
	{}
	//Could try and center the perpindicular direction
	eraseDisplay();

	homePoint(XMotor, S1, 15); //15 could be the const int and its the direction towards user

	displayString(5, "It is safe to take the object");
	wait1Msec(10000);
}



void printLayer(TFileHandle inFile)
{
	const float EXTRUDERPOWER = 30;
	//input file syntax
	float x = 0;
	float y = 0;
	bool eStop = 0;
	int extrude = 0;
	int nextLayer = 0;
	while(!nextLayer && !eStop)
	{
		readPoint(x, y, extrude, nextLayer, inFile);
		if(extrude)
		{
			motor[extMotor] = EXTRUDERPOWER;
		}
		moveToPoint(x, y, eStop);
	}
	if (!eStop)
	{
		goToNextLayer();
	}
}

void goToNextLayer()
{
	const float DRYTIME = 120000, Z_POWER = 30;
	clearTimer(T1);
	motor[ZMotor] = Z_POWER;
	while(nMotorEncoder[ZMotor] < 5*TICKSTOMM && !getButtonPress(buttonEnter))
	{}
	motor[ZMotor] = 0;
	while(time1[T1] < DRYTIME && !getButtonPress(buttonEnter))
	{}
}

/*
sample file:
my function will be called inside a loop that read the first number

number of layers
int X, int Y, bool Extrude, bool moveToNextLayer

2
4 0 1 0
0 4 1 0
-4 0 1 0
0 -4 1 1
4 0 0 0
0 4 0 0
-4 0 0 0
0 -4 0 1

//moves in a square while extruding then moves to the next layer, draws a square without extruding
*/
