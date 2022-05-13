# Low-energy-WI-FI-general-purpose-sensor
Testing the possibility of long-term operation of the general-purpose sensor with battery power and publishing information on mqtt broker

## HW

Schematics:

![Alt Text](https://github.com/mrGrodzki/Low-energy-WI-FI-general-purpose-sensor/blob/main/HW/schematicPowTestPar1.png)

![Alt Text](https://github.com/mrGrodzki/Low-energy-WI-FI-general-purpose-sensor/blob/main/HW/schematicPowTestPar2.png)


![Alt Text](https://github.com/mrGrodzki/Low-energy-WI-FI-general-purpose-sensor/blob/main/HW/powTest_v1.jpg)

## SW

The program sends a JSON object to the MQTT broker every 10 seconds:
```
{
  "status":true,
  "LED":
  {
    "R":0,
    "G":0,
    "B":0,
    "status":false
  },
  "OC_W":false,
  "OC_E":false,
  "scrt":false,
  "light":false,
  "temp":0,
  "scrtStan":false
 }
```

## Tests

Power profile:
Current transformer: 10mV=mA;
V(avg)=729mV ----- = 72mA(avg) for sending data(200 bajt) and connect to WI-FI;
Duration of the transfer = 6s;

Deep sleep = 10uA;

### If it is sending 200 bytes every hour, the average power consumption is: 13uA;


![Alt Text](https://github.com/mrGrodzki/Low-energy-WI-FI-general-purpose-sensor/blob/main/Test/PowerProfile.png)


