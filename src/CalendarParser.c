/*
  Peter Hudel
  1012673
  CIS*2750
*/

#include "CalendarParser_A2temp2.h"
#include "HelperFunctions.h"

ICalErrorCode createCalendar(char* fileName, Calendar** obj) {
    //Variables go here
    FILE *fp;
    char *first, *ptr, *x;
    char line[10000];
    char prevLine[10000];
    char *temp;
    Event *evt = NULL;
    Alarm *alm = NULL;
    ICalErrorCode err = OK;
    int lineCount = 0;
    int addCount = 0;
    char *isICS;
    int isEvent = 0;
    int isAlarm = 0;
    int isUnfolding = 0;
    int isVersion = 0;
    int idk = 0;
    //Check if filename is null
    if (fileName == NULL) {
        return INV_FILE;
    }
    //Create an iCal struct
    *obj = malloc(sizeof(Calendar));

    char fileNameCopy[strlen(fileName)];
    fp = fopen(fileName, "r");
    //Check if the file has been opened properly
    if (fp == NULL) {
        free(*obj);
        *obj = NULL;
        return INV_FILE;
    }
    //Check for valid file type
    strcpy(fileNameCopy, fileName);
    isICS = strtok(fileNameCopy, ".");
    isICS = strtok(NULL, ".");
    if (strcmp(isICS, "ics") != 0) {
        free(*obj);
        *obj = NULL;
        fclose(fp);
        return INV_FILE;
    }
    //If good, initialize structure
    (*obj)->version = -1;
    strcpy((*obj)->prodID, "");
    (*obj)->properties = initializeList((*printProperty), (*deleteProperty), (*compareProperties));
    (*obj)->events = initializeList((*printEvent), (*deleteEvent), (*compareEvents));

    //Main loop for reading the file
    while (fgets(line, sizeof(line), fp)) {
        //Check if the line is too long: must be less than 77 characters including /r/n, as it
        if (strlen(line) > 77) {
            fclose(fp);
            deleteCalendar(*obj);
            if (evt != NULL) {
                deleteEvent(evt);
            }
            if (alm != NULL) {
              deleteAlarm(alm);
            }
            *obj = NULL;
            return INV_FILE;
        }
        //Check if the line ends in /r/n
        if ((line[strlen(line) - 1] != '\n') || (line[strlen(line) - 2] != '\r')) {
            fclose(fp);
            deleteCalendar(*obj);
            if (evt != NULL) {
                deleteEvent(evt);
            }
            if (alm != NULL) {
              deleteAlarm(alm);
            }
            *obj = NULL;
            return INV_FILE;
        }
        //Strip \r\n from line for parsing purposes
        strtok(line, "\r\n");
        //Check for comments
        if (line[0] != ';') {
        	if (lineCount == 0) {
        		strcpy(prevLine, line);
        	}
        	//Determine what state the program is in
        	if (lineCount > 0) {
				//line unfolding
	            //Checks if it is not the first line, and if it has whitespace at the beginning
	            if ((line[0] == ' ') || (line[0] == '\t')) {
	                //make temp string
	                char temp[strlen(line) + strlen(prevLine)];
	                //Append previous part to temp
	                strcpy(temp, prevLine);
	                //Remove first spaces
	                x = strtok(line, "");
	                memmove(x, x+1, strlen(x));
	                //Add to end of list
	                strcat(temp, x);
	                strcpy(prevLine, temp);
	                lineCount++;
	                isUnfolding = 1;
	            }
	            //If the current line does NOT need unfolding, go here
	            else {
	                isUnfolding = 0;
	                if (addCount == 0) {
	                	if (strcmp(prevLine, "BEGIN:VCALENDAR") != 0) {
					        deleteCalendar(*obj);
					        *obj = NULL;
					        return INV_CAL;
					    }
					    addCount++;
	                }
	                //Seperate into first and last part of line, and add to calendar
	                ptr = strtok(prevLine, ":;");
	                first = ptr;
	                ptr = strtok(NULL, "");
	                printf("%s---%s\n", first, ptr);
	                //Check to see if the property value is a string
	                if (ptr == NULL) {
	                    //If it is invalid, we need to determine what state the program is in.
	                    if ((isEvent == 0) && (isAlarm == 0)) {
	                        if (strcmp(first, "VERSION") == 0) {
	                            err = INV_VER;
	                        }
	                        else if (strcmp(first, "PRODID") == 0) {
	                            err = INV_PRODID;
	                        }
	                        else {
	                            err = INV_CAL;
	                        }
	                    }
	                    else if ((isEvent == 1) && (isAlarm == 0)) {
	                        if (strcmp(first, "DTSTART") == 0) {
	                            err = INV_DT;
	                        }
	                        else if (strcmp(first, "DTSTAMP") == 0) {
	                            err = INV_DT;
	                        }
	                        else {
	                            err = INV_EVENT;
	                        }
	                    }
	                    else if ((isEvent == 1) && (isAlarm == 1)) {
	                            err = INV_ALARM;
	                    }
	                }
	                else {
	                	if (addCount != 0) {
							updateState(&isEvent, &isAlarm, first, ptr, &evt, obj, &alm, &err);
		                    if (err == OK) {
		                        //Determine what state the program is in
		                        if ((isEvent == 0) && (isAlarm == 0)) {
		                            addToCal(first, ptr, obj, isUnfolding, &err, &isVersion);
		                        }
		                        else if ((isEvent == 1) && (isAlarm == 0)) {
		                            addToEvent(first, ptr, obj, &evt, isUnfolding, &err);
		                        }
		                        else if ((isEvent == 1) && (isAlarm == 1)) {
		                            addToAlarm(first, ptr, &evt, &alm, isUnfolding);
		                        }
		                    }
	                	}
	                }
	                //Do error checking if it occurs in one of the addition functions
	                if (err == OK) {
	                  //This is absolutely spaghetti code, this if statement needs to be here otherwise everything gets beaned
	                  if (idk == 0) {
	                      idk = 1;
	                  }
	                }
	                //This part handles freeing the memory based on the error thrown
	                else if (err == INV_DT) {
	                    fclose(fp);
	                    deleteEvent(evt);
	                    deleteCalendar(*obj);
	                    *obj = NULL;
	                    return err;
	                }
	                else if (err == INV_EVENT) {
	                    fclose(fp);
	                    deleteEvent(evt);
	                    deleteCalendar(*obj);
	                    *obj = NULL;
	                    return err;
	                }
	                else if (err == INV_ALARM) {
	                    fclose(fp);
	                    deleteEvent(evt);
	                    deleteAlarm(alm);
	                    deleteCalendar(*obj);
	                    *obj = NULL;
	                    return err;
	                }
	                else {
	                    fclose(fp);
	                    deleteCalendar(*obj);
	                    *obj = NULL;
	                    return err;
	                }
	                strcpy(prevLine, line);
	            }
	        }
        }
        //Increment line counters
        lineCount++;
    }

    fclose(fp);

    //Error checking occurs here
    //Checks if last line is correct
    if (strcmp(line, "END:VCALENDAR") != 0) {
        deleteCalendar(*obj);
        *obj = NULL;
        return INV_CAL;
    }
    //If event is missing a closing statement at the end
    if (isEvent == 1) {
        deleteCalendar(*obj);
        deleteEvent(evt);
        *obj = NULL;
        return INV_EVENT;
    }
    //If alarm is missing a closing statement at the end
    if (isAlarm == 1) {
      deleteCalendar(*obj);
      deleteAlarm(alm);
      *obj = NULL;
      return INV_ALARM;
    }
    if ((*obj)->version == -1) {
        deleteCalendar(*obj);
        *obj = NULL;
        return INV_CAL;
    }
    //Check if it is the correct version/ if it exists
    if ((*obj)->version != 2.0) {
        deleteCalendar(*obj);
        *obj = NULL;
        return INV_VER;
    }
    //Check if prodID exists
    if (strlen((*obj)->prodID) == 0) {
        deleteCalendar(*obj);
        *obj = NULL;
        return INV_CAL;
    }
    //Check if the cal has at least one event
    temp = toString((*obj)->events);
    if (strcmp(temp, "") == 0) {
        deleteCalendar(*obj);
        free(temp);
        *obj = NULL;
        return INV_CAL;
    }
    free(temp);
    //Check if event is missing closing tag

    return OK;
}

