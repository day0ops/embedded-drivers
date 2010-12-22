/* GPS NMEA Data Parser
 *
 * Backend supports GPGGA, GPGLL, and GPRMC NMEA data types.
 *
 * Vanya A. Sergeev - <vsergeev@gmail.com> - copyright 2010
 * please inform author of possible use, licensing is still being decided
 */

#include "gps.h"

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int GPS_NMEA_Find(char *nmea_data, int len, int *start, int *stop);
int GPS_NMEA_Checksum(char *nmea_string, int len);

/* Token Parsing Helper Functions */
char *nextToken(char **data, char *delimeter);
char ascii2hex(char hexchar);
char *returnToken(char *tokenData, int dataLen, int tokenIndex);

/* String Manipulation Helper Functions */
#ifdef CUSTOM_STRING_FUNCTIONS
#define NULL 		0
int strlen(const char *str);
char *strncpy(char *dest, const char *src, int len);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int len);
char *strstr(char *haystack, char *needle);
#endif

/* GPS Data Definitions */
#define GPS_DATA_GGA		(1<<0)
#define GPS_DATA_GLL		(1<<1)
#define GPS_DATA_RMC		(1<<2)

#define GPS_START_CHAR		'$'
#define GPS_END_CHAR		'*'

#define GPS_GGA_START		"$GPGGA"
#define GPS_GGA_INDEX_ID	0
#define GPS_GGA_INDEX_UTC_TIME	1
#define GPS_GGA_INDEX_LATITUDE	2
#define GPS_GGA_INDEX_N_S	3
#define GPS_GGA_INDEX_LONGITUDE	4
#define GPS_GGA_INDEX_E_W	5
#define GPS_GGA_INDEX_QUALITY	6
#define GPS_GGA_INDEX_NUM_SATELLITES	7
#define GPS_GGA_INDEX_HDOP		8
#define GPS_GGA_INDEX_ALTITUDE		9
#define GPS_GGA_INDEX_UNITS_ALTITUDE 	10
#define GPS_GGA_INDEX_GEOIDAL_SEP	11
#define GPS_GGA_INDEX_UNITS_GEOIDAL_SEP	12
#define GPS_GGA_INDEX_DGPS_AGE		13
#define GPS_GGA_INDEX_DGPS_STATION_ID	14

#define GPS_GLL_START		"$GPGLL"
#define GPS_GLL_INDEX_ID	0
#define GPS_GLL_INDEX_LATITUDE	1
#define GPS_GLL_INDEX_N_S	2
#define GPS_GLL_INDEX_LONGITUDE	3
#define GPS_GLL_INDEX_E_W	4
#define GPS_GLL_INDEX_UTC_TIME	5
#define GPS_GLL_INDEX_VALID	6

#define GPS_RMC_START		"$GPRMC"
#define GPS_RMC_INDEX_ID	0
#define GPS_RMC_INDEX_UTC_TIME	1
#define GPS_RMC_INDEX_VALID	2
#define GPS_RMC_INDEX_LATITUDE	3
#define GPS_RMC_INDEX_N_S	4
#define GPS_RMC_INDEX_LONGITUDE	5
#define GPS_RMC_INDEX_E_W	6
#define GPS_RMC_INDEX_SPEED	7
#define GPS_RMC_INDEX_COURSE	8
#define GPS_RMC_INDEX_DATE	9

void GPS_Data_Clear(GPS_Data *gps_data) {
	gps_data->nmea_type = 0;
	gps_data->utc_time[0] = '\0';
	gps_data->latitude[0] = '\0';
	gps_data->n_s_indicator = '\0';
	gps_data->longitude[0] = '\0';
	gps_data->e_w_indicator = '\0';
	gps_data->altitude[0] = '\0';
	gps_data->units_altitude = '\0';
	gps_data->speed[0] = '\0';
	gps_data->course[0] = '\0';
	gps_data->utc_date[0] = '\0';
	gps_data->data_valid = '\0';
}

