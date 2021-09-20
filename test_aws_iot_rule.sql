/**
 * A test rule used in AWS_IoT core to ensure that device updates are being 
 * registered.
 * 
 * Note: <<device_id>> has been scrubbed and must be updated with the user's
 * device client id after registering the device with AWS IoT core using the
 * helper scripts. 
 */ 

SELECT 
  CASE state.reported.noiseLevel > 100 WHEN true 
    THEN 'Sound levels are too high!'
  END AS state.desired.notifications
  CASE state.reported.lightIntensity > 2000 WHEN true
    THEN "There's too little light!"
  END AS state.desired.notifications
  CASE state.reported.lightIntensity < 700 WHEN true
    THEN "There's too much light!"
  END AS state.desired.notifications
  CASE state.desired.notifications <> NULL WHEN true
    THEN 1
    ELSE 0
  END AS state.desired.notificationCount
  CASE state.desired.notifications <> NULL WHEN false
    THEN 'No new recommendations.'
  END AS state.desired.notifications
FROM '$aws/things/<<device_id>>/shadow/update/accepted'