void deleteCalendar(Calendar* obj) {
    //need to free: list of events (call delete event), list of properties
    freeList(obj->events);
    freeList(obj->properties);
    free(obj);
}

char* printCalendar(const Calendar* obj) {
	if (obj == NULL) {
		return "";
	}
    char *str;
    char version[4];
    char *temp = toString(obj->properties);
    char *eventTemp = toString(obj->events);
    str = malloc((49 + strlen(obj->prodID) + strlen(temp) + strlen(eventTemp)) *sizeof(char));
    strcpy(str, "Version: ");
    snprintf(version, sizeof(str), "%.1f", obj->version);
    strcat(str, version);
    strcat(str, " ProdID: ");
    strcat(str, obj->prodID);
    strcat(str, "\niCal Properties: ");
    strcat(str, temp);
    strcat(str, "\nEvents: ");
    strcat(str, eventTemp);
    free(temp);
    free(eventTemp);
    return str;
}

char* printError(ICalErrorCode err) {
    if (err == INV_FILE) {
        return "Invalid file";
    }
    else if (err == INV_CAL) {
        return "Invalid calendar";
    }
    else if (err == INV_VER) {
        return "Invalid version";
    }
    else if (err == DUP_VER) {
        return "Duplicate version";
    }
    else if (err == INV_PRODID) {
        return "Invalid product ID";
    }
    else if (err == DUP_PRODID) {
        return "Duplicate product ID";
    }
    else if (err == INV_EVENT) {
        return "Invalid event";
    }
    else if (err == INV_DT) {
        return "Invalid datetime";
    }
    else if (err == INV_ALARM) {
        return "Invalid alarm";
    }
    else {
        return "Other error";
    }
}