int GPS_NMEA_Type(char *nmea_string) {
	int data_available = 0;

	if (strstr(nmea_string, GPS_GGA_START) != NULL)
		data_available |= GPS_DATA_GGA;

	if (strstr(nmea_string, GPS_GLL_START) != NULL)
		data_available |= GPS_DATA_GLL;
	
	if (strstr(nmea_string, GPS_RMC_START) != NULL)
		data_available |= GPS_DATA_RMC;

	if (data_available > 0)
		return data_available;
	
	return -1;
}

int GPS_NMEA_Find(char *nmea_data, int len, int *start, int *stop) {
	int i, j, found;

	found = -1;
	for (i = 0; i < len; i++) {
		/* Find the start character of a NMEA data string */
		if (nmea_data[i] == GPS_START_CHAR) {
			for (j = i; j < len; j++) {
				/* Find the stop character of a NMEA data
				 * string */
				/* Check that the end character + 3 is within
				 * the length of the NMEA data.
				 * This is the end character, plus two checksum
				 * character, + 1 since the offset is from 0. */
				if (nmea_data[j] == GPS_END_CHAR && (j+3) < len) {
					/* Save the start/stop offsets */
					*start = i;
					*stop = j+3;
					/* Start now points to $, and stop
					 * points to the character immediately
					 * after the NMEA string checksum. */
					found = 0;
					break;
				}
			}
		}
		if (found == 0)
			break;
	}

	return found;
}

int GPS_NMEA_Extract_Next(char *nmea_data, int len, char *gps_string, int max_len, int *new_offset) {
	int retVal, start, stop;

	/* Set the new_offset to the beginning of the string in case we don't 
	 * find any usable GPS data this time around and will later. */
	*new_offset = 0;
	/* Find the offsets of the first usable GPS data string */ 
	retVal = GPS_NMEA_Find(nmea_data, len, &start, &stop);
	if (retVal < 0 || (stop-start) <= 0) 
		return -1;
	/* Since we've found a usable GPS string, set the offset to the end
	 * of this GPS string */
	*new_offset = stop;

	/* Checksum the GPS string to ensure it is valid */
	if (GPS_NMEA_Checksum(nmea_data+start, stop-start) < 0)
		return -1;

	/* Make sure we have enough space to fit this GPS string */
	if ((stop-start+1) > max_len)
		return -1;

	/* Copy the GPS substring to its own string */
	strncpy(gps_string, nmea_data+start, stop-start);
	/* Null-terminate the new GPS string */
	gps_string[stop-start] = '\0';

	return 0;
}

int GPS_NMEA_Parse_Next(char *gps_string, GPS_Data *gps_data) { 
	int nmea_data_type;
	int retVal = -1;
	
	/* Evaluate the data type of this GPS string */
	nmea_data_type = GPS_NMEA_Type(gps_string);
	if (nmea_data_type < 0)
		return -1;

	/* Parse the GPS string appropriately */
	if ((nmea_data_type & GPS_DATA_GGA))
		retVal = GPGGA_Parse(gps_string, gps_data);
	else if ((nmea_data_type & GPS_DATA_GLL))
		retVal = GPGLL_Parse(gps_string, gps_data);
	else if ((nmea_data_type & GPS_DATA_RMC))
		retVal = GPRMC_Parse(gps_string, gps_data);

	/* If we successfully parsed the NMEA data, save the nmea data type in
	 * the GPS_Data structure as well */
	if (retVal == 0)
		gps_data->nmea_type = nmea_data_type;

	return retVal;
}

