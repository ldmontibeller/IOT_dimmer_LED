/**
 * This example program reads the current board temperature of a Raspberry PI 
 * and delivers it to a ThingWorx server.
 */
#include "twOSPort.h"
#include "twLogger.h"
#include "twApi.h"

#include <stdio.h>
#include <string.h>

/* Name of the thing you are binding to. */
char * thingName = "PIDataCollector1";

/* Server Details */
#define TW_HOST "dev-hamk.elisaiot.com" // Set this to your ThingWorx server name
#define TW_APP_KEY "use-the-appKey-generated-by-the-platform" // Change this to a appKey from your server

/* A simple structure to store current property values. */
struct  {
    double BoardTemperature;
    char * DeviceName;
} properties;

/** 
 * Reads the current board temperature of this raspberry PI from the /sys tree. 
 */
double getBoardTemperature(){
    FILE *temperatureFile;
    double T;
    temperatureFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temperatureFile == NULL) {
        TW_LOG(TW_FORCE,"device file /sys/class/thermal/thermal_zone0/temp failed to open.");
        return 0;
    }
    fscanf (temperatureFile, "%lf", &T);
    T /= 1000;
    //T = T * 1.8f + 32;
    //printf ("The temperature is %6.3f F.\n", T);
    fclose (temperatureFile);
    return T;
}

/**
 * Sends property values stored on your PI back to the ThingWorx server. You must
 *  add your properties to this list for them to be updated on the server.
 */
void sendPropertyUpdate() {

	/* Create the property list */
	propertyList * proplist = twApi_CreatePropertyList("BoardTemperature",twPrimitive_CreateFromNumber(properties.BoardTemperature), 0);
	if (!proplist) {
		TW_LOG(TW_ERROR,"sendPropertyUpdate: Error allocating property list");
		return;
	}
	twApi_AddPropertyToList(proplist,"DeviceName",twPrimitive_CreateFromString(properties.DeviceName,TRUE), 0);
	twApi_PushProperties(TW_THING, thingName, proplist, -1, FALSE);
	twApi_DeletePropertyList(proplist);
}

/**
 * Called every DATA_COLLECTION_RATE_MSEC milliseconds, this function is responsible for polling
 * and updating property values.
 */
#define DATA_COLLECTION_RATE_MSEC 2000
void dataCollectionTask(DATETIME now, void * params) {
    properties.BoardTemperature = getBoardTemperature();
	sendPropertyUpdate();
}

/**
 * This function is responsible for processing read and write requests from the server for the BoardTemperature property.
 */
enum msgCodeEnum propertyHandlerBoardTemperature(const char * entityName, const char * propertyName,  twInfoTable ** value, char isWrite, void * userdata) {
    if (value) {
        if (isWrite && *value) {
            // Value is being set from the server
            // We are not supporting writes to this property
        } else {
            // Value is being read from the server
            *value = twInfoTable_CreateFromNumber(propertyName, properties.BoardTemperature);
        }
        return TWX_SUCCESS;
    } else {
        TW_LOG(TW_ERROR,"Error updating value");
    }
    return TWX_BAD_REQUEST;
}

/**
 * This function is responsible for processing read and write requests from the server for the DeviceName property.
 */
enum msgCodeEnum propertyHandlerDeviceName(const char * entityName, const char * propertyName,  twInfoTable ** value, char isWrite, void * userdata) {
    if (value) {
        if (isWrite && *value) {
            // Value is being set from the server
            // We are not supporting writes to this property
        } else {
            // Value is being read from the server
             *value = twInfoTable_CreateFromString(propertyName, properties.DeviceName, TRUE);
        }
        return TWX_SUCCESS;
    } else {
        TW_LOG(TW_ERROR,"Error updating value");
        return TWX_BAD_REQUEST;
    }
}

/**
 * This is the main entry point of this application.
 * It is responsible for establishing a ThingName, registering properties
 * and establishing your connection to the servers.
 */
int main( int argc, char** argv ) {

#if defined NO_TLS
	int16_t port = 8080;
#else
	int16_t port = 443;
#endif
	int err = 0;
	DATETIME nextDataCollectionTime = 0;

	twLogger_SetLevel(TW_TRACE);
	twLogger_SetIsVerbose(1);
	TW_LOG(TW_FORCE, "Starting up");

	/* Initialize Properties */
    properties.BoardTemperature = 0.0;
    properties.DeviceName = "My Raspberry PI";

	/* Initialize the API */
	err = twApi_Initialize(TW_HOST, port, TW_URI, TW_APP_KEY, NULL, MESSAGE_CHUNK_SIZE, MESSAGE_CHUNK_SIZE, TRUE);
	if (err) {
		TW_LOG(TW_ERROR, "Error initializing the API");
		exit(err);
	}

	/* Regsiter our properties */
    twApi_RegisterProperty(TW_THING, thingName, "DeviceName", TW_STRING, NULL, "ALWAYS", 0, propertyHandlerDeviceName, NULL);
    twApi_RegisterProperty(TW_THING, thingName, "BoardTemperature", TW_NUMBER, NULL, "ALWAYS", 0, propertyHandlerBoardTemperature, NULL);

	/* Bind our thing - Can bind before connecting or do it when the onAuthenticated callback is made.  Either is acceptable */
	twApi_BindThing(thingName);

	/* Connect to server */
	err = twApi_Connect(CONNECT_TIMEOUT, twcfg.connect_retries);
	if (!err) {
		/* Register our "Data collection Task" with the tasker */
		twApi_CreateTask(DATA_COLLECTION_RATE_MSEC, dataCollectionTask);
	} else {
		TW_LOG(TW_ERROR,"main: Server connection failed after %d attempts.  Error Code: %d", twcfg.connect_retries, err);
	}

	while(TRUE) {
		DATETIME now = twGetSystemTime(TRUE);
		twApi_TaskerFunction(now, NULL);
		twMessageHandler_msgHandlerTask(now, NULL);
		if (twTimeGreaterThan(now, nextDataCollectionTime)) {
			dataCollectionTask(now, NULL);
			nextDataCollectionTime = twAddMilliseconds(now, DATA_COLLECTION_RATE_MSEC);
		}
		twSleepMsec(5);
	}
	twApi_UnbindThing(thingName);
	twSleepMsec(100);

	twApi_Delete(); 
	exit(0);
}