ICalErrorCode writeCalendar(char* fileName, const Calendar* obj) {
    char *isICS;
    char *tempString;
    char *tempString2;
    char *tempPropList;
    char version[4];
    FILE *fp;
    //Check for object or filename being null
    if (obj == NULL) {
        return WRITE_ERROR;
    }
    if (fileName == NULL) {
      return WRITE_ERROR;
    }
    char fileNameCopy[strlen(fileName)];

    //Check for valid file extension
    strcpy(fileNameCopy, fileName);
    isICS = strtok(fileNameCopy, ".");
    isICS = strtok(NULL, ".");
    if (strcmp(isICS, "ics") != 0) {
        return WRITE_ERROR;
    }

    //open/ create file for writing
    fp = fopen(fileName, "w");

    fputs("BEGIN:VCALENDAR\r\n", fp);
    //Allocate memory for a temporary string, and print version
    tempString = malloc(sizeof(char) * 14);
    strcpy(tempString, "VERSION:");
    snprintf(version, 4, "%.1f", obj->version);
    strcat(tempString, version);
    strcat(tempString, "\r\n");
    fputs(tempString, fp);
    free(tempString);

    //Allocate memory for prodID string, and print to file
    tempString2 = malloc(sizeof(char) * (10 + strlen(obj->prodID)));
    strcpy(tempString2, "ProdID:");
    strcat(tempString2, obj->prodID);
    fputs(tempString2, fp);
    free(tempString2);

    //Print list of cal properties to the file
    tempPropList = toString(obj->properties);
    fputs(tempPropList, fp);
    free(tempPropList);
    fputs("\r\n", fp);

    ListIterator iter = createIterator(obj->events);
    void* elem;
  	while((elem = nextElement(&iter)) != NULL){
        fputs("BEGIN:VEVENT\r\n", fp);
    		char* currDescr = serializeEvent(elem, &fp);
        fputs(currDescr, fp);
        free(currDescr);
        fputs("END:VEVENT\r\n", fp);

  	}
    fputs("END:VCALENDAR\r\n", fp);

    fclose(fp);
    return OK;
}

ICalErrorCode validateCalendar(const Calendar* obj) {
    ICalErrorCode err;
    err = OK;
    int calScaleCount = 0;
    int methodCount = 0;
    int i = 0;
    int isIn = 0;
    char *propList[2] = {"CALSCALE", "METHOD"};
    //Check for null pointer
    if (obj == NULL) {
        return INV_CAL;
    }

    //Check if version is valid
    if (obj->version != 2.0) {
        return INV_CAL;
    }
    //Check if prodID is valid
    if ((strlen(obj->prodID) >= 1000) || (strlen(obj->prodID) < 1)) {
        return INV_CAL;
    }
    //check if the lists are NULL
    if ((obj->events == NULL) || (obj->properties == NULL)) {
        return INV_CAL;
    }

    //Check if the events list is empty
    if ((obj->events->head == NULL) && (obj->events->tail == NULL)) {
        return INV_CAL;
    }

    if (obj->events == NULL) {
      return INV_CAL;
    }

    if (obj->properties == NULL) {
      return INV_CAL;
    }

    //Iterate through the list of properties
    ListIterator iter = createIterator(obj->properties);
    void* elem;
  	while((elem = nextElement(&iter)) != NULL){
    		Property *prop = (Property*)elem;
        isIn = 0;
        //Iterate through list of properties and determine if they should be in the list
        for (i = 0; i < 2; ++i) {
            if (strcmp(prop->propName, propList[i]) == 0) {
                isIn = 1;
            }
        }
        //If a prop should not be in the array, exit
        if (isIn == 0) {
            return INV_CAL;
        }
        //Checkhow many times certain properties can occur
        if (strcmp(prop->propName, "CALSCALE") == 0) {
            calScaleCount++;
            //Calscale can only ever be gregorian in this version of an iCal file
            if (strcmp(prop->propDescr, "GREGORIAN") != 0) {
                return INV_CAL;
            }
        }
        else if (strcmp(prop->propName, "METHOD") == 0) {
            methodCount++;
        }
        //If they occur more than once, return an error
        if ((calScaleCount > 1) || (methodCount > 1)) {
            return INV_CAL;
        }
  	}
    //Iterate through the list of events and validate that each event is valid
    ListIterator iter1 = createIterator(obj->events);
    void* elem1;
  	while((elem1 = nextElement(&iter1)) != NULL){
    		err = validateEvent(elem1);
        //Check if an error occurred when iterating through the events
        if (err != OK) {
            return err;
        }
  	}
    return err;
}