int GPS_NMEA_Checksum(char *nmea_string, int len) {
	int i;
	unsigned char checksum;
	
	/* Calculate the checksum of the GPS data string. The
	 * checksum is the sum of the XOR of all characters,
	 * starting from after the $ to the end before the *. */
	checksum = 0;
	for (i = 1; i < len; i++) {
		/* Check if we are at the end of this GPS data message */
		if (nmea_string[i] == GPS_END_CHAR) {
			/* Make sure there is room for the checksum data */
			if (i+3 > len)
				return -1;
			/* Update the length of our GPS message data */
			len = i+3;
			/* Exit the checksum calculation loop */
			break;
		}
		checksum ^= nmea_string[i];
	}

	/* Compare the first checksum nibble */
	if ((checksum&0xF0)>>4 != ascii2hex(nmea_string[len-2]))
		return -1;
	/* Compare the second checksum nibble */
	if ((checksum&0x0F) != ascii2hex(nmea_string[len-1]))
		return -1;
	
	return 0;
}


/* NMEA-type Specific Data Extraction Functions */

/* Parse a GPS data string, if it is an GGA message ID, the time, latitude, 
 * longitude, and altitude are extracted and stored. */
int GPGGA_Parse(char *nmea_string, GPS_Data *gps_data) {
	int i, len;

	len = strlen(nmea_string);

	/* Assert the length */
	if (len <= 1)
		return -1;

	/* Check the GPS checksum of the GPS data */
	if (GPS_NMEA_Checksum(nmea_string, len) < 0) 
		return -1;
	
	/* Replace every , or * with a NULL character to separate GPS data into tokens */
	for (i = 0; i < len; i++) {
		if (nmea_string[i] == ',' || nmea_string[i] == '*')
			nmea_string[i] = 0;
	}

	/* Check that this GPS data string has a GPGGA message ID */
	if (strcmp(returnToken(nmea_string, len, GPS_GGA_INDEX_ID), GPS_GGA_START) != 0) 
		return -1;

	/* UTC Time */
	strncpy(gps_data->utc_time, returnToken(nmea_string, len, GPS_GGA_INDEX_UTC_TIME), sizeof(gps_data->utc_time)-1);
	gps_data->utc_time[sizeof(gps_data->utc_time)-1] = 0;

	/* Latitude */
	strncpy(gps_data->latitude, returnToken(nmea_string, len, GPS_GGA_INDEX_LATITUDE), sizeof(gps_data->latitude)-1);
	gps_data->latitude[sizeof(gps_data->latitude)-1] = 0;

	/* Latitude N/S Indicator */
	gps_data->n_s_indicator = *returnToken(nmea_string, len, GPS_GGA_INDEX_N_S);

	/* Longitude */
	strncpy(gps_data->longitude, returnToken(nmea_string, len, GPS_GGA_INDEX_LONGITUDE), sizeof(gps_data->longitude)-1);
	gps_data->longitude[sizeof(gps_data->longitude)-1] = 0;

	/* Longitude E/W Indicator */
	gps_data->e_w_indicator = *returnToken(nmea_string, len, GPS_GGA_INDEX_E_W);

	/* Altitude */
	strncpy(gps_data->altitude, returnToken(nmea_string, len, GPS_GGA_INDEX_ALTITUDE), sizeof(gps_data->altitude)-1);
	gps_data->altitude[sizeof(gps_data->altitude)-1] = 0;

	/* Units of the Altitude */
	gps_data->units_altitude = *returnToken(nmea_string, len, GPS_GGA_INDEX_UNITS_ALTITUDE);

	return 0;
}

/* Parse a GPS data string, if it is an GLL message ID, the time, latitude, 
 * and longitude are extracted and stored. */
