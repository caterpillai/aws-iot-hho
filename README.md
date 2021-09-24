# Healthy Home Office

This project was designed as a submission for the [Reinventing Healthy Spaces with Amazon Web Services](https://www.hackster.io/contests/Healthy-Spaces-with-AWS) hackathon. The initial proposal ([detailed here](https://www.hackster.io/contests/Healthy-Spaces-with-AWS/hardware_applications/13847)) was called `Healthy Workspace`, but has since been renamed to `Healthy Home Office`.

In short, office spaces for knowledge workers often control various factors to ensure the health and productivity of their employees. There have been many studies documenting [what makes an office building "healthy"](https://hbr.org/2020/04/what-makes-an-office-building-healthy), but, as many of us are working from home, these factors are not being actively monitored and adjusted. A simple solution would be a small, unobtrusive device that measures the critical components of a healthy workspace (temperature, light intensity, noise levels and air quality/CO2 levels), and provides recommendations to a user when a given factor should be adjusted (i.e. the room is too bright and could result in migranes).

In theory, this could be done completely on the device (assuming it is not triggering other devices in the home office like a fan/ the AC). However, everyone's environment and their ideal working conditions are different, so one set of blanket recommendations would not be useful to most users. For that reason, the generation of recommendations happens on AWS, where initial recommendations can be preprogrammed (and adjusted in real-time) and where the data can be aggregated and leveraged in an ML model to provide more personalized recommendations that stay within the healthy/productive guidelines identified by the research.

![Basic Architecture Diagram](https://github.com/caterpillai/aws-iot-hho/blob/main/HHO_v1_AWS_Diagram.png)

For a quick demo of the current capabilities of the device, check out [this video](https://www.youtube.com/watch?v=8kJCjdPjQkM).


## Setup

This project builds upon the [Smart Spaces](https://edukit.workshop.aws/en/smart-spaces.html) tutorial (last accessed on 09/19/2021) detailed on the AWS IoT EduKit workshop [page](https://edukit.workshop.aws/en/). Please make sure the pre-requisites for that tutorial are met before proceeding.


### Materials

1. [AWS IoT EduKit](https://aws.amazon.com/iot/edukit/)
1. [M5 Stack Light Sensor](https://shop.m5stack.com/products/light-sensor-unit) - or another photoresistor with GPIO connectivity to `Port B` of the AWS IoT EduKit (communicates via DAC/ADC)
1. [M5 Stack Gas Sensor](https://shop.m5stack.com/products/tvoc-eco2-gas-unit-sgp30) - or another unit like the SGP30 that uses the I2C protocol with GPIO connectivity to `Port A` of the AWS IoT EduKit

*Note: Grove cables are needed to connect the sensors linked above to the AWS IoT EduKit.*


### On Device Setup

1. Finishing the prerequisites described in the [Smart Spaces](https://edukit.workshop.aws/en/smart-spaces.html) tutorial mentioned above are crucial. Those prerequisites ensure that:
    1. The code from this project can be flashed to the target AWS IoT EduKit hardware
    1. The target hardware can already communicate securely with AWS IoT Core via MQTT
    1. The appropriate services can be provisioned via AWS to support recommendation generation and data storage
1. Attach the additional sensors. For the purposes of this project, the light sensor (photoresistor) was attached to `Port B` while the gas sensor was attached to `Port A` via Grove cable.
1. Open this project using `pio` and flash the code to the target hardware. If following along with the previously linked tutorial(s), this should be the command:
`pio run --environment core2foraws --target upload --target monitor`


### On AWS Setup

In addition to prerequisites from the external tutorials linked above, there are a few things that need to be provisioned in AWS for recommendations to be generated and sent back to the device.

#### - Set a basic rule to test recommendation generation

> **_NOTE:_**  This is **Optional**. The lastest demo video uses recommendations generated in this manner, but this serves as a test to ensure that the device can properly communicate with AWS IoT Core.

Using the instructions from [this part of the Smart Thermostat](https://edukit.workshop.aws/en/smart-thermostat/data-transforms-and-routing.html) tutorial, create an IoT Core rule with the contents of the [test_aws_iot_rule.sql](https://github.com/caterpillai/aws-iot-hho/blob/main/test_aws_iot_rule.sql) (feel free to ignore the comments). Be sure to update the topic referenced in the last line of the query and republish to the appropriate device's `update` topic.


#### - Set up an AWS Lambda instance to process recommendations and send data to AWS IoT Analytics

> **_NOTE:_** After working with AWS IoT Analytics more, we believe it would be easier to add pipeline processing as opposed to triggering the lambda function on every update message. A new approach leveraging AWS IoT Analytics functionality is being tested currently.

1. In AWS Lambda, create a function using `python 3.8` (or equivalent runtime), and paste the code from [aws_lambda_update_desired.py](https://github.com/caterpillai/aws-iot-hho/blob/main/aws_lambda_update_desired.py) from this repository.
2. Provision an AWS Iot Analytics channel (for this we used the name `aws_iot_hho_channel`), but do not associate it with your message key
3. Update the `<<device_id>>` and `<<channel_name>>` in the script so that recommendations can be sent to the right place and data can be stored appropriately.
4. Update the AWS Lambda function with the appropriate permissions
    1. In `IAM`, we created two policies (one with `Action` =`iot:UpdateThingShadow` and another with `Action`=`iotanalytics:BatchPutMessage`) and the appropriate resource `arn`s.
    1. These policies were attached to the lambda function - giving it the right permissions to write where it needed to. Without attaching these policies, the lambda function will not work.
5. Test the script using the `hho_test_input.json` file. **If there are permission errors here (i.e. errors like: `ForbiddenException`), they need to be resolved before creating and enabling the rule.**
6. Create a rule in AWS IoT Core to trigger the lambda function on each update.

#### - Train a SageMaker model using the data in S3

> **_NOTE:_**  The model for this project is currently being trained. This step is currently **OPTIONAL**, but the documentation will be updated when the recommendation generations switches to finished model.

Steps for this can be found [here](https://edukit.workshop.aws/en/smart-spaces/machine-learning.html) using the S3 bucket described above as the data source.


## Remaining Items/ TODOs

* On Device
    * UI
        * Use the LEDs and motor on device to identify when a new recommendation has come in from the cloud
        * Update each measure on the `Measurements` page with a unique border if they are at `WARNING` or `DANGEROUS` levels
    * Base Functionality 
        * Properly baseline the gas sensor before the first read
        * Adjust the photoresistance readout to a human-readable unit (e.g. lumens)
* On AWS
    * Leverage the SageMaker model using serverless caller ([described here](https://edukit.workshop.aws/en/smart-spaces/working-with-ml-models.html))
    * Enable user feedback on recommendations to help personalize SageMaker recommendations

## Additional Credits/Shoutouts

Thanks to the [Hackster.io]() team for coordinating this hackathon, the AWS IoT EduKit team for putting together the contest, providing clear and useful documentation and answering questions promptly on the hackathon forums and the M5Stack team for making easily-to-use peripherals.