void deleteEvent(void* toBeDeleted){
    //Need to free: List of properties, list of alarms(should call deleteAlarm),
    //creationdatetime struct, startdatetime struct
    Event *evt = (Event*)toBeDeleted;
    freeList(evt->properties);
    freeList(evt->alarms);
    free(toBeDeleted);
}

int compareEvents(const void* first, const void* second) {
    return 0;
}

char* printEvent(void* toBePrinted) {
    Event *evt = (Event*)toBePrinted;
    char *str;
    char *tempCreateDate = printDate(&evt->creationDateTime);
    char *tempStartDate = printDate(&evt->startDateTime);
    char *tempAlarm = toString(evt->alarms);
    char *tempProp = toString(evt->properties);
    str = malloc(sizeof(char) * (strlen(tempAlarm) + strlen(tempProp) + strlen(evt->UID) + 47 + strlen(tempStartDate) + strlen(tempCreateDate)));
    strcpy(str, "UID: ");
    strcat(str, evt->UID);
    strcat(str, "\nCreation: ");
    strcat(str, tempCreateDate);
    strcat(str, "\nStart: ");
    strcat(str, tempStartDate);
    strcat(str, "\nProperties: ");
    strcat(str, tempProp);
    strcat(str, "\nAlarms: ");
    strcat(str, tempAlarm);
    free(tempAlarm);
    free(tempProp);
    free(tempStartDate);
    free(tempCreateDate);
    return str;
}

void deleteAlarm(void* toBeDeleted) {
    //(need to free: List of properties, trigger char* array)
    Alarm *alm = (Alarm*)toBeDeleted;
    freeList(alm->properties);
    free(alm->trigger);
    free(toBeDeleted);
}

int compareAlarms(const void* first, const void* second) {
    return 0;
}

char* printAlarm(void* toBePrinted) {
    Alarm *alm = (Alarm*)toBePrinted;
    char *str;
    char *tempProp = toString(alm->properties);
    str = malloc(sizeof(char) * (strlen(alm->action) + strlen(alm->trigger) + strlen(tempProp) + 32));
    strcpy(str, "Action: ");
    strcat(str, alm->action);
    strcat(str, " Trigger: ");
    strcat(str, alm->trigger);
    strcat(str, "\nProperties: ");
    strcat(str, tempProp);
    free(tempProp);
    return str;
}

void deleteProperty(void* toBeDeleted) {
    free(toBeDeleted);
}

int compareProperties(const void* first, const void* second) {
    Property *temp1;
    Property *temp2;

    if (first == NULL || second == NULL){
  		return 0;
  	}

    temp1 = (Property*)first;
    temp2 = (Property*)second;

    return (strcmp(temp1->propName, temp2->propName) && strcmp(temp1->propDescr, temp2->propDescr));
}

char* printProperty(void* toBePrinted) {
    char *str;
    Property *propToPrint = (Property*)toBePrinted;
    str = malloc(sizeof(char) * (strlen(propToPrint->propName) + strlen(propToPrint->propDescr) + 2));
    strcpy(str, propToPrint->propName);
    strcat(str, ":");
    strcat(str, propToPrint->propDescr);
    strcat(str, "\0");
    return str;
}

void deleteDate(void* toBeDeleted) {
    free(toBeDeleted);
}

int compareDates(const void* first, const void* second) {
    return 0;
}

char* printDate(void* toBePrinted) {
    DateTime *dt = (DateTime*)toBePrinted;
    char *str;
    str = malloc(sizeof(char) * (strlen(dt->date) + strlen(dt->time) + 21));
    strcpy(str, dt->date);
    strcat(str, "T");
    strcat(str, dt->time);
    if (dt->UTC) {
        strcat(str, "Z");
    }
    strcat(str, "\0");
    return str;
}