int GPGLL_Parse(char *nmea_string, GPS_Data *gps_data) {
	int i, len;

	len = strlen(nmea_string);

	/* Assert the length */
	if (len <= 1)
		return -1;

	/* Check the GPS checksum of the GPS data */
	if (GPS_NMEA_Checksum(nmea_string, len) != 0)
		return -1;

	/* Replace every , or * with a NULL character to separate GPS data into tokens */
	for (i = 0; i < len; i++) {
		if (nmea_string[i] == ',' || nmea_string[i] == '*')
			nmea_string[i] = 0;
	}

	/* Check that this GPS data string has a GPGLL message ID */
	if (strcmp(returnToken(nmea_string, len, GPS_GLL_INDEX_ID), GPS_GLL_START) != 0)
		return -1;

	/* UTC Time */
	strncpy(gps_data->utc_time, returnToken(nmea_string, len, GPS_GLL_INDEX_UTC_TIME), sizeof(gps_data->utc_time)-1);
	gps_data->utc_time[sizeof(gps_data->utc_time)-1] = 0;

	/* Latitude */
	strncpy(gps_data->latitude, returnToken(nmea_string, len, GPS_GLL_INDEX_LATITUDE), sizeof(gps_data->latitude)-1);
	gps_data->latitude[sizeof(gps_data->latitude)-1] = 0;

	/* Latitude N/S Indicator */
	gps_data->n_s_indicator = *returnToken(nmea_string, len, GPS_GLL_INDEX_N_S);

	/* Longitude */
	strncpy(gps_data->longitude, returnToken(nmea_string, len, GPS_GLL_INDEX_LONGITUDE), sizeof(gps_data->longitude)-1);
	gps_data->longitude[sizeof(gps_data->longitude)-1] = 0;
	
	/* Longitude E/W Indicator */
	gps_data->e_w_indicator = *returnToken(nmea_string, len, GPS_GLL_INDEX_E_W);

	/* Data from Satellite fix (A) or InValid (V) */
	gps_data->data_valid = *returnToken(nmea_string, len, GPS_GLL_INDEX_VALID);

	return 0;
}



/* Parse a GPS data string, if it is an RMC message ID, the time, latitude, 
 * longitude, speed, course, and date are extracted and stored. */
int GPRMC_Parse(char *nmea_string, GPS_Data *gps_data) {
	int i, len;

	len = strlen(nmea_string);

	/* Assert the length */
	if (len <= 1)
		return -1;

	/* Check the GPS checksum of the GPS data */
	if (GPS_NMEA_Checksum(nmea_string, len) != 0)
		return -1;

	/* Replace every , or * with a NULL character to separate GPS data into tokens */
	for (i = 0; i < len; i++) {
		if (nmea_string[i] == ',' || nmea_string[i] == '*')
			nmea_string[i] = 0;
	}

	/* Check that this GPS data string has a GPRMC message ID */
	if (strcmp(returnToken(nmea_string, len, GPS_RMC_INDEX_ID), GPS_RMC_START) != 0)
		return -1;

	/* UTC Time */
	strncpy(gps_data->utc_time, returnToken(nmea_string, len, GPS_RMC_INDEX_UTC_TIME), sizeof(gps_data->utc_time)-1);
	gps_data->utc_time[sizeof(gps_data->utc_time)-1] = 0;

	/* Data from Satellite fix (A) or InValid (V) */
	gps_data->data_valid = *returnToken(nmea_string, len, GPS_RMC_INDEX_VALID);
	/* Latitude */
	strncpy(gps_data->latitude, returnToken(nmea_string, len, GPS_RMC_INDEX_LATITUDE), sizeof(gps_data->latitude)-1);
	gps_data->latitude[sizeof(gps_data->latitude)-1] = 0;

	/* Latitude N/S Indicator */
	gps_data->n_s_indicator = *returnToken(nmea_string, len, GPS_RMC_INDEX_N_S);

	/* Longitude */
	strncpy(gps_data->longitude, returnToken(nmea_string, len, GPS_RMC_INDEX_LONGITUDE), sizeof(gps_data->longitude)-1);
	gps_data->longitude[sizeof(gps_data->longitude)-1] = 0;
	

	/* Longitude E/W Indicator */
	gps_data->e_w_indicator = *returnToken(nmea_string, len, GPS_RMC_INDEX_E_W);

	/* Speed */
	strncpy(gps_data->speed, returnToken(nmea_string, len, GPS_RMC_INDEX_SPEED), sizeof(gps_data->speed)-1);
	gps_data->speed[sizeof(gps_data->speed)-1] = 0;

	/* Course */
	strncpy(gps_data->course, returnToken(nmea_string, len, GPS_RMC_INDEX_COURSE), sizeof(gps_data->course)-1);
	gps_data->course[sizeof(gps_data->course)-1] = 0;

	/* Date */
	strncpy(gps_data->utc_date, returnToken(nmea_string, len, GPS_RMC_INDEX_DATE), sizeof(gps_data->utc_date)-1);
	gps_data->utc_date[sizeof(gps_data->utc_date)-1] = 0;
	
	return 0;
}

