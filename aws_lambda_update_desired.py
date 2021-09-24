import boto3
import json

# Lambda function that takes the reported state and publishes new notifications
# back to the 'update' topic.
#
# Note: the lambda function will also need permisions to publish to IoT topics,
# see: https://docs.aws.amazon.com/iot/latest/developerguide/lambda-rule-action.html.
#
# Thresholds based on articles like:
# https://hbr.org/2020/04/what-makes-an-office-building-healthy
#
def lambda_handler(event, context):
    deviceShadowClient = boto3.client('iot-data')
    
    # TODO(caterpillai): squash input using incoming rule
    reported = event['state']['reported']
    temperature = reported['temperature']
    sound = reported['noiseLevel']
    light = reported['lightIntensity']
    tvoc = reported['tvoc']
    eCO2 = reported['eCO2']
    
    notifications = []
    
    if (temperature >= 100):
        notifications.append("Dangerously high temperature!")
    elif (temperature >= 80):
        notifications.append("Its a little warm in here.")
    elif (temperature < 70):
        notifications.append("Its a little chilly in here.")
    elif (temperature < 55):
        notifications.append("Dangerously low temperature!")
    
    if (sound >= 200):
        notifications.append("Dangerously high levels of noise!")
    elif (sound >= 50):
        notifications.append("Its a little too noisy.")
    
    # Assumes that the regular resistance numbers are being sent back for now
    # TODO(caterpillai): change to handle lumens
    if (light >= 2250):
        notifications.append("Its a little too dark in here.")
    elif (light <= 1000):
        notifications.append("Its a little too bright in here.")
        
    if (tvoc >= 20):
        notifications.append("Dangerous air quality levels!")
    elif (tvoc >= 5):
        notifications.append("TVOC is a little high.")
        
    # Assumes ppm
    if(eCO2 >= 5000):
        notifications.append("Dangerous ambient CO2 levels!")
    elif(eCO2 >= 500):
        notifications.append("High CO2 levels - maybe open a window?")
    elif(eCO2 < 100):
        notifications.append("CO2 levels seem a bit low.")
    elif(eCO2 < 50):
        notifications.append("Dangerously low CO2 levels!")
    
    # Overwrite the desired notifications.
    event['state']['desired'] = {}
    event['state']['desired']['notifications'] = '\n'.join(notifications)
    event['state']['desired']['notificationCount'] = len(notifications)
    
    payload = json.dumps(event).encode('utf-8')

    # Update IoT Analytics dataset
    iotAnalyticsClient = boto3.client('iotanalytics')
    response = iotAnalyticsClient.batch_put_message(
        channelName='<<channel_name>>', # replace with actual channel name
        messages=[
            {
              'messageId': event['clientId'],
              'payload': payload
            },
            ]
        )
    
    if (len(response['batchPutMessageErrorEntries']) > 0):
        # Log the entire error in CloudWatch
        print(response)
    
    # Update shadow device
    deviceShadowClient = boto3.client('iot-data')
    print("Updating Shadow device with %s", payload)
    response = deviceShadowClient.update_thing_shadow(
        thingName='<<device_id>>', # replace with actual client id
        payload=payload)