/* Token Helper Function central to the NMEA data extraction */

/* Returns the token specified by the token index of of a GPS data string 
 * that has been separated into tokens by NULL characters. */
char *returnToken(char *tokenData, int dataLen, int tokenIndex) {
	int i;
	char *token = tokenData;
	for (i = 0; i < tokenIndex; i++) {
		/* If the next token is out of bounds, return NULL */
		if ((token-tokenData)+strlen(token) >= dataLen)
			return NULL;
		/* Move along to the next token */
		token += strlen(token)+1;
	}
	return token;
}

/* Converts an ASCII Hex character to its numerical value,
 * returns -1 if the conversion was unsuccessful. */
char ascii2hex(char hexchar) {
	switch (hexchar) {
		case '0': return 0x0;
		case '1': return 0x1;
		case '2': return 0x2;
		case '3': return 0x3;
		case '4': return 0x4;
		case '5': return 0x5;
		case '6': return 0x6;
		case '7': return 0x7;
		case '8': return 0x8;
		case '9': return 0x9;
		case 'A': return 0xA;
		case 'B': return 0xB;
		case 'C': return 0xC;
		case 'D': return 0xD;
		case 'E': return 0xE;
		case 'F': return 0xF;
		default:
			return -1;
	}
}

/* Some basic string manipulation functions */

#ifdef CUSTOM_STRING_FUNCTIONS

int strlen(const char *str) {
	int len;

	for (len = 0; *str != '\0'; str++)
		len++;

	return len;
}


char *strncpy(char *dest, const char *src, int len) {
	int i;
	char *orig_dest = dest;

	for (i = 0; i < len; i++, dest++) {
		if (*src == '\0')
			*dest = '\0';
		else {
			*dest = *src;
			src++;
		}
	}

	return orig_dest;
}


int strcmp(const char *s1, const char *s2) {
	int result;

	for (result = 0; *s1 != '\0' && *s2 != '\0'; s1++, s2++) {
		if (*s1 == *s2)
			;
		else if ((unsigned char)*s1 < (unsigned char)*s2)
			return -1;
		else
			return 1;
	}
	if (*s1 == '\0' && *s2 != '\0')
		return -1;
	else if (*s1 != '\0' && *s2 == '\0')
		return 1;
	
	return result;
}

int strncmp(const char *s1, const char *s2, int len) {
	int result;

	for (result = 0; *s1 != '\0' && *s2 != '\0' && len > 0; s1++, s2++, len--) {
		if (*s1 == *s2)
			;
		else if ((unsigned char)*s1 < (unsigned char)*s2)
			return -1;
		else
			return 1;
	}
	if (len == 0)
		return 0;
	else if (*s1 == '\0' && *s2 != '\0')
		return -1;
	else if (*s1 != '\0' && *s2 == '\0')
		return 1;

	return result;
}

char *strstr(char *haystack, char *needle) {
	int len_needle;

	len_needle = strlen(needle);
	for (; *haystack != '\0'; haystack++) {
		if (strncmp(needle, haystack, len_needle) == 0)
			return haystack;
	}

	return NULL;
}
#endif
